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
{% if cookiecutter.copyright_name != "" -%}
/// Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
/// Reserved.
{% endif -%}


#include <base/pass_pipelines.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
{% if "replace_mem"  in cookiecutter.feature.split(";") -%}
#include <compiler/utils/replace_mem_intrinsics_pass.h>
{% endif -%}
#include <compiler/utils/simple_callback_pass.h>
#include <compiler/utils/verify_reqd_sub_group_size_pass.h>
#include <compiler/utils/work_item_loops_pass.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <optional>
#include <vecz/pass.h>
// Add our pass machinery
#include <{{cookiecutter.target_name}}/{{cookiecutter.target_name}}_pass_machinery.h>
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
// Add an additional wrapper pass for refsi
#include <{{cookiecutter.target_name}}/refsi_wrapper_pass.h>
{% endif -%}

using namespace llvm;

namespace {{cookiecutter.target_name}} {

{{cookiecutter.target_name.capitalize()}}PassMachinery::{{cookiecutter.target_name.capitalize()}}PassMachinery(
    const {{cookiecutter.target_name.capitalize()}}Target &target, llvm::LLVMContext &Ctx,
    llvm::TargetMachine *TM, const compiler::utils::DeviceInfo &Info,
    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
    bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
    bool timePasses)
    : compiler::BaseModulePassMachinery(Ctx, TM, Info, BICallback, verifyEach,
                                        debugLogLevel, timePasses),
      target(target) {}

struct OptimizationOptions {
  std::optional<vecz::VeczPassOptions> vecz_pass_opts;
  bool force_no_tail = false;
  bool early_link_builtins = false;
};

// Process vecz flags based off build options and environment variables
// return true if we want to vectorize
static OptimizationOptions processOptimizationOptions(
    std::optional<std::string> env_debug_prefix) {
  OptimizationOptions env_var_opts;
  vecz::VeczPassOptions vecz_options;
  // The minimum number of elements to vectorize for. For a fixed-length VF,
  // this is the exact number of elements to vectorize by. For scalable VFs,
  // the actual number of elements is a multiple (vscale) of these, unknown at
  // compile time. Default is scalar, can be updated here.
  vecz_options.factor = compiler::utils::VectorizationFactor::getScalar();

  vecz_options.choices.enable(vecz::VectorizationChoices::eDivisionExceptions);

  // This is of the form of a comma separated set of fields
  // S     - use scalable vectorization
  // V     - vectorize only, otherwise produce both scalar and vector kernels
  // A     - let vecz automatically choose the vectorization factor
  // 1-64  - vectorization factor multiplier: the fixed amount itself, or the
  //         value that multiplies the scalable amount
  // VP    - produce a vector-predicated kernel
  bool found_error = false;
  if (const auto *vecz_vf_flags_env = std::getenv("CA_{{cookiecutter.target_name_capitals}}_VF")) {
    // Set scalable to off and let users add it explicitly with 'S'.
    vecz_options.factor.setIsScalable(false);
    llvm::SmallVector<llvm::StringRef, 4> flags;
    llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "A" || r == "a") {
        vecz_options.vecz_auto = true;
      } else if (r == "V" || r == "v") {
        // Note: This is a legacy toggle for forcing vectorization with no
        // scalar tail based on the "VF" environment variable. Ideally we'd be
        // setting it on a per-function basis, and we'd also be setting the
        // vectorization options themselves on a per-function basis. Until we've
        // designed a new method, keep the legacy behaviour by re-parsing the
        // "VF" environment variable and look for a "v/V" toggle.
        env_var_opts.force_no_tail = true;
      } else if (r == "S" || r == "s") {
        vecz_options.factor.setIsScalable(true);
        env_var_opts.early_link_builtins = true;
      } else if (isdigit(r[0])) {
        vecz_options.factor.setKnownMin(std::stoi(r.str()));
      } else if (r == "VP" || r == "vp") {
        vecz_options.choices.enable(
            vecz::VectorizationChoices::eVectorPredication);
      } else {
        found_error = true;
        break;
      }
    }
  }

  // Choices override the cost model
  const char *ptr = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (ptr) {
    bool success = vecz_options.choices.parseChoicesString(ptr);
    if (!success) {
      llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    }
  }

  // Allow any decisions made on early linking builtins to be overridden
  // with an env variable
  if (env_debug_prefix) {
    std::string env_name = *env_debug_prefix + "_EARLY_LINK_BUILTINS";
    if (const char *early_link_builtins_env = getenv(env_name.c_str())) {
      env_var_opts.early_link_builtins = atoi(early_link_builtins_env) != 0;
    }
  }

  if (!found_error) {
    env_var_opts.vecz_pass_opts = std::move(vecz_options);
  }

  return env_var_opts;
}

