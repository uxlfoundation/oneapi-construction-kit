// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/pass_pipelines.h>
#include <cargo/small_vector.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_metadata_pass.h>
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/compute_local_memory_usage_pass.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/handle_barriers_pass.h>
#include <compiler/utils/make_function_name_unique_pass.h>
#include <compiler/utils/metadata_analysis.h>
#include <compiler/utils/metadata_hooks.h>
#include <compiler/utils/remove_exceptions_pass.h>
#include <compiler/utils/remove_fences_pass.h>
#include <compiler/utils/remove_lifetime_intrinsics_pass.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_local_module_scope_variables_pass.h>
#include <compiler/utils/replace_mux_dma_pass.h>
#include <host/add_entry_hook_pass.h>
#include <host/add_floating_point_control_pass.h>
#include <host/disable_neon_attribute_pass.h>
#include <host/info.h>
#include <host/module.h>
#include <host/passes.h>
#include <host/remove_byval_attributes_pass.h>
#include <host/target.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <utils/system.h>
#include <vecz/pass.h>

#include <algorithm>

namespace host {

cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result> emitBinary(
    llvm::Module *module, llvm::TargetMachine *target_machine) {
  llvm::SmallVector<char, 1024> object_code_buffer;
  llvm::raw_svector_ostream stream(object_code_buffer);

  auto result = compiler::emitCodeGenFile(*module, target_machine, stream);
  if (result != compiler::Result::SUCCESS) {
    return cargo::make_unexpected(result);
  }

  cargo::dynamic_array<uint8_t> binary;
  if (binary.alloc(object_code_buffer.size())) {
    return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
  }

  std::copy(object_code_buffer.begin(), object_code_buffer.end(),
            binary.begin());

  return {std::move(binary)};
}

llvm::ModulePassManager hostGetKernelPasses(
    compiler::Options options, llvm::PassBuilder &PB,
    cargo::array_view<compiler::BaseModule::SnapshotDetails> snapshots,
    cargo::optional<llvm::StringRef> unique_prefix) {
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
#if defined(CA_COMPILER_ENABLE_CL_VERSION_3_0)
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass()));
#endif

  addPreVeczPasses(PM, tuner);

  PM.addPass(vecz::RunVeczPass());

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "host", HOST_SNAPSHOT_VECTORIZED, snapshots);

  addLateBuiltinsPasses(PM, tuner);

  compiler::utils::HandleBarriersOptions HBOpts;
  // Barriers with opt disabled are broken on 32 bit for some llvm versions, see
  // CA-3952.
#if !defined(UTILS_SYSTEM_32_BIT)
  HBOpts.IsDebug = options.opt_disable;
#endif

  PM.addPass(compiler::utils::HandleBarriersPass(HBOpts));

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "host", HOST_SNAPSHOT_BARRIER, snapshots);

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
#if defined(CA_COMPILER_ENABLE_CL_VERSION_3_0)
  if (options.standard != compiler::Standard::OpenCLC30) {
    PM.addPass(llvm::createModuleToFunctionPassAdaptor(
        compiler::utils::RemoveFencesPass()));
  }
#else
  PM.addPass(llvm::createModuleToFunctionPassAdaptor(
      compiler::utils::RemoveFencesPass()));
#endif

  PM.addPass(compiler::utils::ComputeLocalMemoryUsagePass());

  PM.addPass(compiler::utils::AddMetadataPass<
             compiler::utils::VectorizeMetadataAnalysis,
             handler::VectorizeInfoMetadataHandler>());

  compiler::BaseModule::addSnapshotPassIfEnabled(
      PM, "host", HOST_SNAPSHOT_SCHEDULED, snapshots);

  return PM;
}
}  // namespace host
