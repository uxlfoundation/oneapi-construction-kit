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

#include <compiler/utils/dma.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <multi_llvm/llvm_version.h>
#include <refsi_m1/refsi_mux_builtin_info.h>

#include <cstdint>

#include "device/dma_regs.h"

using namespace llvm;
using namespace refsi_m1;

// Map Mux DMA builtin names to RefSi builtin names.
static StringRef getRefSiBuiltinName(StringRef MuxName) {
  return StringSwitch<StringRef>(MuxName)
      .Case(compiler::utils::MuxBuiltins::dma_wait, "__refsi_dma_wait")
      .Case(compiler::utils::MuxBuiltins::dma_read_1d,
            "__refsi_dma_start_seq_read")
      .Case(compiler::utils::MuxBuiltins::dma_write_1d,
            "__refsi_dma_start_seq_write")
      .Case(compiler::utils::MuxBuiltins::dma_read_2d,
            "__refsi_dma_start_2d_read")
      .Case(compiler::utils::MuxBuiltins::dma_write_2d,
            "__refsi_dma_start_2d_write")
      .Case(compiler::utils::MuxBuiltins::dma_read_3d,
            "__refsi_dma_start_3d_read")
      .Case(compiler::utils::MuxBuiltins::dma_write_3d,
            "__refsi_dma_start_3d_write")
      .Default(MuxName);
}

// Return the type used to interact with DMA registers.
static IntegerType *getDmaRegTy(LLVMContext &Ctx) {
  return IntegerType::getInt64Ty(Ctx);
}

// Return the type used to represent RefSi DMA transfer IDs.
static IntegerType *getTransferIDTy(Module &M) {
  return compiler::utils::getSizeType(M);
}

// Materialize the address of a DMA memory-mapped register in a basic block.
static Value *getDmaRegAddress(IRBuilder<> &B, unsigned RegIdx) {
  Type *const DmaRegTy = getDmaRegTy(B.getContext());
  Type *const DmaRegPtrTy = PointerType::get(DmaRegTy, 0);
  Value *const DmaRegAddr = ConstantInt::get(
      DmaRegTy, REFSI_DMA_REG_ADDR(REFSI_DMA_IO_ADDRESS, RegIdx));
  return B.CreateIntToPtr(DmaRegAddr, DmaRegPtrTy);
}

// Convert the value to a format that can be directly written to a DMA register.
static Value *getDmaRegVal(IRBuilder<> &B, Value *Val) {
  IntegerType *const DmaRegTy = getDmaRegTy(B.getContext());
  if (Val->getType()->isPointerTy()) {
    // Automatically cast pointers to an integer.
    return B.CreatePtrToInt(Val, DmaRegTy);
  }

  if (auto *const ValIntTy = dyn_cast<IntegerType>(Val->getType())) {
    if (ValIntTy->getBitWidth() != DmaRegTy->getBitWidth()) {
      // Automatically truncate or zero-extend integer values to fit the DMA
      // register width.
      return B.CreateZExtOrTrunc(Val, DmaRegTy);
    }
  }

  return Val;
}

// Write a value to the DMA register specified by the register index.
static Instruction *writeDmaReg(IRBuilder<> &B, unsigned RegIdx, Value *Val) {
  auto *RegAddr = getDmaRegAddress(B, RegIdx);
  Val = getDmaRegVal(B, Val);
  return B.CreateStore(Val, RegAddr, /* isVolatile */ true);
}

// Read a value from the DMA register specified by the register index.
static Value *readDmaReg(IRBuilder<> &B, unsigned RegIdx) {
  auto *const RegAddr = getDmaRegAddress(B, RegIdx);
  Type *const RegTy = getDmaRegTy(B.getContext());
  return B.CreateLoad(RegTy, RegAddr, /* isVolatile */ true);
}

static void startDmaTransfer1D(IRBuilder<> &B, Value *DstAddr, Value *SrcAddr,
                               Value *Size) {
  Type *const DmaRegTy = getDmaRegTy(B.getContext());

  // Set the destination address.
  writeDmaReg(B, REFSI_REG_DMADSTADDR, DstAddr);

  // Set the source address.
  writeDmaReg(B, REFSI_REG_DMASRCADDR, SrcAddr);

  // Set the transfer size.
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0, Size);  // Bytes

  // Configure and start a 1D DMA transfer.
  uint64_t Config = REFSI_DMA_1D | REFSI_DMA_STRIDE_NONE | REFSI_DMA_START;
  writeDmaReg(B, REFSI_REG_DMACTRL, ConstantInt::get(DmaRegTy, Config));
}

