// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/pass_pipelines.h>
#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/define_mux_builtins_pass.h>
#include <compiler/utils/define_mux_dma_pass.h>
#include <compiler/utils/degenerate_sub_group_pass.h>
#include <compiler/utils/fixup_calling_convention_pass.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/materialize_absent_work_item_builtins_pass.h>
#include <compiler/utils/optimal_builtin_replacement_pass.h>
#include <compiler/utils/prepare_barriers_pass.h>
#include <compiler/utils/reduce_to_function_pass.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <compiler/utils/replace_mux_math_decls_pass.h>
#include <compiler/utils/replace_wgc_pass.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/Internalize.h>

using namespace llvm;

namespace compiler {

void addPreVeczPasses(ModulePassManager &PM,
                      const BasePassPipelineTuner &tuner) {
  if (!tuner.options.soft_math) {
    PM.addPass(createModuleToPostOrderCGSCCPassAdaptor(
        compiler::utils::OptimalBuiltinReplacementPass()));
  }

  if (tuner.degenerate_sub_groups) {
    PM.addPass(compiler::utils::DegenerateSubGroupPass());
  }

  // We need to use the software implementation of the work-group collective
  // builtins. Because ReplaceWGCPass may introduce barrier calls it needs to be
  // run before PrepareBarriersPass.
#if defined(CA_COMPILER_ENABLE_CL_VERSION_3_0)
  PM.addPass(compiler::utils::ReplaceWGCPass());
#endif

  // We have to inline all functions containing barriers before running vecz,
  // because the barriers in both the scalar and vector kernels need to be
  // associated with each other. To do so, the Prepare Barriers Pass also gives
  // each barrier a unique ID in metadata.
  PM.addPass(compiler::utils::PrepareBarriersPass());
}

void addLateBuiltinsPasses(ModulePassManager &PM,
                           const BasePassPipelineTuner &tuner) {
  PM.addPass(compiler::utils::LinkBuiltinsPass(/*EarlyLinking*/ false));

  PM.addPass(compiler::utils::MaterializeAbsentWorkItemBuiltinsPass());

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
  CodeGenFileType type =
      !create_assembly ? llvm::CGFT_ObjectFile : llvm::CGFT_AssemblyFile;
  if (TM->addPassesToEmitFile(PM, ostream, /*DwoOut*/ nullptr, type,
                              /*DisableVerify*/ false)) {
    return compiler::Result::FAILURE;
  }
  PM.run(M);
  return compiler::Result::SUCCESS;
}

void encodeVectorizationMode(llvm::Function &F, VectorizationMode mode) {
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

llvm::Optional<VectorizationMode> getVectorizationMode(
    const llvm::Function &F) {
  llvm::Attribute Attr = F.getFnAttribute("vecz-mode");
  if (Attr.isValid()) {
    return StringSwitch<Optional<VectorizationMode>>(Attr.getValueAsString())
        .Case("auto", VectorizationMode::AUTO)
        .Case("always", VectorizationMode::ALWAYS)
        .Case("never", VectorizationMode::NEVER)
        .Default(llvm::None);
  }
  return None;
}

}  // namespace compiler
