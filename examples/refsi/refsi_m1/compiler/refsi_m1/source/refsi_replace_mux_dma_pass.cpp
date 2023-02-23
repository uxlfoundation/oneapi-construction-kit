// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/dma.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/opaque_pointers.h>
#include <refsi_m1/refsi_replace_mux_dma_pass.h>

#include <functional>

#include "device/dma_regs.h"

// Return the type used to interact with DMA registers.
static llvm::IntegerType *getDmaRegTy(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt64Ty(ctx);
}

// Return the type used to represent RefSi DMA transfer IDs.
static llvm::IntegerType *getTransferIDTy(llvm::LLVMContext &ctx) {
  return llvm::IntegerType::getInt32Ty(ctx);
}

// Materialize the address of a DMA memory-mapped register in a basic block.
static llvm::Value *getDmaRegAddress(llvm::IRBuilder<> &builder,
                                     unsigned regIdx) {
  llvm::Type *dmaRegTy = getDmaRegTy(builder.getContext());
  llvm::Type *dmaRegPtrTy = llvm::PointerType::get(dmaRegTy, 0);
  llvm::Value *dmaRegAddr = llvm::ConstantInt::get(
      dmaRegTy, REFSI_DMA_REG_ADDR(REFSI_DMA_IO_ADDRESS, regIdx));
  return builder.CreateIntToPtr(dmaRegAddr, dmaRegPtrTy);
}

// Convert the value to a format that can be directly written to a DMA register.
static llvm::Value *getDmaRegVal(llvm::IRBuilder<> &builder, llvm::Value *val) {
  llvm::IntegerType *dmaRegTy = getDmaRegTy(builder.getContext());
  if (val->getType()->isPointerTy()) {
    // Automatically cast pointers to an integer.
    return builder.CreatePtrToInt(val, dmaRegTy);
  } else if (llvm::IntegerType *valIntTy =
                 llvm::dyn_cast<llvm::IntegerType>(val->getType())) {
    if (valIntTy->getBitWidth() != dmaRegTy->getBitWidth()) {
      // Automatically truncate or zero-extend integer values to fit the DMA
      // register width.
      return builder.CreateZExtOrTrunc(val, dmaRegTy);
    }
  }
  return val;
}

// Write a value to the DMA register specified by the register index.
static llvm::Instruction *writeDmaReg(llvm::IRBuilder<> &builder,
                                      unsigned regIdx, llvm::Value *val) {
  auto *regAddr = getDmaRegAddress(builder, regIdx);
  val = getDmaRegVal(builder, val);
  return builder.CreateStore(val, regAddr, /* isVolatile */ true);
}

// Read a value from the DMA register specified by the register index.
static llvm::Value *readDmaReg(llvm::IRBuilder<> &builder, unsigned regIdx) {
  auto *regAddr = getDmaRegAddress(builder, regIdx);
  llvm::Type *regTy = getDmaRegTy(builder.getContext());
  return builder.CreateLoad(regTy, regAddr, /* isVolatile */ true);
}

static void startDmaTransfer1D(llvm::IRBuilder<> &builder, llvm::Value *dstAddr,
                               llvm::Value *srcAddr, llvm::Value *size) {
  llvm::Type *dmaRegTy = getDmaRegTy(builder.getContext());

  // Set the destination address.
  writeDmaReg(builder, REFSI_REG_DMADSTADDR, dstAddr);

  // Set the source address.
  writeDmaReg(builder, REFSI_REG_DMASRCADDR, srcAddr);

  // Set the transfer size.
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0, size);  // Bytes

  // Configure and start a 1D DMA transfer.
  uint64_t config = REFSI_DMA_1D | REFSI_DMA_STRIDE_NONE | REFSI_DMA_START;
  writeDmaReg(builder, REFSI_REG_DMACTRL,
              llvm::ConstantInt::get(dmaRegTy, config));
}

