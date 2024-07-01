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
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <refsi_g1_wi/refsi_mux_builtin_info.h>

#include <cstdint>

using namespace llvm;
using namespace refsi_g1_wi;

StructType *RefSiG1BIMuxInfo::getExecStateStruct(Module &M) {
  static constexpr const char *StructName = "exec_state";
  if (auto *ty = StructType::getTypeByName(M.getContext(), StructName)) {
    return ty;
  }

  auto &Ctx = M.getContext();
  auto *const uint_type = Type::getInt32Ty(Ctx);
  auto *const size_type = compiler::utils::getSizeType(M);
  auto *const ptr_type = PointerType::get(Ctx, 0);

  Type *elements[ExecStateStruct::total];

  elements[ExecStateStruct::wg] = compiler::utils::getWorkGroupInfoStructTy(M);
  // FIXME: These two are actually in the HAL's wg info
  elements[ExecStateStruct::num_groups_per_call] = ArrayType::get(size_type, 3);
  elements[ExecStateStruct::hal_extra] = ptr_type;

  elements[ExecStateStruct::local_id] = ArrayType::get(uint_type, 3);
  elements[ExecStateStruct::kernel_entry] = ptr_type;
  elements[ExecStateStruct::packed_args] = ptr_type;
  elements[ExecStateStruct::magic] = uint_type;
  elements[ExecStateStruct::state_size] = uint_type;
  elements[ExecStateStruct::flags] = uint_type;
  elements[ExecStateStruct::next_xfer_id] = uint_type;
  elements[ExecStateStruct::thread_id] = uint_type;

  return StructType::create(elements, StructName);
}

Function *RefSiG1BIMuxInfo::getOrDeclareMuxBuiltin(
    compiler::utils::BuiltinID ID, Module &M,
    llvm::ArrayRef<llvm::Type *> OverloadInfo) {
  auto *F = compiler::utils::BIMuxInfoConcept::getOrDeclareMuxBuiltin(
      ID, M, OverloadInfo);
  if (!F) {
    return F;
  }
  switch (ID) {
    default:
      break;
    case compiler::utils::eMuxBuiltinMemBarrier:
    case compiler::utils::eMuxBuiltinSubGroupBarrier:
    case compiler::utils::eMuxBuiltinWorkGroupBarrier:
      if (F->hasFnAttribute(Attribute::AlwaysInline)) {
        F->removeFnAttr(Attribute::AlwaysInline);
      }
      F->addFnAttr(Attribute::NoInline);
      break;
  }
  return F;
}

