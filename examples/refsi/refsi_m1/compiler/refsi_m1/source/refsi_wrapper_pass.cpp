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
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <refsi_m1/refsi_wrapper_pass.h>

using namespace llvm;

namespace {

/// @brief Store a value to the schedule struct
/// @param Builder IRBuilder to use
/// @param MuxWorkGroupStructTy Scheduling structure type
/// @param Sched Schedule struct
/// @param Element Top level index into the struct
/// @param Index Index into the sub array of the element. If this is not an
/// array element, this value will be ignored.
/// @param Val Value to be stored
void storeToSchedStruct(IRBuilder<> &Builder, StructType *MuxWorkGroupStructTy,
                        Value *Sched, uint32_t Element, uint32_t Index,
                        Value *Val) {
  assert(Sched->getType()->isPointerTy());

  Value *IndicesArray[3] = {Builder.getInt32(0), Builder.getInt32(Element),
                            Builder.getInt32(Index)};

  Type *ElTy = GetElementPtrInst::getIndexedType(
      MuxWorkGroupStructTy, llvm::ArrayRef<Value *>(IndicesArray, 2));
  ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

  if (ArrayTy) {
    assert(Index < ArrayTy->getNumElements());
  }

  Value *SchedLookupPtr =
      Builder.CreateGEP(MuxWorkGroupStructTy, Sched,
                        ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));

  Builder.CreateStore(Val, SchedLookupPtr);
}

/// @brief Load a value from the schedule struct
/// @param Builder IRBuilder to use
/// @param MuxWorkGroupStructTy Scheduling structure type
/// @param Sched Schedule struct
/// @param Element Top level index into the struct
/// @param Index Index into the sub array of the element. If this is not an
/// array element, this value will be ignored.
/// @return The value loaded from the struct
Value *loadFromSchedStruct(IRBuilder<> &Builder,
                           StructType *MuxWorkGroupStructTy, Value *Sched,
                           uint32_t Element, uint32_t Index) {
  assert(Sched->getType()->isPointerTy());
  Value *IndicesArray[3] = {Builder.getInt32(0), Builder.getInt32(Element),
                            Builder.getInt32(Index)};
  // Check if it's an array type
  Type *ElTy = GetElementPtrInst::getIndexedType(
      MuxWorkGroupStructTy, llvm::ArrayRef<Value *>(IndicesArray, 2));
  ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

  if (ArrayTy) {
    assert(Index < ArrayTy->getNumElements());
  }

  Value *SchedLookupPtr =
      Builder.CreateGEP(MuxWorkGroupStructTy, Sched,
                        ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));
  Type *ValTy = GetElementPtrInst::getIndexedType(
      MuxWorkGroupStructTy, ArrayRef<Value *>(IndicesArray, ArrayTy ? 3 : 2));
  Value *SchedValue = Builder.CreateLoad(ValTy, SchedLookupPtr);

  return SchedValue;
}

/// @brief Copy a whole element from one struct to another
/// @param Builder IRBuilder to use
/// @param MuxWorkGroupStructTy Scheduling structure type
/// @param SchedIn Input scheduling struct
/// @param SchedOut Output scheduling struct
/// @param Element Element index within scheduling struct
void CopyElementToNewSchedStruct(IRBuilder<> &Builder,
                                 StructType *MuxWorkGroupStructTy,
                                 Value *SchedIn, Value *SchedOut,
                                 uint32_t Element) {
  Value *IndicesArray[2] = {Builder.getInt32(0), Builder.getInt32(Element)};
  Type *ElTy =
      GetElementPtrInst::getIndexedType(MuxWorkGroupStructTy, IndicesArray);
  ArrayType *ArrayTy = dyn_cast_or_null<ArrayType>(ElTy);

  // If it's an array get the number of elements
  uint32_t Count = ArrayTy ? ArrayTy->getNumElements() : 1;
  for (uint32_t i = 0; i < Count; i++) {
    Value *SchedValue =
        loadFromSchedStruct(Builder, MuxWorkGroupStructTy, SchedIn, Element, i);
    storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedOut, Element, i,
                       SchedValue);
  }
}

}  // namespace

