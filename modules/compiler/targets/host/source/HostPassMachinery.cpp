// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
/// Host's LLVM pass machinery interface.

#include <base/pass_pipelines.h>
#include <compiler/module.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/compute_local_memory_usage_pass.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/make_function_name_unique_pass.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/pipeline_parse_helpers.h>
#include <compiler/utils/remove_exceptions_pass.h>
#include <compiler/utils/remove_fences_pass.h>
#include <compiler/utils/remove_lifetime_intrinsics_pass.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/unique_opaque_structs_pass.h>
#include <compiler/utils/verify_reqd_sub_group_size_pass.h>
#include <compiler/utils/work_item_loops_pass.h>
#include <host/add_entry_hook_pass.h>
#include <host/add_floating_point_control_pass.h>
#include <host/disable_neon_attribute_pass.h>
#include <host/host_pass_machinery.h>
#include <host/remove_byval_attributes_pass.h>
#include <host/target.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/bit.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <multi_llvm/llvm_version.h>
#include <utils/system.h>
#include <vecz/pass.h>

namespace host {

bool hostVeczPassOpts(llvm::Function &F, llvm::ModuleAnalysisManager &MAM,
                      llvm::SmallVectorImpl<vecz::VeczPassOptions> &Opts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (vecz_mode != compiler::VectorizationMode::ALWAYS &&
      vecz_mode != compiler::VectorizationMode::AUTO) {
    return false;
  }
  // We only vectorize kernels
  if (!compiler::utils::isKernelEntryPt(F)) {
    return false;
  }
  // Handle required sub-group sizes
  if (auto reqd_subgroup_vf = vecz::getReqdSubgroupSizeOpts(F)) {
    Opts.assign(1, *reqd_subgroup_vf);
    return true;
  }
  const auto &DI =
      MAM.getResult<compiler::utils::DeviceInfoAnalysis>(*F.getParent());
  auto max_work_width = DI.max_work_width;

  vecz::VeczPassOptions vecz_options;
  vecz_options.choices.enable(vecz::VectorizationChoices::eDivisionExceptions);

  const char *choices_string = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (choices_string &&
      !vecz_options.choices.parseChoicesString(choices_string)) {
    llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    return false;
  }

  auto local_sizes = compiler::utils::getLocalSizeMetadata(F);

  const uint32_t local_vec_dim = 0;
  const uint32_t local_size = local_sizes ? (*local_sizes)[local_vec_dim] : 0;

  vecz_options.vec_dim_idx = local_vec_dim;
  vecz_options.vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;

  vecz_options.local_size = local_size;

  // Although we can vectorize to much wider than 16, it is often
  // not beneficial to do so. Thus when the vectorization mode is
  // ALWAYS, we cap it at 16 as a compromise to prevent execution
  // times from becoming too long in UnitCL. When the mode is AUTO,
  // it may decide to vectorize narrower than the given width, but
  // never wider.
  const uint32_t work_width =
      (vecz_mode == compiler::VectorizationMode::ALWAYS) ? 16u : max_work_width;

  // The final vector width will be the kernel's dynamic work width,
  // and dynamic work width must not exceed the device's maximum
  // work width, so cap it before we even attempt vectorization.
  // Only try to vectorize to widths of powers of two.
#if LLVM_VERSION_GREATER_EQUAL(16, 0)
  const uint32_t SIMDWidth = llvm::bit_floor(
#else
  const uint32_t SIMDWidth = llvm::PowerOf2Floor(
#endif
      local_size != 0 ? std::min(local_size, work_width) : work_width);

  vecz_options.factor =
      compiler::utils::VectorizationFactor::getFixedWidth(SIMDWidth);

  Opts.push_back(vecz_options);
  return true;
}

void HostPassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();

#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);

#include "host_pass_registry.def"
}

/// @brief Helper functions for parsing options
/// @note These functions are small but keep the def file simpler and is in line
/// with PassBuilder.cpp
llvm::Expected<bool> parseFloatPointControlPassOptions(llvm::StringRef Params) {
  return compiler::utils::parseSinglePassOption(Params, "ftz",
                                                "FloatPointControlPass");
}

void HostPassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "host_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

void HostPassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS)   \
  if (compiler::utils::checkParametrizedPassName(Name, NAME)) {             \
    auto Params = compiler::utils::parsePassParameters(PARSER, Name, NAME); \
    if (!Params) {                                                          \
      llvm::errs() << toString(Params.takeError()) << "\n";                 \
      return false;                                                         \
    }                                                                       \
    PM.addPass(CREATE_PASS(Params.get()));                                  \
    return true;                                                            \
  }

