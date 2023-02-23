// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/set_barrier_convergent_pass.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <multi_llvm/llvm_version.h>

static bool IsBarrierName(llvm::Function *callee) {
  if (!callee) {
    return false;
  }
  return
#if defined(CA_COMPILER_ENABLE_CL_VERSION_3_0)
      callee->getName().equals("_Z18work_group_barrierj") ||
      callee->getName().equals("_Z18work_group_barrierjj") ||
#endif
      callee->getName().equals("_Z7barrierj");
}

static bool runOnFunction(llvm::Function &func) {
  bool changed = false;

  // We go over each function, identify ones of type barrier so that we can add
  // the 'convergent' attribute. This attribute is required to be set on the
  // barrier function to stop LLVM optimizers modifying it illegally.  For
  // OpenCL C we already set this attribute directly in the header, but if
  // we're consuming SPIR we need to modify the definitions and calls to
  // respect the attribute.  This must be done before any optimizations can
  // be run.
  if (IsBarrierName(&func)) {
    func.addFnAttr(llvm::Attribute::Convergent);
    return true;
  }

  // Function calls can have different attributes set so we need to also go
  // through all instructions, check if they are a call to barrier and add
  // the 'convergent' attribute.
  for (llvm::BasicBlock &BB : func) {
    for (llvm::Instruction &I : BB) {
      if (llvm::CallInst *call_inst = llvm::dyn_cast<llvm::CallInst>(&I)) {
        if (IsBarrierName(call_inst->getCalledFunction())) {
          call_inst->addFnAttr(llvm::Attribute::Convergent);
          changed = true;
        }
      }
    }
  }

  return changed;
}

llvm::PreservedAnalyses compiler::SetBarrierConvergentPass::run(
    llvm::Module &M, llvm::ModuleAnalysisManager &) {
  auto pa = llvm::PreservedAnalyses::all();
  bool changed = false;

  // This pass needs to operate on function declarations as well as
  // definitions, so this is not actually equivalent to a FunctionPass
  // (despite the appearance).
  for (llvm::Function &F : M) {
    changed |= runOnFunction(F);
  }
  return changed ? llvm::PreservedAnalyses::none()
                 : llvm::PreservedAnalyses::all();
}
