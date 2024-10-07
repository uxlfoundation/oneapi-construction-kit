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

#include <base/pass_pipelines.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/manual_type_legalization_pass.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/verify_reqd_sub_group_size_pass.h>
#include <compiler/utils/work_item_loops_pass.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <riscv/ir_to_builtins_pass.h>
#include <riscv/riscv_pass_machinery.h>
#include <vecz/pass.h>

namespace riscv {

RiscvPassMachinery::RiscvPassMachinery(
    const RiscvTarget &target, llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
    const compiler::utils::DeviceInfo &Info,
    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
    bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
    bool timePasses)
    : compiler::BaseModulePassMachinery(Ctx, TM, Info, BICallback, verifyEach,
                                        debugLogLevel, timePasses),
      target(target) {}

// Process various compiler options based off compiler build options and common
// environment variables
RiscvPassMachinery::OptimizationOptions
RiscvPassMachinery::processOptimizationOptions(
    std::optional<std::string> env_debug_prefix,
    std::optional<compiler::VectorizationMode> vecz_mode) {
  OptimizationOptions env_var_opts;
  vecz::VeczPassOptions vecz_opts;
  // The minimum number of elements to vectorize for. For a fixed-length VF,
  // this is the exact number of elements to vectorize by. For scalable VFs,
  // the actual number of elements is a multiple (vscale) of these, unknown at
  // compile time. Default taken from config. May be overriden later.
  vecz_opts.factor = compiler::utils::VectorizationFactor::getScalar();

  vecz_opts.choices.enable(vecz::VectorizationChoices::eDivisionExceptions);

  vecz_opts.vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;
  vecz_opts.vec_dim_idx = 0;

  // This is of the form of a comma separated set of fields
  // S     - use scalable vectorization
  // V     - vectorize only, otherwise produce both scalar and vector kernels
  // A     - let vecz automatically choose the vectorization factor
  // 1-64  - vectorization factor multiplier: the fixed amount itself, or the
  //         value that multiplies the scalable amount
  // VP    - produce a vector-predicated kernel
  // VVP   - produce both a vectorized and a vector-predicated kernel
  bool add_vvp = false;
  if (const auto *vecz_vf_flags_env = std::getenv("CA_RISCV_VF")) {
    // Set scalable to off and let users add it explicitly with 'S'.
    vecz_opts.factor.setIsScalable(false);
    llvm::SmallVector<llvm::StringRef, 4> flags;
    const llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "A" || r == "a") {
        vecz_opts.vecz_auto = true;
      } else if (r == "V" || r == "v") {
        // Note: This is a legacy toggle for forcing vectorization with no
        // scalar tail based on the "VF" environment variable. Ideally we'd be
        // setting it on a per-function basis, and we'd also be setting the
        // vectorization options themselves on a per-function basis. Until we've
        // designed a new method, keep the legacy behaviour by re-parsing the
        // "VF" environment variable and look for a "v/V" toggle.
        env_var_opts.force_no_tail = true;
      } else if (r == "S" || r == "s") {
        vecz_opts.factor.setIsScalable(true);
        env_var_opts.early_link_builtins = true;
      } else if (isdigit(r[0])) {
        vecz_opts.factor.setKnownMin(std::stoi(r.str()));
      } else if (r == "VP" || r == "vp") {
        vecz_opts.choices.enable(
            vecz::VectorizationChoices::eVectorPredication);
      } else if (r == "VVP" || r == "vvp") {
        // Add the vectorized pass option now (controlled by other iterations
        // of this loop), and flag that we have to add a vector-predicated form
        // later.
        add_vvp = true;
      } else {
        // An error - just stop processing the environment variable now.
        break;
      }
    }
  }

  // Choices override the cost model
  const char *ptr = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (ptr) {
    const bool success = vecz_opts.choices.parseChoicesString(ptr);
    if (!success) {
      llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    }
  }

  env_var_opts.vecz_pass_opts.push_back(vecz_opts);
  if (add_vvp) {
    vecz_opts.choices.enable(vecz::VectorizationChoices::eVectorPredication);
    env_var_opts.vecz_pass_opts.push_back(vecz_opts);
  }

  // Allow any decisions made on early linking builtins to be overridden
  // with an env variable
  if (env_debug_prefix) {
    const std::string env_name = *env_debug_prefix + "_EARLY_LINK_BUILTINS";
    if (const char *early_link_builtins_env = getenv(env_name.c_str())) {
      env_var_opts.early_link_builtins = atoi(early_link_builtins_env) != 0;
    }
  }

  return env_var_opts;
}

