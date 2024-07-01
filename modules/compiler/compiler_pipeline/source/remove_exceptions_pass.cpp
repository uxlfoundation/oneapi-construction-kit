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

#include <compiler/utils/remove_exceptions_pass.h>
#include <llvm/IR/Function.h>

using namespace llvm;

PreservedAnalyses compiler::utils::RemoveExceptionsPass::run(
    Function &F, FunctionAnalysisManager &) {
  // We don't use exceptions
  // Adding this attribute here is the "nuclear option", it would be better
  // to ensure it is added at source but it is not always plausible to do.
  if (!F.hasFnAttribute(llvm::Attribute::NoUnwind)) {
    F.addFnAttr(llvm::Attribute::NoUnwind);
  }

  return PreservedAnalyses::all();
}