#include "host_pass_registry.def"
        return false;
      });
}

bool HostPassMachinery::handlePipelineElement(llvm::StringRef Name,
                                              llvm::ModulePassManager &PM) {
  if (Name.consume_front("host-late-passes")) {
    PM.addPass(getLateTargetPasses());
    return true;
  }

  if (Name.consume_front("host-kernel-passes")) {
    std::optional<std::string> unique_prefix;
    if (Name.consume_front("<")) {
      if (!Name.consume_back(">")) {
        llvm::errs() << "Invalid 'host-kernel-passes' parameterization\n";
        return false;
      }
      unique_prefix = Name.str();
    }

    PM.addPass(getKernelFinalizationPasses(unique_prefix));
    return true;
  }

  return false;
}

llvm::ModulePassManager HostPassMachinery::getLateTargetPasses() {
  // We may have a situation where there were already opaque structs in the
  // context associated with HostTarget which have the same name as those
  // in the deserialized module. LLVM tries to resolve this name clash by
  // introducing suffixes to the opaque structs in the deserialized module e.g.
  // __mux_dma_event_t becomes __mux_dma_event_t.0. This is a problem since
  // later passes may rely on the name __mux_dma_event_t to identify the type,
  // so here we remap the structs.
  llvm::ModulePassManager PM;
  PM.addPass(compiler::utils::UniqueOpaqueStructsPass());
  PM.addPass(compiler::utils::SimpleCallbackPass([](llvm::Module &m) {
    // the llvm identifier is no longer needed by our host code
    if (auto md = m.getNamedMetadata("llvm.ident")) {
      md->dropAllReferences();
      md->eraseFromParent();
    }
  }));
#ifdef CA_ENABLE_DEBUG_SUPPORT
  PM.addPass(compiler::utils::SimpleCallbackPass([&](llvm::Module &m) {
    // Custom host options are set as module metadata as a testing aid
    auto &ctx = m.getContext();
    if (options.device_args.data()) {
      if (auto md = m.getOrInsertNamedMetadata("host.build_options")) {
        llvm::MDString *md_string =
            llvm::MDString::get(ctx, options.device_args.data());
        md->addOperand(llvm::MDNode::get(ctx, md_string));
      }
    }
  }));
#endif  // CA_ENABLE_DEBUG_SUPPORT
  return PM;
}