static void startDmaTransfer2D(llvm::IRBuilder<> &builder, llvm::Value *dstAddr,
                               llvm::Value *srcAddr, llvm::Value *width,
                               llvm::Value *height, llvm::Value *dstStride,
                               llvm::Value *srcStride, unsigned strideMode) {
  // Set the destination address.
  writeDmaReg(builder, REFSI_REG_DMADSTADDR, dstAddr);

  // Set the source address.
  writeDmaReg(builder, REFSI_REG_DMASRCADDR, srcAddr);

  // Set the transfer size for each dimension.
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0 + 0, width);   // Bytes
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0 + 1, height);  // Rows

  // Set the transfer stride.
  switch (strideMode) {
    case REFSI_DMA_STRIDE_NONE:
      break;
    case REFSI_DMA_STRIDE_SRC:
      writeDmaReg(builder, REFSI_REG_DMAXFERSRCSTRIDE0, srcStride);  // Bytes
      break;
    case REFSI_DMA_STRIDE_DST:
      writeDmaReg(builder, REFSI_REG_DMAXFERDSTSTRIDE0, dstStride);  // Bytes
      break;
    case REFSI_DMA_STRIDE_BOTH:
      writeDmaReg(builder, REFSI_REG_DMAXFERSRCSTRIDE0, srcStride);  // Bytes
      writeDmaReg(builder, REFSI_REG_DMAXFERDSTSTRIDE0, dstStride);  // Bytes
      break;
  }

  // Configure and start a write or read 2D DMA transfer.
  uint64_t config = REFSI_DMA_2D | strideMode | REFSI_DMA_START;
  writeDmaReg(builder, REFSI_REG_DMACTRL, builder.getInt64(config));
}

static void startDmaTransfer3D(llvm::IRBuilder<> &builder, llvm::Value *dstAddr,
                               llvm::Value *srcAddr, llvm::Value *width,
                               llvm::Value *height, llvm::Value *depth,
                               llvm::Value *lineStrideDst,
                               llvm::Value *lineStrideSrc,
                               llvm::Value *planeStrideDst,
                               llvm::Value *planeStrideSrc) {
  // Set the destination address.
  writeDmaReg(builder, REFSI_REG_DMADSTADDR, dstAddr);

  // Set the source address.
  writeDmaReg(builder, REFSI_REG_DMASRCADDR, srcAddr);

  // Set the transfer size for each dimension.
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0 + 0, width);   // Bytes
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0 + 1, height);  // Rows
  writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0 + 2, depth);   // Planes

  // Set the transfer strides.
  writeDmaReg(builder, REFSI_REG_DMAXFERSRCSTRIDE0, lineStrideSrc);  // Bytes
  writeDmaReg(builder, REFSI_REG_DMAXFERSRCSTRIDE0 + 1, planeStrideSrc);
  writeDmaReg(builder, REFSI_REG_DMAXFERDSTSTRIDE0, lineStrideDst);  // Bytes
  writeDmaReg(builder, REFSI_REG_DMAXFERDSTSTRIDE0 + 1, planeStrideDst);

  // Configure and start a 3D DMA transfer.
  uint64_t config = REFSI_DMA_3D | REFSI_DMA_STRIDE_BOTH | REFSI_DMA_START;
  writeDmaReg(builder, REFSI_REG_DMACTRL, builder.getInt64(config));
}

static void fetchAndReturnLastTransferID(llvm::IRBuilder<> &builder,
                                         llvm::Function *func) {
  // Retrieve the transfer ID and convert it to an event.
  auto *xferId = readDmaReg(builder, REFSI_REG_DMASTARTSEQ);
  auto *event = builder.CreateIntToPtr(xferId, func->getReturnType());
  builder.CreateRet(event);
}

