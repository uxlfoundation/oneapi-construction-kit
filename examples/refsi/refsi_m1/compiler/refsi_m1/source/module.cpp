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
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/handle_barriers_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/replace_mem_intrinsics_pass.h>
#include <compiler/utils/simple_callback_pass.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Target/TargetMachine.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <refsi_m1/module.h>
#include <refsi_m1/refsi_mux_builtin_info.h>
#include <refsi_m1/refsi_pass_machinery.h>
#include <refsi_m1/refsi_wrapper_pass.h>
#include <refsi_m1/target.h>
#include <riscv/ir_to_builtins_pass.h>
#include <vecz/pass.h>

namespace refsi_m1 {
RefSiM1Module::RefSiM1Module(RefSiM1Target &target,
                             compiler::BaseContext &context,
                             uint32_t &num_errors, std::string &log)
    : riscv::RiscvModule(target, context, num_errors, log) {}

std::unique_ptr<compiler::utils::PassMachinery>
RefSiM1Module::createPassMachinery() {
  auto *TM = getTargetMachine();
  auto *Builtins = getTarget().getBuiltins();
  const auto &BaseContext = getTarget().getContext();

  compiler::utils::DeviceInfo Info = compiler::initDeviceInfoFromMux(
      getTarget().getCompilerInfo()->device_info);

  auto Callback = [Builtins](const llvm::Module &) {
    return compiler::utils::BuiltinInfo(
        std::make_unique<RefSiM1BIMuxInfo>(),
        compiler::utils::createCLBuiltinInfo(Builtins));
  };
  llvm::LLVMContext &Ctx = Builtins->getContext();
  return std::make_unique<RefSiM1PassMachinery>(
      Ctx, TM, Info, Callback, BaseContext.isLLVMVerifyEachEnabled(),
      BaseContext.getLLVMDebugLoggingLevel(),
      BaseContext.isLLVMTimePassesEnabled());
}

void RefSiM1Module::addFinalKernelPasses(llvm::ModulePassManager &PM) {
  cargo::string_view hal_name(getTarget().riscv_hal_device_info->target_name);
  if (hal_name.ends_with("Tutorial")) {
    PM.addPass(refsi_m1::RefSiM1WrapperPass());
  }
}

llvm::ModulePassManager RefSiM1Module::getLateTargetPasses(
    compiler::utils::PassMachinery &pass_mach) {
  if (getOptions().llvm_stats) {
    llvm::EnableStatistics();
  }

  const auto &env_debug_prefix = getTarget().env_debug_prefix;

  compiler::BasePassPipelineTuner tuner(options);

  cargo::string_view hal_name(getTarget().riscv_hal_device_info->target_name);

  llvm::ModulePassManager PM;

  PM.addPass(compiler::utils::TransferKernelMetadataPass());

#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_REFSI_M1_DEMO_MODE)
  std::string dump_ir_env_name = env_debug_prefix + "_DUMP_IR";
  if (!env_debug_prefix.empty() && std::getenv(dump_ir_env_name.c_str())) {
    PM.addPass(compiler::utils::SimpleCallbackPass(
        [](llvm::Module &m) { m.print(llvm::dbgs(), /*AAW*/ nullptr); }));
  }
#endif

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

  if (isEarlyBuiltinLinkingEnabled(env_debug_prefix)) {
    PM.addPass(compiler::utils::LinkBuiltinsPass(/*EarlyLinking*/ true));
  }

  // TODON'T temporary fix to get subgroup tests passing while we refactor
  // subgroup support. CA-4712 CA-4679
  tuner.degenerate_sub_groups = true;
  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::HandleBarriersOptions HBOpts;
  HBOpts.IsDebug = options.opt_disable;
  HBOpts.ForceNoTail = hasForceNoTail(env_debug_prefix);
  PM.addPass(compiler::utils::HandleBarriersPass(HBOpts));

  compiler::addPrepareWorkGroupSchedulingPasses(PM);

  compiler::utils::AddKernelWrapperPassOptions KWOpts;
  // We don't bundle kernel arguments in a packed struct.
  KWOpts.IsPackedStruct = false;
  PM.addPass(compiler::utils::AddKernelWrapperPass(KWOpts));

  PM.addPass(compiler::utils::ReplaceLocalModuleScopeVariablesPass());

  // RefSi M1 specific kernel passes
  addFinalKernelPasses(PM);

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  addLLVMDefaultPerModulePipeline(PM, pass_mach.getPB(), options);

#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_REFSI_M1_DEMO_MODE)
  // With all passes scheduled, add a callback pass to view the assembly/object
  // file, if requested.
  std::string dump_asm_env_name = env_debug_prefix + "_DUMP_ASM";
  if (!env_debug_prefix.empty() && std::getenv(dump_asm_env_name.c_str())) {
    PM.addPass(compiler::utils::SimpleCallbackPass(
        [TM = pass_mach.getTM()](llvm::Module &m) {
          // Clone the module so we leave it in the same state after we compile.
          auto cloned_m = llvm::CloneModule(m);
          compiler::emitCodeGenFile(*cloned_m, TM, llvm::outs(),
                                    /*create_assembly*/ true);
        }));
  }
#endif

  return PM;
}

}  // namespace refsi_m1
