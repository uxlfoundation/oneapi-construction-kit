// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replaces calls to sub-group builtins with their analagous work-group
/// builtin.
///
/// @copyright
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/degenerate_sub_group_pass.h>
#include <compiler/utils/group_collective_helpers.h>
#include <compiler/utils/mangling.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>

#include <map>
#include <set>

using namespace llvm;

namespace {
/// @brief Helper for determining if a call instruction calls a sub-group
/// builtin function.
///
/// @param[in] CI Call instruction to query.
///
/// @return True if CI is a call to sub-group builtin, false otherwise.
bool isSubGroupFunction(CallInst *CI) {
  auto *Fcn = CI->getCalledFunction();
  assert(Fcn && "virtual calls are not supported");
  if (auto GC = compiler::utils::isGroupCollective(Fcn)) {
    return GC->scope == compiler::utils::GroupCollective::Scope::SubGroup;
  }

  return Fcn->getName() == compiler::utils::MuxBuiltins::sub_group_barrier;
}

/// @brief Helper for building the symbol name of the mangled work-group builtin
/// corresponding to the sub-group builtin.
///
/// @param[in] SubgroupBuiltin sub-group builtin to map to a work-group builtin.
/// @param[in] Ctx Context used to create llvm objects against.
/// @param[in] M in which these builtins are created.
///
/// @return The mangled work-group builtin corresponding to `SubgroupBuiltin`.
std::string lookupWGBuiltin(StringRef SubgroupBuiltin, LLVMContext *Ctx,
                            Module *M) {
  // We must handle the case where we're replacing a __mux_sub_group_barrier
  // with a __mux_work_group_barrier. Our 'demangleName' API works differently
  // with non-mangled builtin names and returns an empty string. Just work
  // around it specifically.
  if (SubgroupBuiltin == compiler::utils::MuxBuiltins::sub_group_barrier) {
    return compiler::utils::MuxBuiltins::work_group_barrier;
  }

  compiler::utils::NameMangler Mangler(Ctx, M);
  SmallVector<Type *, 4> ArgumentTypes;
  SmallVector<compiler::utils::TypeQualifiers, 4> Qualifiers;

  const auto DemangledName = std::string(
      Mangler.demangleName(SubgroupBuiltin, ArgumentTypes, Qualifiers));

  const auto WorkGroupBuiltinName = "work" + DemangledName.substr(3);
  // We have to special case broadcast here since the sub-group version takes
  // a single uint but we need to map this to the 3D work-group version which
  // takes a size_t.
  if (std::string::npos != WorkGroupBuiltinName.find("broadcast")) {
    // Here we are mapping Tj -> Tmmm for any type T (assuming size_t is
    // unsigned long). So first remap the existing type.
    auto *const sizeTy = compiler::utils::getSizeType(*M);
    ArgumentTypes.back() = sizeTy;

    // Then we need to push back two more size_ts for the Y and Z arguments.
    for (unsigned i = 0; i < 2; ++i) {
      ArgumentTypes.push_back(sizeTy);
      Qualifiers.push_back(compiler::utils::eTypeQualNone);
    }
  }

  return Mangler.mangleName(WorkGroupBuiltinName, ArgumentTypes, Qualifiers);
}

/// @brief Helper for determining if a call instruction calls a sub-group
/// work-item buitlin function.
///
/// @param[in] CI Call instruction to query.
///
/// @return True if CI is a call to sub-group work-item builtin, false
/// otherwise.
bool isSubGroupWorkItemFunction(CallInst *CI) {
  static const std::set<StringRef> SubGroupWorkItemBuiltins{
      "_Z18get_sub_group_sizev", "_Z22get_max_sub_group_sizev",
      "_Z18get_num_sub_groupsv", "_Z27get_enqueued_num_sub_groupsv",
      "_Z16get_sub_group_idv",   "_Z22get_sub_group_local_idv"};

  return SubGroupWorkItemBuiltins.count(CI->getCalledFunction()->getName());
}

/// @brief Replaces sub-group builtin calls with their work-group equivalents.
///
/// @param[in] SubGroupBuiltinCalls Buitlin calls to replace.
void replaceSubGroupBuiltinCalls(
    const SmallVectorImpl<CallInst *> &SubGroupBuiltinCalls,
    compiler::utils::BuiltinInfo &BI) {
  for (auto *I : SubGroupBuiltinCalls) {
    auto *const SubGroupBuiltin = I->getCalledFunction();
    auto *const M = I->getModule();
    if (!SubGroupBuiltin->getName().contains("broadcast")) {
      // We can just forward the argument directly to the
      // work-group builtin for everything except broadcasts.
      SmallVector<Value *, 4> Args;
      for (auto &arg : I->args()) {
        Args.push_back(arg);
      }
      auto WorkGroupBuiltin = M->getOrInsertFunction(
          lookupWGBuiltin(SubGroupBuiltin->getName(), &M->getContext(), M),
          SubGroupBuiltin->getFunctionType());
      auto *const WorkGroupBuiltinFcn =
          cast<Function>(WorkGroupBuiltin.getCallee());
      WorkGroupBuiltinFcn->setCallingConv(SubGroupBuiltin->getCallingConv());
      WorkGroupBuiltinFcn->setConvergent();
      auto *WGCI = CallInst::Create(WorkGroupBuiltin, Args, "", I);
      WGCI->setCallingConv(I->getCallingConv());
      I->replaceAllUsesWith(WGCI);
      continue;
    }
    // Broadcasts don't map particularly well from sub-groups to work-groups.
    // This is because the sub-group broadcast expects an index in the half
    // closed interval [0, get_sub_group_size()), where as the work-group
    // broadcasts expect the index arguments to be in the ranges [0,
    // get_local_size(0)), [0, get_local_size(1)), [0, get_local_size(2)) for
    // the 1D, 2D and 3D overloads respectively. This means that we need to
    // invert the mapping of sub-group local id to the local (x, y, z)
    // coordinates of the enqueue. This amounts to solving get_local_linear_id
    // (since this is the sub-group local id) for x, y and z given ID of a
    // sub-group element: x = ID % get_local_size(0) y = (ID - x) /
    // get_local_size(0) % get_local_size(1) z = (ID - x - y *
    // get_local_size(0) / (get_local_size(0) * get_local_size(1)
    IRBuilder<> Builder{I};
    auto *const Value = I->getArgOperand(0);
    auto *const SubGroupElementID = I->getArgOperand(1);

    auto *const GetLocalSize =
        BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalSize, *M);
    auto *const LocalSizeX = Builder.CreateIntCast(
        Builder.CreateCall(
            GetLocalSize,
            ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)),
        SubGroupElementID->getType(), /* isSigned */ false);
    auto *const LocalSizeY = Builder.CreateIntCast(
        Builder.CreateCall(
            GetLocalSize,
            ConstantInt::get(Type::getInt32Ty(M->getContext()), 1)),
        SubGroupElementID->getType(), /* isSigned */ false);