void dmaWait(llvm::Function *func, compiler::utils::BuiltinInfo &) {
  auto args = func->arg_begin();
  auto &context = func->getContext();
  llvm::Argument &numEvents = *args++;
  llvm::Argument &eventList = *args++;

  auto *entryBB = llvm::BasicBlock::Create(context, "entry", func);
  auto *bodyBB = llvm::BasicBlock::Create(context, "body", func);
  auto *epilogBB = llvm::BasicBlock::Create(context, "epilog", func);

  auto *xferIdTy = getTransferIDTy(func->getContext());
  auto *zero = llvm::ConstantInt::get(xferIdTy, 0);
  auto *one = llvm::ConstantInt::get(xferIdTy, 1);

  // Build the entry of the DMA builtin. This either branches to the body (if
  // there is at least one event in the list) or the epilog (empty list).
  {
    llvm::IRBuilder<> entryBuilder(entryBB);
    assert(zero->getType() == llvm::IntegerType::getInt32Ty(context));
    auto *emptyListCond = entryBuilder.CreateICmpEQ(&numEvents, zero);
    entryBuilder.CreateCondBr(emptyListCond, epilogBB, bodyBB);
  }

  // Build the body of the DMA builtin. This computes the maximum transfer ID of
  // all the events in the event list.
  llvm::Value *maxXferId;
  {
    llvm::IRBuilder<> bodyBuilder(bodyBB);

    auto *loopIVPhi = bodyBuilder.CreatePHI(xferIdTy, 2, "loop_iv");
    loopIVPhi->addIncoming(zero, entryBB);

    auto *maxXferIdPhi = bodyBuilder.CreatePHI(xferIdTy, 2, "max_xfer_id");
    maxXferIdPhi->addIncoming(zero, entryBB);

    // Retrieve the n-th event from the list.
    auto *coreDMAEventTy =
        compiler::utils::getOrCreateMuxDMAEventType(*func->getParent());
    auto *eventPtrTy = coreDMAEventTy->getPointerTo();
    assert(
        multi_llvm::isOpaqueOrPointeeTypeMatches(
            llvm::cast<llvm::PointerType>(eventList.getType()), eventPtrTy) &&
        "__mux_dma_wait() parameter expected to be __mux_dma_event_t**");
    auto *eventGep = bodyBuilder.CreateGEP(eventPtrTy, &eventList, loopIVPhi);
    auto *event = bodyBuilder.CreateLoad(eventPtrTy, eventGep);
    auto *eventID = bodyBuilder.CreatePtrToInt(event, xferIdTy, "xfer_id");
    auto *newIV = bodyBuilder.CreateAdd(loopIVPhi, one, "new_iv");

    // Find the higher value between the current maximum and n-th event ID.
    auto *newMaxCond = bodyBuilder.CreateICmpUGT(eventID, maxXferIdPhi);
    maxXferId = bodyBuilder.CreateSelect(newMaxCond, eventID, maxXferIdPhi,
                                         "new_max_xfer_id");

    // Branch back to the loop body if there are more events to process.
    loopIVPhi->addIncoming(newIV, bodyBB);
    maxXferIdPhi->addIncoming(maxXferId, bodyBB);
    auto *exitCond = bodyBuilder.CreateICmpULT(newIV, &numEvents, "exit_cond");
    bodyBuilder.CreateCondBr(exitCond, bodyBB, epilogBB);
  }

  // Build the epilog of the DMA builtin. This waits for all the DMA transfers
  // specified in the list to be finished.
  {
    llvm::IRBuilder<> epilogBuilder(epilogBB);
    llvm::PHINode *eventIdToWait =
        epilogBuilder.CreatePHI(xferIdTy, 2, "event_id_to_wait");
    eventIdToWait->addIncoming(zero, entryBB);
    eventIdToWait->addIncoming(maxXferId, bodyBB);
    writeDmaReg(epilogBuilder, REFSI_REG_DMADONESEQ, eventIdToWait);
    epilogBuilder.CreateRetVoid();
  }
}

void dma1D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argWidth = *argIter++;
  llvm::Argument &argEvent = *argIter++;

  auto &context = func->getContext();

  auto *entryBB = llvm::BasicBlock::Create(context, "entry", func);
  auto *bodyBB = llvm::BasicBlock::Create(context, "body", func);
  auto *epilogBB = llvm::BasicBlock::Create(context, "epilog", func);
  compiler::utils::buildThreadCheck(entryBB, bodyBB, epilogBB, BI);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  llvm::IRBuilder<> bodyBuilder(bodyBB);
  startDmaTransfer1D(bodyBuilder, &argDstDmaPointer, &argSrcDmaPointer,
                     &argWidth);
  bodyBuilder.CreateBr(epilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  llvm::IRBuilder<> epilogBuilder(epilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, func);

  // TODO: RVE-163 Handle the case where argEvent is non-zero.
  (void)argEvent;
}

