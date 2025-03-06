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
#include <compiler/utils/metadata.h>
#include <compiler/utils/replace_target_ext_tys_pass.h>
#include <compiler/utils/target_extension_types.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/llvm_version.h>

using namespace llvm;

class TargetExtTypeRemapper : public ValueMapTypeRemapper {
 public:
  TargetExtTypeRemapper(Module &M, compiler::utils::BuiltinInfo &BI,
                        bool ReplaceImages, bool ReplaceSamplers,
                        bool ReplaceEvents)
      : M(M),
        BI(BI),
        ReplaceImages(ReplaceImages),
        ReplaceSamplers(ReplaceSamplers),
        ReplaceEvents(ReplaceEvents) {}

  Type *remapType(Type *Ty) override {
    // Look up the cache in case we've seen this type before.
    if (auto I = TyReplacementCache.find(Ty); I != TyReplacementCache.end()) {
      return I->getSecond();
    }

    // Replace array types with remappable element types.
    if (auto *const ArrayTy = dyn_cast<ArrayType>(Ty)) {
      if (auto *NewTy = remapType(ArrayTy->getElementType()); Ty != NewTy) {
        auto *const NewArrayTy =
            ArrayType::get(NewTy, ArrayTy->getArrayNumElements());
        TyReplacementCache[Ty] = NewArrayTy;
        return NewArrayTy;
      }
    }

    // Replace any struct types with remappable element types.
    if (auto *const StructTy = dyn_cast<StructType>(Ty)) {
      SmallVector<Type *> NewStructEltTys(StructTy->getNumElements());
      transform(StructTy->elements(), NewStructEltTys.begin(),
                [this](Type *EltTy) { return remapType(EltTy); });
      // No change to be made to this struct
      if (equal(StructTy->elements(), NewStructEltTys)) {
        return Ty;
      }
      auto *const NewStructTy =
          StructTy->hasName()
              ? StructType::create(StructTy->getContext(), NewStructEltTys,
                                   StructTy->getName(), StructTy->isPacked())
              : StructType::get(StructTy->getContext(), NewStructEltTys,
                                StructTy->isPacked());
      TyReplacementCache[Ty] = NewStructTy;
      return NewStructTy;
    }

    // Don't replace this type if it's:
    // * not a TargetExtType
    // * an image and we don't want to replace images
    // * a sampler we don't want to replace samplers
    // * an event and we don't want to replace events
    if (auto *const TgtExtTy = dyn_cast<TargetExtType>(Ty);
        !TgtExtTy || (!ReplaceImages && TgtExtTy->getName() == "spirv.Image") ||
        (!ReplaceSamplers && TgtExtTy->getName() == "spirv.Sampler") ||
        (!ReplaceEvents && TgtExtTy->getName() == "spirv.Event")) {
      return Ty;
    }
    // Check if the target wants to remap this type.
    if (auto *NewTy = BI.getRemappedTargetExtTy(Ty, M)) {
      // Check that the replacement type's size and alignment is valid, as
      // otherwise this pass may leave the module in an invalid state.
      [[maybe_unused]] auto &DL = M.getDataLayout();
      assert(
          DL.getTypeAllocSize(NewTy) == DL.getTypeAllocSize(Ty) &&
          "Chosen target replacement type may leave module in invalid state");
      // Cache this type for next time.
      TyReplacementCache[Ty] = NewTy;
      return NewTy;
    }
    return Ty;
  }

 private:
  Module &M;
  compiler::utils::BuiltinInfo &BI;
  bool ReplaceImages = true;
  bool ReplaceSamplers = true;
  bool ReplaceEvents = true;
  DenseMap<Type *, Type *> TyReplacementCache;
};

PreservedAnalyses compiler::utils::ReplaceTargetExtTysPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<compiler::utils::BuiltinInfoAnalysis>(M);

  ValueToValueMapTy VM;
  TargetExtTypeRemapper TyMapper(M, BI, ReplaceImages, ReplaceSamplers,
                                 ReplaceEvents);

  SmallPtrSet<Function *, 4> ToDelete;
  // Note that despite us using a ValueMapper to remap functions, below, we're
  // still safest creating new IR values and replacing the old ones with them.
  // The ValueMapper ostensibly can mutate function arguments in-place. However,
  // in practice I've observed the functions looking like they've been remapped,
  // even dumping this to the console after the pass has run, but LLVM's
  // verifier can still pick up on the old module state and throw an error due
  // to mismatched function arguments.
  for (auto &F : M) {
    auto *FTy = F.getFunctionType();
    auto *const NewFRetTy = TyMapper.remapType(FTy->getReturnType());

    // Transform old parameter types to new types.
    SmallVector<Type *> NewFParams(FTy->getNumParams());
    transform(FTy->params(), NewFParams.begin(),
              [&TyMapper](Type *Ty) { return TyMapper.remapType(Ty); });

    // Skip this function if its prototype doesn't need replaced.
    if (FTy->getReturnType() == NewFRetTy && equal(FTy->params(), NewFParams)) {
      continue;
    }

    auto *const NewFTy = FunctionType::get(NewFRetTy, NewFParams, F.isVarArg());

    auto *NewF = Function::Create(NewFTy, F.getLinkage(), "", &M);

    // Set up a mapping from the old function to the new one
    VM[&F] = NewF;

    // Steal the old function's name
    NewF->takeName(&F);

    // Set the correct calling convention
    NewF->setCallingConv(F.getCallingConv());

    // Steal all the old parameters' names
    for (auto [Old, New] : zip(F.args(), NewF->args())) {
      New.takeName(&Old);
    }

    if (F.isDeclaration()) {
      // Copy debug info for function over; CloneFunctionInto takes care of this
      // if this function has a body
      NewF->setSubprogram(F.getSubprogram());
      // Copy the attributes over to the new function
      NewF->setAttributes(F.getAttributes());
      // Copy the metadata over to the new function, ignoring any debug info.
      compiler::utils::copyFunctionMetadata(F, *NewF);
    } else {
      // Map all original function arguments to the new ones
      for (auto [Old, New] : zip(F.args(), NewF->args())) {
        VM[&Old] = &New;
      }
      SmallVector<ReturnInst *, 4> Returns;
      CloneFunctionInto(NewF, &F, VM, CloneFunctionChangeType::LocalChangesOnly,
                        Returns, /*NameSuffix*/ "", /*CodeInfo*/ nullptr,
                        &TyMapper);
    }

    ToDelete.insert(&F);
  }

  ValueMapper Mapper(VM, RF_IgnoreMissingLocals | RF_ReuseAndMutateDistinctMDs,
                     &TyMapper);

  // Keep the dead functions around for a bit longer so that we can auto-remap
  // their uses to their replacements.
  for (auto &F : M) {
    if (!ToDelete.contains(&F)) {
      Mapper.remapFunction(F);
    }
  }

  for (auto *F : ToDelete) {
    // There might be remaining uses of the old function, outside of other
    // functions, e.g., in metadata. Clear those up now before deleting the old
    // function.
    F->replaceAllUsesWith(VM[F]);
    F->eraseFromParent();
  }

  return PreservedAnalyses::none();
}