namespace refsi_m1 {

/// @brief The index of the scheduling struct in the list of arguments.
const unsigned int SchedStructArgIndex = 3;
const unsigned int InstanceArgIndex = 0;
const unsigned int SliceArgIndex = 1;

llvm::Function *addKernelWrapper(llvm::Module &M, llvm::Function &F) {
  // Make types for the wrapper pass based on original parameters and
  // additional instance/slice params.
  // We add two int64Ty for the Instance Id and Slice Id prior to the kernel
  // arguments.

  SmallVector<Type *, 4> ArgTypes;
  ArgTypes.push_back(Type::getInt64Ty(M.getContext()));
  ArgTypes.push_back(Type::getInt64Ty(M.getContext()));
  for (auto &Arg : F.getFunctionType()->params()) {
    ArgTypes.push_back(Arg);
  }
  Function *NewFunction = compiler::utils::createKernelWrapperFunction(
      M, F, ArgTypes, ".refsi-wrapper");

  // Copy over the old parameter names and attributes
  for (unsigned i = 0, e = F.arg_size(); i != e; i++) {
    auto *NewArg = NewFunction->getArg(i + 2);
    NewArg->setName(F.getArg(i)->getName());
    NewFunction->addParamAttrs(
        i + 2, AttrBuilder(F.getContext(), F.getAttributes().getParamAttrs(i)));
  }
  NewFunction->getArg(InstanceArgIndex)->setName("instance");
  NewFunction->getArg(SliceArgIndex)->setName("slice");

  if (!NewFunction->hasFnAttribute(Attribute::NoInline)) {
    NewFunction->addFnAttr(Attribute::AlwaysInline);
  }

  // get the arguments
  SmallVector<Value *, 8> Args;

  Argument *SchedArg = NewFunction->getArg(SchedStructArgIndex);
  Argument *InstanceArg = NewFunction->getArg(InstanceArgIndex);
  Argument *SliceArg = NewFunction->getArg(SliceArgIndex);
  IRBuilder<> Builder(
      BasicBlock::Create(NewFunction->getContext(), "", NewFunction));
  auto *MuxWorkGroupStructTy = compiler::utils::getWorkGroupInfoStructTy(M);
  auto *SchedCopyInst = Builder.CreateAlloca(MuxWorkGroupStructTy);

  Value *NumGroups1 = loadFromSchedStruct(
      Builder, MuxWorkGroupStructTy, SchedArg,
      compiler::utils::WorkGroupInfoStructField::num_groups, 1);

  CopyElementToNewSchedStruct(
      Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
      compiler::utils::WorkGroupInfoStructField::num_groups);
  CopyElementToNewSchedStruct(
      Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
      compiler::utils::WorkGroupInfoStructField::global_offset);
  CopyElementToNewSchedStruct(
      Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
      compiler::utils::WorkGroupInfoStructField::local_size);
  CopyElementToNewSchedStruct(
      Builder, MuxWorkGroupStructTy, SchedArg, SchedCopyInst,
      compiler::utils::WorkGroupInfoStructField::work_dim);

  Value *GroupId1 = Builder.CreateURem(SliceArg, NumGroups1);
  Value *GroupId2 = Builder.CreateUDiv(SliceArg, NumGroups1);

  storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                     compiler::utils::WorkGroupInfoStructField::group_id, 0,
                     InstanceArg);

  storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                     compiler::utils::WorkGroupInfoStructField::group_id, 1,
                     GroupId1);

  storeToSchedStruct(Builder, MuxWorkGroupStructTy, SchedCopyInst,
                     compiler::utils::WorkGroupInfoStructField::group_id, 2,
                     GroupId2);

  unsigned int ArgIndex = 0;
  for (auto &Arg : NewFunction->args()) {
    if (ArgIndex > SliceArgIndex) {
      if (ArgIndex == SchedStructArgIndex) {
        Args.push_back(SchedCopyInst);
      } else {
        Args.push_back(&Arg);
      }
    }
    ArgIndex++;
  }

  compiler::utils::createCallToWrappedFunction(
      F, Args, Builder.GetInsertBlock(), Builder.GetInsertPoint());

  Builder.CreateRetVoid();
  return NewFunction;
}

llvm::PreservedAnalyses RefSiM1WrapperPass::run(llvm::Module &M,
                                                llvm::ModuleAnalysisManager &) {
  (void)M;
  bool modified = false;
  SmallPtrSet<Function *, 4> NewKernels;
  for (auto &F : M.functions()) {
    if (compiler::utils::isKernel(F) && !NewKernels.count(&F)) {
      auto *NewFunction = addKernelWrapper(M, F);
      modified = true;
      NewKernels.insert(NewFunction);
    }
  }
  return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
}  // namespace refsi_m1
