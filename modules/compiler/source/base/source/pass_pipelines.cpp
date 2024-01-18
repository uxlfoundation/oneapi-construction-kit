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
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/define_mux_dma_pass.h>
#include <compiler/utils/degenerate_sub_group_pass.h>
#include <compiler/utils/fixup_calling_convention_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/optimal_builtin_replacement_pass.h>
#include <compiler/utils/prepare_barriers_pass.h>
#include <compiler/utils/reduce_to_function_pass.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_mux_math_decls_pass.h>
#include <compiler/utils/replace_wgc_pass.h>
#include <compiler/utils/sub_group_usage_pass.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <multi_llvm/multi_llvm.h>

#include <optional>

using namespace llvm;

namespace compiler {

void addPreVeczPasses(ModulePassManager &PM,
                      const BasePassPipelineTuner &tuner) {
  if (!tuner.options.soft_math) {
    PM.addPass(createModuleToPostOrderCGSCCPassAdaptor(
        compiler::utils::OptimalBuiltinReplacementPass()));
  }

  PM.addPass(compiler::utils::SubgroupUsagePass());

  if (tuner.degenerate_sub_groups) {
    PM.addPass(compiler::utils::DegenerateSubGroupPass());
  }

  if (tuner.replace_work_group_collectives) {
    // Because ReplaceWGCPass may introduce barrier calls it needs to be run
    // before PrepareBarriersPass.
    PM.addPass(compiler::utils::ReplaceWGCPass());
  }

  // We have to inline all functions containing barriers before running vecz,
  // because the barriers in both the scalar and vector kernels need to be
  // associated with each other. To do so, the Prepare Barriers Pass also gives
  // each barrier a unique ID in metadata.
  PM.addPass(compiler::utils::PrepareBarriersPass());

  PM.addPass(compiler::utils::ReplaceMuxMathDeclsPass(
      tuner.options.unsafe_math_optimizations));
}

void addLateBuiltinsPasses(ModulePassManager &PM,
                           const BasePassPipelineTuner &tuner) {
  PM.addPass(compiler::utils::LinkBuiltinsPass());

  PM.addPass(compiler::utils::DefineMuxDmaPass());

  PM.addPass(compiler::utils::ReplaceMuxMathDeclsPass(
      tuner.options.unsafe_math_optimizations));

  if (!tuner.options.soft_math) {
    PM.addPass(createModuleToPostOrderCGSCCPassAdaptor(
        compiler::utils::OptimalBuiltinReplacementPass()));
  }

  PM.addPass(compiler::utils::ReduceToFunctionPass());

  // We run an internalizer pass to allow removal of the dead barrier calls.
  // The removal happens when we call the inlining pass before the barrier
  // pass
  PM.addPass(InternalizePass([](const GlobalValue &gv) -> bool {
    return isa<Function>(&gv) &&
           compiler::utils::isKernel(*cast<Function>(&gv));
  }));

  // This pass fixes up the calling convention - typically SPIR_KERNEL or
  // SPIR_FUNCTION - to be the convention we pass to it. Note that it doesn't
  // actually adjust the function return type or parameters, so it only
  // correctly supports calling conventions which are ABI-compatible with the
  // existing IR.
  PM.addPass(
      compiler::utils::FixupCallingConventionPass(tuner.calling_convention));
}

void addPrepareWorkGroupSchedulingPasses(ModulePassManager &PM) {
  PM.addPass(compiler::utils::AddSchedulingParametersPass());
  PM.addPass(compiler::utils::DefineMuxBuiltinsPass());
}

void addLLVMDefaultPerModulePipeline(ModulePassManager &PM, PassBuilder &PB,
                                     const compiler::Options &options) {
  if (!options.opt_disable) {
    PM.addPass(PB.buildPerModuleDefaultPipeline(OptimizationLevel::O3));
  } else {
    PM.addPass(PB.buildO0DefaultPipeline(OptimizationLevel::O0,
                                         /*LTOPreLink*/ false));
    // LLVM's new inliners do less than the legacy ones, so run a round of
    // global optimization to remove any dead functions.
    // FIXME: This isn't just optimization: we have internal functions without
    // bodies that *require* DCE or we may see missing symbols. This should be
    // fixed so that targets are free to skip GlobalOpt if they want to. See
    // CA-4126.
    PM.addPass(GlobalOptPass());
  }
}

Result emitCodeGenFile(llvm::Module &M, TargetMachine *TM,
                       raw_pwrite_stream &ostream, bool create_assembly) {
  legacy::PassManager PM;
  const CodeGenFileType type = !create_assembly
                                   ? multi_llvm::CodeGenFileType::ObjectFile
                                   : multi_llvm::CodeGenFileType::AssemblyFile;
  if (TM->addPassesToEmitFile(PM, ostream, /*DwoOut*/ nullptr, type,
                              /*DisableVerify*/ false)) {
    return compiler::Result::FAILURE;
  }
  PM.run(M);
  return compiler::Result::SUCCESS;
}

void encodeVectorizationMode(Function &F, VectorizationMode mode) {
  switch (mode) {
    case VectorizationMode::AUTO:
      F.addFnAttr("vecz-mode", "auto");
      break;
    case VectorizationMode::ALWAYS:
      F.addFnAttr("vecz-mode", "always");
      break;
    case VectorizationMode::NEVER:
      F.addFnAttr("vecz-mode", "never");
      break;
  }
}

std::optional<VectorizationMode> getVectorizationMode(const Function &F) {
  const Attribute Attr = F.getFnAttribute("vecz-mode");
  if (Attr.isValid()) {
    return StringSwitch<std::optional<VectorizationMode>>(
               Attr.getValueAsString())
        .Case("auto", VectorizationMode::AUTO)
        .Case("always", VectorizationMode::ALWAYS)
        .Case("never", VectorizationMode::NEVER)
        .Default(std::nullopt);
  }
  return std::nullopt;
}

}  // namespace compiler