static void startDmaTransfer2D(IRBuilder<> &B, Value *DstAddr, Value *SrcAddr,
                               Value *Width, Value *Height, Value *DstStride,
                               Value *SrcStride, unsigned StrideMode) {
  // Set the destination address.
  writeDmaReg(B, REFSI_REG_DMADSTADDR, DstAddr);

  // Set the source address.
  writeDmaReg(B, REFSI_REG_DMASRCADDR, SrcAddr);

  // Set the transfer size for each dimension.
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0 + 0, Width);   // Bytes
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0 + 1, Height);  // Rows

  // Set the transfer stride.
  switch (StrideMode) {
    case REFSI_DMA_STRIDE_NONE:
      break;
    case REFSI_DMA_STRIDE_SRC:
      writeDmaReg(B, REFSI_REG_DMAXFERSRCSTRIDE0, SrcStride);  // Bytes
      break;
    case REFSI_DMA_STRIDE_DST:
      writeDmaReg(B, REFSI_REG_DMAXFERDSTSTRIDE0, DstStride);  // Bytes
      break;
    case REFSI_DMA_STRIDE_BOTH:
      writeDmaReg(B, REFSI_REG_DMAXFERSRCSTRIDE0, SrcStride);  // Bytes
      writeDmaReg(B, REFSI_REG_DMAXFERDSTSTRIDE0, DstStride);  // Bytes
      break;
  }

  // Configure and start a write or read 2D DMA transfer.
  uint64_t config = REFSI_DMA_2D | StrideMode | REFSI_DMA_START;
  writeDmaReg(B, REFSI_REG_DMACTRL, B.getInt64(config));
}

static void startDmaTransfer3D(IRBuilder<> &B, Value *DstAddr, Value *SrcAddr,
                               Value *Width, Value *Height, Value *Depth,
                               Value *LineStrideDst, Value *LineStrideSrc,
                               Value *PlaneStrideDst, Value *PlaneStrideSrc) {
  // Set the destination address.
  writeDmaReg(B, REFSI_REG_DMADSTADDR, DstAddr);

  // Set the source address.
  writeDmaReg(B, REFSI_REG_DMASRCADDR, SrcAddr);

  // Set the transfer size for each dimension.
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0 + 0, Width);   // Bytes
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0 + 1, Height);  // Rows
  writeDmaReg(B, REFSI_REG_DMAXFERSIZE0 + 2, Depth);   // Planes

  // Set the transfer strides.
  writeDmaReg(B, REFSI_REG_DMAXFERSRCSTRIDE0, LineStrideSrc);  // Bytes
  writeDmaReg(B, REFSI_REG_DMAXFERSRCSTRIDE0 + 1, PlaneStrideSrc);
  writeDmaReg(B, REFSI_REG_DMAXFERDSTSTRIDE0, LineStrideDst);  // Bytes
  writeDmaReg(B, REFSI_REG_DMAXFERDSTSTRIDE0 + 1, PlaneStrideDst);

  // Configure and start a 3D DMA transfer.
  uint64_t Config = REFSI_DMA_3D | REFSI_DMA_STRIDE_BOTH | REFSI_DMA_START;
  writeDmaReg(B, REFSI_REG_DMACTRL, B.getInt64(Config));
}

static void fetchAndReturnLastTransferID(IRBuilder<> &B, Function &F) {
  // Retrieve the transfer ID and convert it to an event.
  auto *XferId = readDmaReg(B, REFSI_REG_DMASTARTSEQ);
  assert(F.getReturnType()->isIntegerTy() &&
         "Event target types should have been replaced with i32s");
  B.CreateRet(B.CreateZExtOrTrunc(XferId, F.getReturnType()));
}

static void defineRefSiDma1D(Function &F,
                             compiler::utils::BIMuxInfoConcept &BI) {
  Argument *const ArgDstDmaPointer = F.getArg(0);
  Argument *const ArgSrcDmaPointer = F.getArg(1);
  Argument *const ArgWidth = F.getArg(2);
  Argument *const ArgEvent = F.getArg(3);

  auto &M = *F.getParent();
  auto &Ctx = F.getContext();

  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);
  auto *const BodyBB = BasicBlock::Create(Ctx, "body", &F);
  auto *const EpilogBB = BasicBlock::Create(Ctx, "epilog", &F);
  auto *const MuxGetLocalIdFn =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalId, M);
  compiler::utils::buildThreadCheck(EntryBB, BodyBB, EpilogBB,
                                    *MuxGetLocalIdFn);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  IRBuilder<> BodyBuilder(BodyBB);
  startDmaTransfer1D(BodyBuilder, ArgDstDmaPointer, ArgSrcDmaPointer, ArgWidth);
  BodyBuilder.CreateBr(EpilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  IRBuilder<> epilogBuilder(EpilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, F);

  // TODO: DDK-42 Handle the case where argEvent is non-zero.
  (void)ArgEvent;
}

