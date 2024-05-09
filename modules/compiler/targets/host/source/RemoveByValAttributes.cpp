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

#include <host/remove_byval_attributes_pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/vector_type_helper.h>

using namespace llvm;

PreservedAnalyses host::RemoveByValAttributesPass::run(
    Module &M, ModuleAnalysisManager &) {
  // This pass is a workaround for an issue specific to 64-bit X86 ABI
  // lowering: bug https://github.com/llvm/llvm-project/issues/34300.
  if (Triple(M.getTargetTriple()).getArch() != Triple::x86_64) {
    return PreservedAnalyses::all();
  }

  for (auto &F : M) {
    // While the bug only manifests on the fifth register parameter onwards,
    // replace functions with *any* byval parameters, in case earlier
    // parameters are split into multiple registers during calling convention
    // lowering.
    if (none_of(F.args(), [](const Argument &A) { return A.hasByValAttr(); })) {
      continue;
    }

    for (auto &A : F.args()) {
      A.removeAttr(Attribute::AttrKind::ByVal);
    }

    for (auto *U : F.users()) {
      if (auto *CI = dyn_cast<CallInst>(U)) {
        for (auto &P : CI->args()) {
          CI->removeParamAttr(P.getOperandNo(), Attribute::AttrKind::ByVal);
        }
      }
    }
  }

  return PreservedAnalyses::all();
}
