// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/define_mux_dma_pass.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/manual_type_legalization_pass.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <refsi_g1_wi/refsi_pass_machinery.h>
#include <refsi_g1_wi/refsi_wg_loop_pass.h>
#include <riscv/ir_to_builtins_pass.h>
#include <vecz/pass.h>

namespace refsi_g1_wi {

RefSiG1PassMachinery::RefSiG1PassMachinery(
    const riscv::RiscvTarget &target, llvm::LLVMContext &Ctx,
    llvm::TargetMachine *TM, const compiler::utils::DeviceInfo &Info,
    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
    bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
    bool timePasses)
    : riscv::RiscvPassMachinery(target, Ctx, TM, Info, BICallback, verifyEach,
                                debugLogLevel, timePasses) {}

void RefSiG1PassMachinery::addClassToPassNames() {
  RiscvPassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#include "refsi_pass_registry.def"
}

void RefSiG1PassMachinery::registerPassCallbacks() {
  RiscvPassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#include "refsi_pass_registry.def"
        return false;
      });
}

bool RefSiG1PassMachinery::handlePipelineElement(llvm::StringRef Name,
                                                 llvm::ModulePassManager &PM) {
  if (Name.consume_front("refsi-g1-wi-late-passes")) {
    PM.addPass(getLateTargetPasses());
    return true;
  }

  return false;
}

llvm::ModulePassManager RefSiG1PassMachinery::getLateTargetPasses() {
  llvm::ModulePassManager PM;

  std::optional<std::string> env_debug_prefix;
#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_REFSI_G1_WI_DEMO_MODE)
  env_debug_prefix = target.env_debug_prefix;
#endif

  compiler::BasePassPipelineTuner tuner(options);
  auto env_var_opts =
      processOptimizationOptions(env_debug_prefix, /* vecz_mode*/ {});

  // We don't run the WorkItemLoopsPass; we need an implementation of
  // work-group collective operations.
  tuner.replace_work_group_collectives = true;

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

  // This potentially fixes up any structs to match the spir alignment
  // before we change to the backend layout
  PM.addPass(compiler::utils::AlignModuleStructsPass());

  // Handle the generic address space
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass()));

  PM.addPass(riscv::IRToBuiltinReplacementPass());

  if (env_var_opts.early_link_builtins) {
    PM.addPass(compiler::utils::LinkBuiltinsPass());
  }

  // Bit nasty, but we must schedule a run of the DefineMuxDmaPass to define
  // the __mux_dma_wait builtin - which defers to a work-group barrier - before
  // we run the PrepareBarriersPass (in addPreVeczPasses).
  // We end up running the DefineMuxDmaPass once again in
  // addLateBuiltinsPasses, which isn't ideal.
  PM.addPass(compiler::utils::DefineMuxDmaPass());

  addPreVeczPasses(PM, tuner);

  addLateBuiltinsPasses(PM, tuner);

  PM.addPass(compiler::utils::AddSchedulingParametersPass());

  PM.addPass(RefSiWGLoopPass());

  PM.addPass(compiler::utils::DefineMuxBuiltinsPass());

  compiler::utils::AddKernelWrapperPassOptions KWOpts;
  // We don't bundle kernel arguments in a packed struct.
  KWOpts.IsPackedStruct = false;
  KWOpts.PassLocalBuffersBySize = false;
  PM.addPass(compiler::utils::AddKernelWrapperPass(KWOpts));

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

void RefSiG1PassMachinery::printPassNames(llvm::raw_ostream &OS) {
  riscv::RiscvPassMachinery::printPassNames(OS);

  OS << "\nriscv specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "refsi_pass_registry.def"

  OS << "\nriscv pipelines:\n\n";

  OS << "  refsi-g1-wi-late-passes\n";
  OS << "    Runs the pipeline for BaseModule::getLateTargetPasses\n";
}

}  // namespace refsi_g1_wi