llvm::ModulePassManager HostPassMachinery::getKernelFinalizationPasses(
    std::optional<std::string> unique_prefix) {
  llvm::ModulePassManager PM;
  compiler::BasePassPipelineTuner tuner(options);

  // On host we have degenerate sub-groups i.e. sub-group == work-group.
  tuner.degenerate_sub_groups = true;

  // Forcibly compute the BuiltinInfoAnalysis so that cached retrievals work.
  PM.addPass(llvm::RequireAnalysisPass<compiler::utils::BuiltinInfoAnalysis,
                                       llvm::Module>());

// Fix for alignment issues endemic on 32 bit ARM, but can also arise on 32 bit
// X86. We want this pass to run early so it needs to process less instructions
// and to avoid having to deal with the side effects of other passes.
#if defined(UTILS_SYSTEM_32_BIT)
  PM.addPass(compiler::utils::AlignModuleStructsPass());
#endif

  // Handle the generic address space
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass()));

  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  // Verify that any required sub-group size was met.
  PM.addPass(compiler::utils::VerifyReqdSubGroupSizeSatisfiedPass());

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::WorkItemLoopsPassOptions WIOpts;
  WIOpts.IsDebug = options.opt_disable;

  PM.addPass(compiler::utils::WorkItemLoopsPass(WIOpts));

  PM.addPass(compiler::utils::AddSchedulingParametersPass());

  // With scheduling parameters added, add our work-group loops
  PM.addPass(AddEntryHookPass());
  // Define mux builtins now, since AddEntryHookPass introduces more
  PM.addPass(compiler::utils::DefineMuxBuiltinsPass());

  compiler::utils::AddKernelWrapperPassOptions KWOpts;
  KWOpts.IsPackedStruct = true;
  PM.addPass(compiler::utils::AddKernelWrapperPass(KWOpts));

  PM.addPass(compiler::utils::ReplaceLocalModuleScopeVariablesPass());

  PM.addPass(AddFloatingPointControlPass(options.denorms_may_be_zero));

  if (unique_prefix) {
    PM.addPass(llvm::createModuleToFunctionPassAdaptor(
        compiler::utils::MakeFunctionNameUniquePass(*unique_prefix)));
  }

  // Functions with __attribute__ ((always_inline)) should
  // be inlined even at -O0.
  PM.addPass(llvm::AlwaysInlinerPass());

  // Running this pass here is the "nuclear option", it would be better to
  // ensure exception handling is never introduced in the first place, but
  // it is not always plausible to do.
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::RemoveExceptionsPass()));

  addLLVMDefaultPerModulePipeline(PM, PB, options);

  // createDisableNeonAttributePass only does work on 64-bit ARM to fix a Neon
  // correctness issue.
  PM.addPass(DisableNeonAttributePass());

  // Workaround an x86-64 codegen bug in LLVM.
  PM.addPass(RemoveByValAttributesPass());

  if (options.opt_disable) {
    PM.addPass(llvm::createModuleToFunctionPassAdaptor(
        compiler::utils::RemoveLifetimeIntrinsicsPass()));
  }

  // ENORMOUS WARNING:
  // Removing memory fences can result in invalid code or incorrect behaviour in
  // general. This pass is a workaround for backends that do not yet support
  // memory fences.  This is not required for any of the LLVM backends used by
  // host, but the pass is used here to ensure that it is tested.
  // The memory model on OpenCL 1.2 is so underspecified that we can get away
  // with removing fences. In OpenCL 3.0 the memory model is better defined, and
  // just removing fences could result in incorrect behavior for valid 3.0
  // OpenCL applications.
  if (options.standard != compiler::Standard::OpenCLC30) {
    PM.addPass(llvm::createModuleToFunctionPassAdaptor(
        compiler::utils::RemoveFencesPass()));
  }

  PM.addPass(compiler::utils::ComputeLocalMemoryUsagePass());

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  return PM;
}

void HostPassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);
  OS << "\nHost passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "host_pass_registry.def"

  OS << "Module passes with params:\n";
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  compiler::utils::printPassName(NAME, PARAMS, OS);
#include "host_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "host_pass_registry.def"

  OS << "\nHost pipelines:\n\n";

  OS << "  host-late-passes\n";
  OS << "    Runs the pipeline for BaseModule::getLateTargetPasses\n";
  OS << "  host-kernel-passes\n";
  OS << "  host-kernel-passes<unique-name>\n";
  OS << "    Runs the kernel finalization pipeline (usually done online during "
        "jitting or offline during Module::createBinary).\n"
        "    Optionally takes 'unique-name', which schedules "
        "MakeFunctionNameUniquePass with that name.\n";
}

}  // namespace host