void defineRefSiDma2D(Function &F, compiler::utils::BIMuxInfoConcept &BI) {
  Argument *const ArgDstDmaPointer = F.getArg(0);
  Argument *const ArgSrcDmaPointer = F.getArg(1);
  Argument *const ArgWidth = F.getArg(2);
  Argument *const ArgDstStride = F.getArg(3);
  Argument *const ArgSrcStride = F.getArg(4);
  Argument *const ArgHeight = F.getArg(5);
  Argument *const ArgEvent = F.getArg(6);

  auto &M = *F.getParent();
  auto &Ctx = F.getContext();

  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);
  auto *const BodyBB = BasicBlock::Create(Ctx, "body", &F);
  auto *const EpilogBB = BasicBlock::Create(Ctx, "epilog", &F);

  auto *const MuxGetLocalIdFn =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalId, M);
  compiler::utils::buildThreadCheck(EntryBB, BodyBB, EpilogBB,
                                    *MuxGetLocalIdFn);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  IRBuilder<> BodyBuilder(BodyBB);
  startDmaTransfer2D(BodyBuilder, ArgDstDmaPointer, ArgSrcDmaPointer, ArgWidth,
                     ArgHeight, ArgDstStride, ArgSrcStride,
                     REFSI_DMA_STRIDE_BOTH);
  BodyBuilder.CreateBr(EpilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  IRBuilder<> epilogBuilder(EpilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, F);

  // TODO: DDK-42 Handle the case where argEvent is non-zero.
  (void)ArgEvent;
}

void defineRefSiDma3D(Function &F, compiler::utils::BIMuxInfoConcept &BI) {
  Argument *const ArgDstDmaPointer = F.getArg(0);
  Argument *const ArgSrcDmaPointer = F.getArg(1);
  Argument *const ArgWidth = F.getArg(2);
  Argument *const ArgDstLineStride = F.getArg(3);
  Argument *const ArgSrcLineStride = F.getArg(4);
  Argument *const ArgHeight = F.getArg(5);
  Argument *const ArgDstPlaneStride = F.getArg(6);
  Argument *const ArgSrcPlaneStride = F.getArg(7);
  Argument *const ArgNumPlanes = F.getArg(8);
  Argument *const ArgEvent = F.getArg(9);

  auto &M = *F.getParent();
  auto &Ctx = F.getContext();

  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);
  auto *const BodyBB = BasicBlock::Create(Ctx, "body", &F);
  auto *const EpilogBB = BasicBlock::Create(Ctx, "epilog", &F);
  auto *const MuxGetLocalIdFn =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalId, M);
  compiler::utils::buildThreadCheck(EntryBB, BodyBB, EpilogBB,
                                    *MuxGetLocalIdFn);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  IRBuilder<> BodyBuilder(BodyBB);
  startDmaTransfer3D(BodyBuilder, ArgDstDmaPointer, ArgSrcDmaPointer, ArgWidth,
                     ArgHeight, ArgNumPlanes, ArgDstLineStride,
                     ArgSrcLineStride, ArgDstPlaneStride, ArgSrcPlaneStride);
  BodyBuilder.CreateBr(EpilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  IRBuilder<> epilogBuilder(EpilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, F);

  // TODO: DDK-42 Handle the case where argEvent is non-zero.
  (void)ArgEvent;
}

