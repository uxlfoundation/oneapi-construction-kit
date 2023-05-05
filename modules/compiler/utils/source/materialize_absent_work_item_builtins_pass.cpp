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

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/materialize_absent_work_item_builtins_pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;

static StringMap<compiler::utils::BuiltinID> BuiltinsMap{
    {"_Z20get_global_linear_idv",
     compiler::utils::eMuxBuiltinGetGlobalLinearId},
    {"_Z19get_local_linear_idv", compiler::utils::eMuxBuiltinGetLocalLinearId},
    {"_Z23get_enqueued_local_sizej",
     compiler::utils::eMuxBuiltinGetEnqueuedLocalSize}};

static bool runOnFunction(Function &F, compiler::utils::BuiltinInfo &BI) {
  // Check whether this is actually a builtin we need to implement.
  auto BuiltinNameIt = BuiltinsMap.find(F.getName());
  if (BuiltinNameIt == std::end(BuiltinsMap)) {
    return false;
  }

  // Check it doesn't already have a body.
  if (!F.empty()) {
    return false;
  }

  // Otherwise we are good to go.
  auto *Module = F.getParent();
  auto *const BuiltinFn =
      BI.getOrDeclareMuxBuiltin(BuiltinNameIt->getValue(), *Module);
  BuiltinFn->setCallingConv(F.getCallingConv());
  if (!BuiltinFn->hasFnAttribute(Attribute::NoInline)) {
    BuiltinFn->addFnAttr(Attribute::AlwaysInline);
  }

  SmallVector<Value *, 8> Arguments(make_pointer_range(F.args()));

  IRBuilder<> BBBuilder(BasicBlock::Create(F.getContext(), "entry", &F));
  auto *BuiltinFunctionCall = BBBuilder.CreateCall(BuiltinFn, Arguments, "ret");
  BuiltinFunctionCall->setCallingConv(F.getCallingConv());
  BBBuilder.CreateRet(BuiltinFunctionCall);

  return true;
}

PreservedAnalyses compiler::utils::MaterializeAbsentWorkItemBuiltinsPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);
  for (auto &F : M) {
    Changed |= runOnFunction(F, BI);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
