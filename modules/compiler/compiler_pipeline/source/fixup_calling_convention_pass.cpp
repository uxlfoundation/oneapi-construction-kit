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

#include <compiler/utils/attributes.h>
#include <compiler/utils/fixup_calling_convention_pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;

PreservedAnalyses compiler::utils::FixupCallingConventionPass::run(
    Module &M, ModuleAnalysisManager &) {
  bool Changed = false;

  for (auto &F : M.functions()) {
    // set the calling convention to system default
    // don't update calling convention for intrinsics
    if (F.isIntrinsic()) {
      continue;
    }

    CallingConv::ID ActualCC;
    if (CC == CallingConv::SPIR_KERNEL || CC == CallingConv::SPIR_FUNC) {
      if (isKernel(F)) {
        ActualCC = CallingConv::SPIR_KERNEL;
      } else {
        ActualCC = CallingConv::SPIR_FUNC;
      }
    } else {
      ActualCC = CC;
    }

    if (F.getCallingConv() != ActualCC) {
      Changed = true;
      // set the calling convention of the function
      F.setCallingConv(ActualCC);
    }

    // go through all users of the function
    for (auto &Use : F.uses()) {
      auto CI = dyn_cast<CallInst>(Use.getUser());

      // if we had a call instruction
      if (CI && CI->getCallingConv() != ActualCC) {
        Changed = true;
        // set the calling convention of the call too
        CI->setCallingConv(ActualCC);
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