Function *RefSiG1BIMuxInfo::defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                             Module &M,
                                             ArrayRef<Type *> OverloadInfo) {
  assert(compiler::utils::BuiltinInfo::isMuxBuiltinID(ID) &&
         "Only handling mux builtins");
  Function *F = M.getFunction(
      compiler::utils::BuiltinInfo::getMuxBuiltinName(ID, OverloadInfo));

  // FIXME: We'd ideally want to declare it here to reduce pass
  // inter-dependencies.
  assert(F && "Function should have been pre-declared");
  if (!F->isDeclaration()) {
    return F;
  }

  if (ID == compiler::utils::eMuxBuiltinWorkGroupBarrier) {
    // Set 'noinline' now so setDefaultBuiltinAttributes doesn't add
    // 'alwaysinline'
    if (F->hasFnAttribute(Attribute::AlwaysInline)) {
      F->removeFnAttr(Attribute::AlwaysInline);
    }
    F->addFnAttr(Attribute::NoInline);
    F->addFnAttr(Attribute::NoDuplicate);
    RefSiG1BIMuxInfo::setDefaultBuiltinAttributes(*F);
    F->addFnAttr(Attribute::Convergent);
    // We don't mark this builtin as 'internal', unlike other builtins, as LLVM
    // will optimize away the parameter in some cases and we're left with 'mv
    // a0, 0' which isn't valid assembly!
    IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

    // Call the __mux_mem_barrier builtin, to ensure memory is synchronized.
    auto *MemBarrier =
        getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinMemBarrier, M);
    assert(MemBarrier);
    B.CreateCall(MemBarrier, {F->getArg(1), F->getArg(2)});

    // Now we synchronize the threads, using the RefSi system call:
    // 1. Load its syscall ID (2) into a7
    // 2. Move the barrier ID into the first argument a0
    // 3. Call the barrier.
    auto *const InlineAsmTy =
        FunctionType::get(B.getVoidTy(), {B.getInt64Ty()}, false);
    auto *IDVal = B.CreateZExt(F->getArg(0), B.getInt64Ty());
    InlineAsm *CallBarrier = InlineAsm::get(InlineAsmTy,
                                            "li a7, 2\n"
                                            "mv a0, $0\n"
                                            "ecall\n",
                                            "r", true);
    B.CreateCall(CallBarrier, {IDVal});
    B.CreateRetVoid();
    return F;
  }

  // Our DMA implementation is such that DMA transfer triggers instantly, so
  // the __mux_dma_wait builtin just becomes a barrier, to force work-items to
  // wait on the transfer to 'complete'.
  if (ID == compiler::utils::eMuxBuiltinDMAWait) {
    F->addFnAttr(Attribute::NoDuplicate);
    RefSiG1BIMuxInfo::setDefaultBuiltinAttributes(*F);
    F->addFnAttr(Attribute::Convergent);
    F->setLinkage(GlobalValue::InternalLinkage);
    IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

    auto *CtrlBarrier =
        getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinWorkGroupBarrier, M);
    assert(CtrlBarrier);
    B.CreateCall(CtrlBarrier, {B.getInt32(0), B.getInt32(MemScopeWorkGroup),
                               B.getInt32(MemSemanticsAcquireRelease |
                                          MemSemanticsWorkGroupMemory |
                                          MemSemanticsCrossWorkGroupMemory)});
    B.CreateRetVoid();
    return F;
  }

  if (ID == compiler::utils::eMuxBuiltinGetSubGroupId) {
    // The sub-group ID is just the linear ID, since we use trivial sub-groups.
    RefSiG1BIMuxInfo::setDefaultBuiltinAttributes(*F);
    F->addFnAttr(Attribute::Convergent);
    F->setLinkage(GlobalValue::InternalLinkage);
    IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));
    auto *LocalLinearIdFn =
        getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalLinearId, M);
    assert(LocalLinearIdFn);
    auto *LocalLinearId =
        B.CreateCall(LocalLinearIdFn, {F->getArg(0), F->getArg(1)});
    auto *const T = B.CreateTrunc(LocalLinearId, F->getReturnType());
    B.CreateRet(T);
    return F;
  }

  if (ID == compiler::utils::eMuxBuiltinGetMaxSubGroupSize) {
    // The size of sub-groups should always be one, as we use trivial
    // sub-groups.
    RefSiG1BIMuxInfo::setDefaultBuiltinAttributes(*F);
    F->addFnAttr(Attribute::Convergent);
    F->setLinkage(GlobalValue::InternalLinkage);
    IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));
    B.CreateRet(B.getInt32(1));
    return F;
  }

  if (ID == compiler::utils::eMuxBuiltinGetNumSubGroups) {
    // Our sub-groups are of size 1, so there's as many as there are
    // work-items. Multiply all of the local work-group sizes together.
    RefSiG1BIMuxInfo::setDefaultBuiltinAttributes(*F);
    F->addFnAttr(Attribute::Convergent);
    F->setLinkage(GlobalValue::InternalLinkage);
    IRBuilder<> B(BasicBlock::Create(M.getContext(), "", F));

    auto *const LocalSizeFn =
        getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalSize, M);
    assert(LocalSizeFn);

    auto *const LocalSizeX =
        B.CreateCall(LocalSizeFn, {B.getInt32(0), F->getArg(0), F->getArg(1)});
    LocalSizeX->setAttributes(LocalSizeFn->getAttributes());
    LocalSizeX->setCallingConv(LocalSizeFn->getCallingConv());

    auto *const LocalSizeY =
        B.CreateCall(LocalSizeFn, {B.getInt32(1), F->getArg(0), F->getArg(1)});
    LocalSizeY->setAttributes(LocalSizeFn->getAttributes());
    LocalSizeY->setCallingConv(LocalSizeFn->getCallingConv());

    auto *const LocalSizeZ =
        B.CreateCall(LocalSizeFn, {B.getInt32(2), F->getArg(0), F->getArg(1)});
    LocalSizeZ->setAttributes(LocalSizeFn->getAttributes());
    LocalSizeZ->setCallingConv(LocalSizeFn->getCallingConv());

    auto *const LocalSizeXY = B.CreateMul(LocalSizeX, LocalSizeY);
    auto *const LocalSizeXYZ = B.CreateMul(LocalSizeXY, LocalSizeZ);

    B.CreateRet(B.CreateTrunc(LocalSizeXYZ, B.getInt32Ty()));

    return F;
  }

  if (ID != compiler::utils::eMuxBuiltinGetLocalId) {
    return compiler::utils::BIMuxInfoConcept::defineMuxBuiltin(ID, M,
                                                               OverloadInfo);
  }

  unsigned ParamIdx = 1;
  unsigned WGFieldIdx = ExecStateStruct::thread_id;

  assert(F);

  auto SchedParams = getFunctionSchedulingParameters(*F);
  assert(SchedParams.size() > ParamIdx && "Missing scheduling parameters");

  auto &Ctx = M.getContext();
  auto *const uint_type = Type::getInt32Ty(Ctx);

  auto *structTy = getExecStateStruct(M);
  const auto &SchedParam = SchedParams[ParamIdx];

  auto *const ty = F->getReturnType();
  IRBuilder<> ir(BasicBlock::Create(F->getContext(), "", F));

  Value *rank = F->getArg(0);

  SmallVector<Value *, 3> gep_indices{ir.getInt32(0), ir.getInt32(WGFieldIdx)};
  auto *const gep = ir.CreateGEP(structTy, SchedParam.ArgVal, gep_indices);

  Value *thread_id = ir.CreateLoad(uint_type, gep);
  thread_id = ir.CreateZExt(thread_id, ty);

  auto get_local_size_fn =
      getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalSize, M);
  assert(get_local_size_fn);

  auto sched_params = getFunctionSchedulingParameters(*F);
  SmallVector<Value *, 4> local_size_x_args{ir.getInt32(0)};
  SmallVector<Value *, 4> local_size_y_args{ir.getInt32(1)};
  for (auto &i : sched_params) {
    local_size_x_args.push_back(i.ArgVal);
    local_size_y_args.push_back(i.ArgVal);
  }
  auto *const local_size_x =
      ir.CreateCall(get_local_size_fn, local_size_x_args, "local_size.x");
  auto *const local_size_y =
      ir.CreateCall(get_local_size_fn, local_size_y_args, "local_size.y");

  auto *const local_id_x = ir.CreateURem(thread_id, local_size_x);
  auto *const local_id_y =
      ir.CreateURem(ir.CreateUDiv(thread_id, local_size_x), local_size_y);
  auto *const local_id_z =
      ir.CreateUDiv(ir.CreateUDiv(thread_id, local_size_x), local_size_y);

  auto *const is_oob = ir.CreateICmpSGT(rank, ir.getInt32(2));
  auto *const local_id_z_or_default =
      ir.CreateSelect(is_oob, Constant::getNullValue(ty), local_id_z);

  auto *const is_y = ir.CreateICmpEQ(rank, ir.getInt32(1));
  auto *const local_id_y_or_z =
      ir.CreateSelect(is_y, local_id_y, local_id_z_or_default);

  auto *const is_x = ir.CreateICmpEQ(rank, ir.getInt32(0));
  auto *const ret = ir.CreateSelect(is_x, local_id_x, local_id_y_or_z);

  ir.CreateRet(ret);

  return nullptr;
}
