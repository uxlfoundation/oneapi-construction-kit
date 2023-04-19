// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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

#if defined(CA_ENABLE_DEBUG_SUPPORT) || defined(CA_REFSI_M1_DEMO_MODE)
  if (!env_debug_prefix.empty()) {
    std::string env_name_dump_ir = env_debug_prefix + "_DUMP_IR";
    if (const char *dumpIR = getenv(env_name_dump_ir.c_str())) {
      addIRSnapshotStages(dumpIR);
    }
    std::string env_name_dump_asm = env_debug_prefix + "_DUMP_ASM";
    if (getenv(env_name_dump_asm.c_str())) {
      std::string name = compiler::BaseModule::getTargetSnapshotName(
          "riscv", riscv::RISCV_SNAPSHOT_BACKEND);
      addInternalSnapshot(name.c_str());
    }
  }
#endif

  compiler::BasePassPipelineTuner tuner(options);

  cargo::string_view hal_name(getTarget().riscv_hal_device_info->target_name);

  llvm::ModulePassManager PM;

  PM.addPass(compiler::utils::TransferKernelMetadataPass());

  // Reuse riscv snapshot names
  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "riscv", riscv::RISCV_SNAPSHOT_INPUT, snapshots);

  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceMemIntrinsicsPass()));

  // Forcibly compute the BuiltinInfoAnalysis so that cached retrievals work.
  PM.addPass(llvm::RequireAnalysisPass<compiler::utils::BuiltinInfoAnalysis,
                                       llvm::Module>());

  // This potentially fixes up any structs to match the spir alignment
  // before we change to the backend layout
  PM.addPass(compiler::utils::AlignModuleStructsPass());

  // Handle the generic address space
#if defined(CA_COMPILER_ENABLE_CL_VERSION_3_0)
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass()));
#endif

  PM.addPass(riscv::IRToBuiltinReplacementPass());

  if (isEarlyBuiltinLinkingEnabled(env_debug_prefix)) {
    PM.addPass(compiler::utils::LinkBuiltinsPass(/*EarlyLinking*/ true));
  }

  // TODON'T temporary fix to get subgroup tests passing while we refactor
  // subgroup support. CA-4712 CA-4679
  tuner.degenerate_sub_groups = true;
  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "riscv", riscv::RISCV_SNAPSHOT_VECTORIZED, snapshots);

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::HandleBarriersOptions HBOpts;
  HBOpts.IsDebug = options.opt_disable;
  HBOpts.ForceNoTail = hasForceNoTail(env_debug_prefix);
  PM.addPass(compiler::utils::HandleBarriersPass(HBOpts));

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "riscv", riscv::RISCV_SNAPSHOT_BARRIER, snapshots);

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

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "riscv", riscv::RISCV_SNAPSHOT_SCHEDULED, snapshots);

  // With all passes scheduled, add a snapshot to view the assembly/object
  // file, if requested.
  if (auto snapshot = compiler::BaseModule::shouldTakeTargetSnapshot(
          "riscv", riscv::RISCV_SNAPSHOT_BACKEND, snapshots)) {
    PM.addPass(compiler::utils::SimpleCallbackPass(
        [snapshot, TM = pass_mach.getTM(), this](llvm::Module &m) {
          takeBackendSnapshot(&m, TM, *snapshot);
        }));
  }

  return PM;
}

}  // namespace refsi_m1
