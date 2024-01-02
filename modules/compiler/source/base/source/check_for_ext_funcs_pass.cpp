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
        !FName.starts_with("_Z") && !FName.starts_with("__")) {
      M.getContext().diagnose(DiagnosticInfoExternalFunc(FName));
    }
  }

  return PreservedAnalyses::all();
}