void dma2D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argWidth = *argIter++;
  llvm::Argument &argDstStride = *argIter++;
  llvm::Argument &argSrcStride = *argIter++;
  llvm::Argument &argHeight = *argIter++;
  llvm::Argument &argEvent = *argIter++;

  auto &context = func->getContext();

  auto *entryBB = llvm::BasicBlock::Create(context, "entry", func);
  auto *bodyBB = llvm::BasicBlock::Create(context, "body", func);
  auto *epilogBB = llvm::BasicBlock::Create(context, "epilog", func);
  compiler::utils::buildThreadCheck(entryBB, bodyBB, epilogBB, BI);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  llvm::IRBuilder<> bodyBuilder(bodyBB);
  startDmaTransfer2D(bodyBuilder, &argDstDmaPointer, &argSrcDmaPointer,
                     &argWidth, &argHeight, &argDstStride, &argSrcStride,
                     REFSI_DMA_STRIDE_BOTH);
  bodyBuilder.CreateBr(epilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  llvm::IRBuilder<> epilogBuilder(epilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, func);

  // TODO: RVE-163 Handle the case where argEvent is non-zero.
  (void)argEvent;
}

void dma3D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argWidth = *argIter++;
  llvm::Argument &argDstLineStride = *argIter++;
  llvm::Argument &argSrcLineStride = *argIter++;
  llvm::Argument &argHeight = *argIter++;
  llvm::Argument &argDstPlaneStride = *argIter++;
  llvm::Argument &argSrcPlaneStride = *argIter++;
  llvm::Argument &argNumPlanes = *argIter++;
  llvm::Argument &argEvent = *argIter++;

  auto &context = func->getContext();

  auto *entryBB = llvm::BasicBlock::Create(context, "entry", func);
  auto *bodyBB = llvm::BasicBlock::Create(context, "body", func);
  auto *epilogBB = llvm::BasicBlock::Create(context, "epilog", func);
  compiler::utils::buildThreadCheck(entryBB, bodyBB, epilogBB, BI);

  // Build the body of the DMA builtin. This is only executed for one work-item
  // in the work-group.
  llvm::IRBuilder<> bodyBuilder(bodyBB);
  startDmaTransfer3D(bodyBuilder, &argDstDmaPointer, &argSrcDmaPointer,
                     &argWidth, &argHeight, &argNumPlanes, &argDstLineStride,
                     &argSrcLineStride, &argDstPlaneStride, &argSrcPlaneStride);
  bodyBuilder.CreateBr(epilogBB);

  // Build the epilog of the DMA builtin. This is executed for all work-items
  // in the work-group, not just the first item. Since each work-group is
  // executed by a single hart, the transfer ID returned by reading the
  // DMASTARTSEQ register after starting the DMA transfer is guaranteed to be
  // valid for that hart.
  llvm::IRBuilder<> epilogBuilder(epilogBB);
  fetchAndReturnLastTransferID(epilogBuilder, func);

  // TODO: RVE-163 Handle the case where argEvent is non-zero.
  (void)argEvent;
}

namespace {
// Map Mux DMA builtin names to RefSi builtin names.
llvm::StringRef getRefSiBuiltinName(llvm::StringRef MuxName) {
  return llvm::StringSwitch<llvm::StringRef>(MuxName)
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

void postProcessDmaBuiltin(llvm::Function *func) {
  // Prevent the DMA builtin from being inlined, to make it clear from looking
  // at the kernel assembly how DMA is implemented.
  func->addFnAttr(llvm::Attribute::NoInline);
  func->setName(getRefSiBuiltinName(func->getName()));
}

bool replaceDmaBuiltin(
    llvm::Module &module, llvm::StringRef name,
    std::function<void(llvm::Function *, compiler::utils::BuiltinInfo &)>
        handler,
    compiler::utils::BuiltinInfo &BI) {
  llvm::Function *func = module.getFunction(name);
  // This pass may be run multiple times - make sure we don't define these
  // builtins twice.
  if (!func || !func->getBasicBlockList().empty()) {
    return false;
  }
  handler(func, BI);
  postProcessDmaBuiltin(func);
  return true;
}

}  // namespace

namespace refsi_m1 {
llvm::PreservedAnalyses RefSiM1ReplaceMuxDmaPass::run(
    llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  bool modified = false;

  auto &BI = AM.getResult<compiler::utils::BuiltinInfoAnalysis>(M);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_read_1d,
                                dma1D, BI);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_write_1d,
                                dma1D, BI);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_read_2d,
                                dma2D, BI);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_write_2d,
                                dma2D, BI);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_read_3d,
                                dma3D, BI);
  modified |= replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_write_3d,
                                dma3D, BI);
  modified |=
      replaceDmaBuiltin(M, compiler::utils::MuxBuiltins::dma_wait, dmaWait, BI);

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}
}  // namespace refsi_m1