    auto *X = Builder.CreateURem(SubGroupElementID, LocalSizeX, "x");
    auto *Y = Builder.CreateURem(
        Builder.CreateUDiv(Builder.CreateSub(SubGroupElementID, X), LocalSizeX),
        LocalSizeY, "y");
    auto *Z = Builder.CreateUDiv(
        Builder.CreateSub(
            SubGroupElementID,
            Builder.CreateAdd(X, Builder.CreateMul(Y, LocalSizeX))),
        Builder.CreateMul(LocalSizeX, LocalSizeY), "z");

    auto *const ValueTy = Value->getType();
    auto *const SizeType = compiler::utils::getSizeType(*M);
    auto *const WGBroadcastFcnTy =
        FunctionType::get(SubGroupBuiltin->getReturnType(),
                          {ValueTy, SizeType, SizeType, SizeType},
                          /* isVarArg */ false);
    const auto WorkGroupBroadcastName =
        lookupWGBuiltin(SubGroupBuiltin->getName(), &M->getContext(), M);
    auto WorkGroupBroadcast =
        M->getOrInsertFunction(WorkGroupBroadcastName, WGBroadcastFcnTy);
    auto *const WorkGroupBroadcastFcn =
        cast<Function>(WorkGroupBroadcast.getCallee());
    WorkGroupBroadcastFcn->setCallingConv(SubGroupBuiltin->getCallingConv());
    WorkGroupBroadcastFcn->setNotConvergent();
    // Because sub_group_broadcast takes uint as its index argument but
    // work_group_broadcast takes size_t we potentially need cast here to the
    // native size_t.
    X = Builder.CreateIntCast(X, SizeType, /* isSigned */ false);
    Y = Builder.CreateIntCast(Y, SizeType, /* isSigned */ false);
    Z = Builder.CreateIntCast(Z, SizeType, /* isSigned */ false);
    auto *const WGCI = Builder.CreateCall(WorkGroupBroadcast, {Value, X, Y, Z});
    I->replaceAllUsesWith(WGCI);
  }
}

