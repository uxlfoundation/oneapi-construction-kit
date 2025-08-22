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

#include <base/check_for_unsupported_types_pass.h>
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

int compiler::DiagnosticInfoHalfNoHalf::DK_HalfNoHalf =
    getNextAvailablePluginDiagnosticKind();

StringRef compiler::DiagnosticInfoHalfNoHalf::formatMessage() const {
  return "A half precision floating point number was generated, "
         "but cl_khr_fp16 is not supported on this target.";
}

void compiler::DiagnosticInfoHalfNoHalf::print(DiagnosticPrinter &P) const {
  P << formatMessage();
}

PreservedAnalyses
compiler::CheckForUnsupportedTypesPass::run(Function &F,
                                            FunctionAnalysisManager &AM) {
  const auto &MAMProxy = AM.getResult<ModuleAnalysisManagerFunctionProxy>(F);
  const auto *DI =
      MAMProxy.getCachedResult<compiler::utils::DeviceInfoAnalysis>(
          *F.getParent());
  // Check if double and half are supported
  bool CheckDouble = !DI || !DI->double_capabilities;
  bool CheckHalf = !DI || !DI->half_capabilities;
  // If we do not have to check for either type, exit right away.
  if (!CheckDouble && !CheckHalf) {
    return PreservedAnalyses::all();
  }
  auto CheckType = [&](llvm::Type *T) {
    if (T->isDoubleTy() && CheckDouble) {
      F.getContext().diagnose(DiagnosticInfoDoubleNoDouble());
      CheckDouble = false;
    }
    if (T->isHalfTy() && CheckHalf) {
      F.getContext().diagnose(DiagnosticInfoHalfNoHalf());
      CheckHalf = false;
    }
  };
  for (const auto &BB : F) {
    for (const auto &I : BB) {
      CheckType(I.getType());
      if (!CheckDouble && !CheckHalf) {
        return PreservedAnalyses::all();
      }
      for (const auto &Op : I.operands()) {
        CheckType(Op->getType());
        if (!CheckDouble && !CheckHalf) {
          return PreservedAnalyses::all();
        }
      }
    }
  }
  return PreservedAnalyses::all();
}