void defineRefSiDmaWait(Function &F) {
  Argument *const NumEvents = F.getArg(0);
  Argument *const EventList = F.getArg(1);

  auto &M = *F.getParent();
  auto &Ctx = F.getContext();
  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);
  auto *const BodyBB = BasicBlock::Create(Ctx, "body", &F);
  auto *const EpilogBB = BasicBlock::Create(Ctx, "epilog", &F);

  auto *const XferIdTy = getTransferIDTy(M);
  auto *const I32Ty = IntegerType::getInt32Ty(Ctx);
  auto *const Zero = ConstantInt::get(I32Ty, 0);
  auto *const One = ConstantInt::get(I32Ty, 1);

  auto *const ZeroEventIdTy = ConstantInt::get(XferIdTy, 0);

  // Build the entry of the DMA builtin. This either branches to the body (if
  // there is at least one event in the list) or the epilog (empty list).
  {
    IRBuilder<> EntryBuilder(EntryBB);
    assert(NumEvents->getType() == IntegerType::getInt32Ty(Ctx));
    auto *EmptyListCond = EntryBuilder.CreateICmpEQ(NumEvents, Zero);
    EntryBuilder.CreateCondBr(EmptyListCond, EpilogBB, BodyBB);
  }

  // Build the body of the DMA builtin. This computes the maximum transfer ID of
  // all the events in the event list.
  Value *MaxXferId;
  {
    IRBuilder<> BodyBuilder(BodyBB);

    auto *LoopIVPhi = BodyBuilder.CreatePHI(I32Ty, 2, "loop_iv");
    LoopIVPhi->addIncoming(Zero, EntryBB);

    auto *MaxXferIdPhi = BodyBuilder.CreatePHI(XferIdTy, 2, "max_xfer_id");
    MaxXferIdPhi->addIncoming(ZeroEventIdTy, EntryBB);

    // Retrieve the n-th event from the list.
    auto *EventTy = getTransferIDTy(M);
    auto *EventGep = BodyBuilder.CreateGEP(EventTy, EventList, LoopIVPhi);
    auto *EventID = BodyBuilder.CreateLoad(EventTy, EventGep, "xfer_id");
    auto *NewIV = BodyBuilder.CreateAdd(LoopIVPhi, One, "new_iv");

    // Find the higher value between the current maximum and n-th event ID.
    auto *NewMaxCond = BodyBuilder.CreateICmpUGT(EventID, MaxXferIdPhi);
    MaxXferId = BodyBuilder.CreateSelect(NewMaxCond, EventID, MaxXferIdPhi,
                                         "new_max_xfer_id");

    // Branch back to the loop body if there are more events to process.
    LoopIVPhi->addIncoming(NewIV, BodyBB);
    MaxXferIdPhi->addIncoming(MaxXferId, BodyBB);
    auto *const ExitCond =
        BodyBuilder.CreateICmpULT(NewIV, NumEvents, "exit_cond");
    BodyBuilder.CreateCondBr(ExitCond, BodyBB, EpilogBB);
  }

  // Build the epilog of the DMA builtin. This waits for all the DMA transfers
  // specified in the list to be finished.
  {
    IRBuilder<> epilogBuilder(EpilogBB);
    PHINode *eventIdToWait =
        epilogBuilder.CreatePHI(XferIdTy, 2, "event_id_to_wait");
    eventIdToWait->addIncoming(ZeroEventIdTy, EntryBB);
    eventIdToWait->addIncoming(MaxXferId, BodyBB);
    writeDmaReg(epilogBuilder, REFSI_REG_DMADONESEQ, eventIdToWait);
    epilogBuilder.CreateRetVoid();
  }
}

Function *RefSiM1BIMuxInfo::defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                             Module &M,
                                             ArrayRef<Type *> OverloadInfo) {
  assert(compiler::utils::BuiltinInfo::isMuxBuiltinID(ID) &&
         "Only handling mux builtins");
  auto FnName =
      compiler::utils::BuiltinInfo::getMuxBuiltinName(ID, OverloadInfo);
  Function *F = M.getFunction(FnName);

  // FIXME: We'd ideally want to declare it here to reduce pass
  // inter-dependencies.
  assert(F && "Function should have been pre-declared");
  if (!F->isDeclaration()) {
    return F;
  }

  switch (ID) {
    default:
      return compiler::utils::BIMuxInfoConcept::defineMuxBuiltin(ID, M,
                                                                 OverloadInfo);
    case compiler::utils::eMuxBuiltinDMARead1D:
    case compiler::utils::eMuxBuiltinDMAWrite1D:
      defineRefSiDma1D(*F, *this);
      break;
    case compiler::utils::eMuxBuiltinDMARead2D:
    case compiler::utils::eMuxBuiltinDMAWrite2D:
      defineRefSiDma2D(*F, *this);
      break;
    case compiler::utils::eMuxBuiltinDMARead3D:
    case compiler::utils::eMuxBuiltinDMAWrite3D:
      defineRefSiDma3D(*F, *this);
      break;
    case compiler::utils::eMuxBuiltinDMAWait:
      defineRefSiDmaWait(*F);
      break;
  }

  // Prevent the DMA builtin from being inlined, to make it clear from looking
  // at the kernel assembly how DMA is implemented.
  if (F->hasFnAttribute(Attribute::AlwaysInline)) {
    F->removeFnAttr(Attribute::AlwaysInline);
  }
  F->addFnAttr(Attribute::NoInline);
  F->setName(getRefSiBuiltinName(FnName));

  return F;
}