llvm::ModulePassManager {{cookiecutter.target_name.capitalize()}}PassMachinery::getLateTargetPasses() {
  llvm::ModulePassManager PM;

  std::optional<std::string> env_debug_prefix;
#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  env_debug_prefix = target.env_debug_prefix;
#endif

  compiler::BasePassPipelineTuner tuner(options);
  auto env_var_opts = processOptimizationOptions(env_debug_prefix);

  PM.addPass(compiler::utils::TransferKernelMetadataPass());

  if (env_debug_prefix) {
    std::string dump_ir_env_name = *env_debug_prefix + "_DUMP_IR";
    if (std::getenv(dump_ir_env_name.c_str())) {
      PM.addPass(compiler::utils::SimpleCallbackPass(
          [](llvm::Module &m) { m.print(llvm::dbgs(), /*AAW*/ nullptr); }));
    }
  }

  {% if "replace_mem"  in cookiecutter.feature.split(";") -%}
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceMemIntrinsicsPass()));
  {% endif -%} 

  // Forcibly compute the BuiltinInfoAnalysis so that cached retrievals work.
  PM.addPass(llvm::RequireAnalysisPass<compiler::utils::BuiltinInfoAnalysis,
                                       llvm::Module>());

  // This potentially fixes up any structs to match the spir alignment
  // before we change to the backend layout
  PM.addPass(compiler::utils::AlignModuleStructsPass());

  // Add builtin replacement passes here directly to PM if needed

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

  // Add final passes here by adding directly to PM as needed
{% if "refsi_wrapper_pass"  in cookiecutter.feature.split(";") -%}
  // Add an additional wrapper pass for refsi
  PM.addPass({{cookiecutter.target_name}}::RefSiM1WrapperPass());
{% endif -%}

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  addLLVMDefaultPerModulePipeline(PM, getPB(), options);

  if (env_debug_prefix) {
    // With all passes scheduled, add a callback pass to view the
    // assembly/object file, if requested.
    std::string dump_asm_env_name = *env_debug_prefix + "_DUMP_ASM";
    if (std::getenv(dump_asm_env_name.c_str())) {
      PM.addPass(compiler::utils::SimpleCallbackPass([this](llvm::Module &m) {
        // Clone the module so we leave it in the same state after we
        // compile.
        auto cloned_m = llvm::CloneModule(m);
        compiler::emitCodeGenFile(*cloned_m, TM, llvm::outs(),
                                  /*create_assembly*/ true);
      }));
    }
  }

  return PM;
}

bool {{cookiecutter.target_name.capitalize()}}VeczPassOpts(
         llvm::Function &F, llvm::ModuleAnalysisManager &,
         llvm::SmallVectorImpl<vecz::VeczPassOptions> &PassOpts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (!compiler::utils::isKernelEntryPt(F) ||
      F.hasFnAttribute(llvm::Attribute::OptimizeNone) ||
      vecz_mode == compiler::VectorizationMode::NEVER) {
    return false;
  }
  // Handle required sub-group sizes
  if (auto reqd_subgroup_vf = vecz::getReqdSubgroupSizeOpts(F)) {
    PassOpts.assign(1, *reqd_subgroup_vf);
    return true;
  }
  auto env_var_opts = processOptimizationOptions(/*env_debug_prefix*/ {});
  if (!env_var_opts.vecz_pass_opts.has_value()) {
    return false;
  }
  env_var_opts.vecz_pass_opts->vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;
  env_var_opts.vecz_pass_opts->vec_dim_idx = 0;
  PassOpts.push_back(*env_var_opts.vecz_pass_opts);
  return true;
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_PASS_NO_PARSE(NAME, CLASS) PIC.addClassToPassName(CLASS, NAME);
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  PIC.addClassToPassName(CLASS, NAME);
#define CGSCC_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#include "{{cookiecutter.target_name}}_pass_registry.def"
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "{{cookiecutter.target_name}}_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

bool {{cookiecutter.target_name.capitalize()}}PassMachinery::handlePipelineElement(
    llvm::StringRef Name, llvm::ModulePassManager &PM) {
  if (Name.consume_front("{{cookiecutter.target_name}}-late-passes")) {
    PM.addPass(getLateTargetPasses());
    return true;
  }

  return false;
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
        (void) Name;
        (void) PM;
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }

#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  if (utils::checkParametrizedPassName(Name, NAME)) {                     \
    auto Params = utils::parsePassParameters(PARSER, Name, NAME);         \
    if (!Params) {                                                        \
      errs() << toString(Params.takeError()) << "\n";                     \
      return false;                                                       \
    }                                                                     \
    PM.addPass(CREATE_PASS(Params.get()));                                \
    return true;                                                          \
  }

