// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/check_for_doubles_pass.h>
#include <compiler/utils/device_info.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

int compiler::DiagnosticInfoDoubleNoDouble::DK_DoubleNoDouble =
    getNextAvailablePluginDiagnosticKind();

StringRef compiler::DiagnosticInfoDoubleNoDouble::formatMessage() const {
  return "A double precision floating point number was generated, "
         "but cl_khr_fp64 is not supported on this target.";
}

void compiler::DiagnosticInfoDoubleNoDouble::print(DiagnosticPrinter &P) const {
  P << formatMessage();
}

namespace {
// Tracks presence of double types in a BasicBlock.
inline bool findDoubleTypes(const BasicBlock &BB) {
  // Each instruction in the BB
  for (const auto &I : BB) {
    // The instruction's return type
    if (I.getType()->isDoubleTy()) {
      return true;
    }
    // Else check each operand of the instruction
    for (const auto &Op : I.operands()) {
      if (Op->getType()->isDoubleTy()) {
        return true;
      }
    }
  }
  return false;
}
}  // namespace

PreservedAnalyses compiler::CheckForDoublesPass::run(
    Function &F, FunctionAnalysisManager &AM) {
  const auto &MAMProxy = AM.getResult<ModuleAnalysisManagerFunctionProxy>(F);
  const auto *DI =
      MAMProxy.getCachedResult<compiler::utils::DeviceInfoAnalysis>(
          *F.getParent());

  // Check if doubles are supported, in which case there's nothing to do.
  if (DI && DI->double_capabilities != 0) {
    return PreservedAnalyses::all();
  }
  for (const auto &BB : F) {
    if (findDoubleTypes(BB)) {
      F.getContext().diagnose(DiagnosticInfoDoubleNoDouble());
      return PreservedAnalyses::all();
    }
  }
  return PreservedAnalyses::all();
}