static bool riscvVeczPassOpts(
    llvm::Function &F, llvm::ModuleAnalysisManager &AM,
    llvm::SmallVectorImpl<vecz::VeczPassOptions> &PassOpts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (!compiler::utils::isKernelEntryPt(F) ||
      F.hasFnAttribute(llvm::Attribute::OptimizeNone) ||
      vecz_mode == compiler::VectorizationMode::NEVER) {
    return false;
  }
  // Handle auto sub-group sizes. If the kernel uses sub-groups or has a
  // required sub-group size, only vectorize to one of those lengths. Let vecz
  // pick.
  if (auto auto_subgroup_vf = vecz::getAutoSubgroupSizeOpts(F, AM)) {
    PassOpts.assign(1, *auto_subgroup_vf);
    return true;
  }
  auto env_var_opts = RiscvPassMachinery::processOptimizationOptions(
      /*env_debug_prefix*/ {}, vecz_mode);
  if (env_var_opts.vecz_pass_opts.empty()) {
    return false;
  }
  PassOpts.assign(env_var_opts.vecz_pass_opts);
  return true;
}

void RiscvPassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);

#include "riscv_pass_registry.def"
}

void RiscvPassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "riscv_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

llvm::ModulePassManager RiscvPassMachinery::getLateTargetPasses() {
  llvm::ModulePassManager PM;

  std::optional<std::string> env_debug_prefix;
#if !defined(NDEBUG) || defined(CA_ENABLE_DEBUG_SUPPORT) || \
    defined(CA_RISCV_DEMO_MODE)
  env_debug_prefix = target.env_debug_prefix;
#endif

  const compiler::BasePassPipelineTuner tuner(options);
  auto env_var_opts =
      processOptimizationOptions(env_debug_prefix, /* vecz_mode*/ {});

  PM.addPass(compiler::utils::TransferKernelMetadataPass());

  if (env_debug_prefix) {
    const std::string dump_ir_env_name = *env_debug_prefix + "_DUMP_IR";
    if (std::getenv(dump_ir_env_name.c_str())) {
      PM.addPass(compiler::utils::SimpleCallbackPass(
          [](llvm::Module &m) { m.print(llvm::dbgs(), /*AAW*/ nullptr); }));
    }
  }

  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceMemIntrinsicsPass()));

  // Forcibly compute the BuiltinInfoAnalysis so that cached retrievals work.
  PM.addPass(llvm::RequireAnalysisPass<compiler::utils::BuiltinInfoAnalysis,
                                       llvm::Module>());

  // Handle the generic address space
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass()));

  PM.addPass(IRToBuiltinReplacementPass());

  if (env_var_opts.early_link_builtins) {
    PM.addPass(compiler::utils::LinkBuiltinsPass());
  }

  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::WorkItemLoopsPassOptions WIOpts;
  WIOpts.IsDebug = options.opt_disable;
  WIOpts.ForceNoTail = env_var_opts.force_no_tail;
  PM.addPass(compiler::utils::WorkItemLoopsPass(WIOpts));

  // Verify that any required sub-group size was met.
  PM.addPass(compiler::utils::VerifyReqdSubGroupSizeSatisfiedPass());

  compiler::addPrepareWorkGroupSchedulingPasses(PM);

  compiler::utils::AddKernelWrapperPassOptions KWOpts;
  // We don't bundle kernel arguments in a packed struct.
  KWOpts.IsPackedStruct = false;
  PM.addPass(compiler::utils::AddKernelWrapperPass(KWOpts));

  PM.addPass(compiler::utils::ReplaceLocalModuleScopeVariablesPass());

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  addLLVMDefaultPerModulePipeline(PM, getPB(), options);

  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ManualTypeLegalizationPass()));

  if (env_debug_prefix) {
    // With all passes scheduled, add a callback pass to view the
    // assembly/object file, if requested.
    const std::string dump_asm_env_name = *env_debug_prefix + "_DUMP_ASM";
    if (std::getenv(dump_asm_env_name.c_str())) {
      PM.addPass(compiler::utils::SimpleCallbackPass([this](llvm::Module &m) {
        // Clone the module so we leave it in the same state after we compile.
        auto cloned_m = llvm::CloneModule(m);
        compiler::emitCodeGenFile(*cloned_m, TM, llvm::outs(),
                                  /*create_assembly*/ true);
      }));
    }
  }

  return PM;
}

void RiscvPassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#include "riscv_pass_registry.def"
        return false;
      });
}

bool RiscvPassMachinery::handlePipelineElement(llvm::StringRef Name,
                                               llvm::ModulePassManager &PM) {
  if (Name.consume_front("riscv-late-passes")) {
    PM.addPass(getLateTargetPasses());
    return true;
  }

  return false;
}

void RiscvPassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);

  OS << "\nriscv specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "riscv_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "riscv_pass_registry.def"

  OS << "\nriscv pipelines:\n\n";

  OS << "  riscv-late-passes\n";
  OS << "    Runs the pipeline for BaseModule::getLateTargetPasses\n";
}

}  // namespace riscv