#define MODULE_ANALYSIS(NAME, CREATE_PASS)                             \
  if (Name == "require<" NAME ">") {                                   \
    PM.addPass(RequireAnalysisPass<                                    \
               std::remove_reference<decltype(CREATE_PASS)>::type,     \
               llvm::Module>());                                       \
    return true;                                                       \
  }                                                                    \
  if (Name == "invalidate<" NAME ">") {                                \
    PM.addPass(InvalidateAnalysisPass<                                 \
               std::remove_reference<decltype(CREATE_PASS)>::type>()); \
    return true;                                                       \
  }

#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                \
  if (Name == "require<" NAME ">") {                                        \
    PM.addPass(createModuleToFunctionPassAdaptor(                           \
        RequireAnalysisPass<std::remove_reference_t<decltype(CREATE_PASS)>, \
                            Function>()));                                  \
    return true;                                                            \
  }                                                                         \
  if (Name == "invalidate<" NAME ">") {                                     \
    PM.addPass(createModuleToFunctionPassAdaptor(                           \
        InvalidateAnalysisPass<                                             \
            std::remove_reference_t<decltype(CREATE_PASS)>>()));            \
    return true;                                                            \
  }

#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS)   \
  if (utils::checkParametrizedPassName(Name, NAME)) {                         \
    auto Params = utils::parsePassParameters(PARSER, Name, NAME);             \
    if (!Params) {                                                            \
      errs() << toString(Params.takeError()) << "\n";                         \
      return false;                                                           \
    }                                                                         \
    PM.addPass(createModuleToFunctionPassAdaptor(CREATE_PASS(Params.get()))); \
    return true;                                                              \
  }

#define FUNCTION_PASS(NAME, CREATE_PASS)                        \
  if (Name == NAME) {                                           \
    PM.addPass(createModuleToFunctionPassAdaptor(CREATE_PASS)); \
    return true;                                                \
  }

#define CGSCC_PASS(NAME, CREATE_PASS)                                 \
  if (Name == NAME) {                                                 \
    PM.addPass(createModuleToPostOrderCGSCCPassAdaptor(CREATE_PASS)); \
    return true;                                                      \
  }
#include "{{cookiecutter.target_name}}_pass_registry.def"
        return false;
      });
}

void {{cookiecutter.target_name.capitalize()}}PassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);

  OS << "\n{{cookiecutter.target_name}} specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"
  OS << "Module passes with params:\n";
#define MODULE_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  utils::printPassName(NAME, PARAMS, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"
  OS << "Function analyses:\n";
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Function passes:\n";
#define FUNCTION_PASS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "Function passes with params:\n";
#define FUNCTION_PASS_WITH_PARAMS(NAME, CLASS, CREATE_PASS, PARSER, PARAMS) \
  utils::printPassName(NAME, PARAMS, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "CGSCC passes:\n";
#define CGSCC_PASS(NAME, CREATE_PASS) utils::printPassName(NAME, OS);
#include "{{cookiecutter.target_name}}_pass_registry.def"

  OS << "\n{{cookiecutter.target_name}} pipelines:\n\n";

  OS << "  {{cookiecutter.target_name}}-late-passes\n";
  OS << "    Runs the pipeline for BaseModule::getLateTargetPasses\n";
}

} // namespace {{cookiecutter.target_name}}
