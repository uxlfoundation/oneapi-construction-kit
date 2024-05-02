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

#include <compiler/utils/metadata.h>
#include <compiler/utils/replace_address_space_qualifier_functions_pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

namespace {

Value *replaceAddressSpaceQualifierFunction(CallBase &call, StringRef name) {
  if (name != "__to_global" && name != "__to_local" && name != "__to_private") {
    return nullptr;
  }

  IRBuilder<> B(&call);
  Value *ptr = *call.arg_begin();

  return B.CreatePointerBitCastOrAddrSpaceCast(ptr, call.getType(), name);
}

}  // namespace

PreservedAnalyses
compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass::run(
    Function &F, FunctionAnalysisManager &) {
  // Only run this pass on modules compatible with OpenCL 2.0 and above.
  // FIXME: These should be left up to the target to implement, like mux
  // builtins.
  auto Version = getOpenCLVersion(*F.getParent());
  if (Version < OpenCLC20) {
    return PreservedAnalyses::all();
  }
  bool Changed = false;

  for (BasicBlock &BB : F) {
    for (auto instI = BB.begin(); instI != BB.end();) {
      auto &inst = *instI++;
      if (!isa<CallInst>(inst)) {
        continue;
      }

      auto &call = cast<CallInst>(inst);
      Function *const callee = call.getCalledFunction();
      const auto name = callee->getName();
      if (auto *const ASQ = replaceAddressSpaceQualifierFunction(call, name)) {
        call.replaceAllUsesWith(ASQ);
        call.eraseFromParent();
        Changed = true;
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
