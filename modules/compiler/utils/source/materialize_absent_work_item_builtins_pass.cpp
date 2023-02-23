// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