/// @brief Replace sub-group work-item builtin calls with suitable values for
/// the degenerate sub-group case.
///
/// @param[in] SubGroupBuiltinCalls Sub-group work-item builtin calls to
/// replace.
void replaceSubGroupWorkItemBuiltinCalls(
    const SmallVectorImpl<CallInst *> &SubGroupBuiltinCalls,
    compiler::utils::BuiltinInfo &BI) {
  for (auto &Call : SubGroupBuiltinCalls) {
    const auto CalledFunctionName = Call->getCalledFunction()->getName();
    // Handle get_sub_group_size & get_max_sub_group_size.
    // The sub-group is the work-group, meaning the sub-group size is the total
    // local size.
    if (CalledFunctionName.contains("sub_group_size")) {
      auto *const M = Call->getModule();
      IRBuilder<> Builder{Call};
      auto *const GetLocalSize = BI.getOrDeclareMuxBuiltin(
          compiler::utils::eMuxBuiltinGetLocalSize, *M);
      GetLocalSize->setCallingConv(Call->getCallingConv());

      Value *TotalLocalSize =
          ConstantInt::get(compiler::utils::getSizeType(*M), 1);
      for (unsigned i = 0; i < 3; ++i) {
        auto *const LocalSize = Builder.CreateCall(
            GetLocalSize,
            ConstantInt::get(Type::getInt32Ty(M->getContext()), i));
        LocalSize->setCallingConv(Call->getCallingConv());
        TotalLocalSize = Builder.CreateMul(LocalSize, TotalLocalSize);
      }
      TotalLocalSize = Builder.CreateIntCast(TotalLocalSize, Call->getType(),
                                             /* isSigned */ false);
      Call->replaceAllUsesWith(TotalLocalSize);
    } else if (CalledFunctionName.contains("num_sub_groups")) {
      // Handle get_num_sub_groups & get_enqueued_num_sub_groups.
      // The sub-group is the work-group, meaning there is exactly 1 sub-group.
      auto *const One = ConstantInt::get(Call->getType(), 1);
      Call->replaceAllUsesWith(One);
    } else if (CalledFunctionName.contains("get_sub_group_id")) {
      // Handle get_sub_group_id. The sub-group is the work-group, meaning the
      // sub-group id is 0.
      auto *const Zero = ConstantInt::get(Call->getType(), 0);
      Call->replaceAllUsesWith(Zero);
    } else if (CalledFunctionName.contains("get_sub_group_local_id")) {
      // Handle get_sub_group_local_id. The sub-group local id is a unique
      // local id of the work item, here we use get_local_linear_id.
      auto *const M = Call->getModule();
      auto *const GetLocalLinearID = BI.getOrDeclareMuxBuiltin(
          compiler::utils::eMuxBuiltinGetLocalLinearId, *M);
      GetLocalLinearID->setCallingConv(Call->getCallingConv());
      auto *const LocalLinearIDCall =
          CallInst::Create(GetLocalLinearID, {}, "", Call);
      LocalLinearIDCall->setCallingConv(Call->getCallingConv());
      auto *const LocalLinearID = CastInst::CreateIntegerCast(
          LocalLinearIDCall, Type::getInt32Ty(M->getContext()),
          /* isSigned */ false, "", Call);
      Call->replaceAllUsesWith(LocalLinearID);
    } else {
      llvm_unreachable("unhandled sub-group builtin function");
    }
  }
}
}  // namespace

PreservedAnalyses compiler::utils::DegenerateSubGroupPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);
  // Find all the sub-group builtin calls in the module.
  // Need to do this up front because we are about to insert and remove
  // instructions, which will invalidate iterators.
  SmallVector<CallInst *, 32> SubGroupFunctionCalls;
  SmallVector<CallInst *, 32> SubGroupWorkItemFunctionCalls;

  for (auto &F : M) {
    if (isKernel(F)) {
      // Flag the presence of degenerate sub-groups in this kernel.
      // There might not be any sub-group builtins, in which case it's academic,
      // but the user has requested them.
      setHasDegenerateSubgroups(F);
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *CI = dyn_cast<CallInst>(&I)) {
          if (isSubGroupFunction(CI)) {
            SubGroupFunctionCalls.push_back(CI);
          } else if (isSubGroupWorkItemFunction(CI)) {
            SubGroupWorkItemFunctionCalls.push_back(CI);
          }
        }
      }
    }
  }

  // If there were no sub-group builtin calls we are done, exit early and
  // preserve all analysis since we didn't touch the module.
  if (SubGroupFunctionCalls.empty() && SubGroupWorkItemFunctionCalls.empty()) {
    return PreservedAnalyses::all();
  }

  // Otherwise replace the sub-group function builtin calls with work-group
  // builtin calls.
  replaceSubGroupBuiltinCalls(SubGroupFunctionCalls, BI);

  // Replace the sub-group work-item builtin calls with work-group work-item
  // builtin calls.
  replaceSubGroupWorkItemBuiltinCalls(SubGroupWorkItemFunctionCalls, BI);

  // Remove the old instructions from the module.
  for (auto *I : SubGroupFunctionCalls) {
    I->eraseFromParent();
  }

  for (auto *I : SubGroupWorkItemFunctionCalls) {
    I->eraseFromParent();
  }

  // If we got this far then we changed something, maybe this is too
  // conservative, but assume we invalidated all analyses.
  return PreservedAnalyses::none();
}
