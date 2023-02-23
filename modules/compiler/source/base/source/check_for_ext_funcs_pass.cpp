// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <base/check_for_ext_funcs_pass.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

int compiler::DiagnosticInfoExternalFunc::DK_ExternalFunc =
    getNextAvailablePluginDiagnosticKind();

std::string compiler::DiagnosticInfoExternalFunc::formatMessage() const {
  return std::string("Could not find a definition for external function '") +
         FName + "'";
}

void compiler::DiagnosticInfoExternalFunc::print(DiagnosticPrinter &P) const {
  P << formatMessage();
}

PreservedAnalyses compiler::CheckForExtFuncsPass::run(Module &M,
                                                      ModuleAnalysisManager &) {
  for (const auto &F : M) {
    auto FName = F.getName();
    if (F.isDeclaration() && !F.isIntrinsic() && !FName.equals("printf") &&
        !FName.startswith("_Z") && !FName.startswith("__")) {
      M.getContext().diagnose(DiagnosticInfoExternalFunc(FName));
    }
  }

  return PreservedAnalyses::all();
}
