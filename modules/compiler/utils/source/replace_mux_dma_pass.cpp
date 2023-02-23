// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/dma.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/replace_mux_dma_pass.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <multi_llvm/opaque_pointers.h>

#define DEBUG_TYPE "replace-mux-dma"

namespace {

llvm::BasicBlock *copy1D(llvm::Module &module, llvm::BasicBlock &parentBlock,
                         llvm::Value *dstPtr, llvm::Value *srcPtr,
                         llvm::Value *numBytes) {
  const llvm::DataLayout &dataLayout = module.getDataLayout();
  llvm::Type *i8Ty = llvm::IntegerType::get(module.getContext(), 8);
  llvm::Type *sizeTy = llvm::IntegerType::get(
      module.getContext(), dataLayout.getPointerSizeInBits(0));

  assert(srcPtr->getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(srcPtr->getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");
  assert(dstPtr->getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(dstPtr->getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");

  llvm::Value *dmaIVs[] = {srcPtr, dstPtr};

  // This is a simple loop copy a byte at a time from srcPtr to dstPtr.
  llvm::BasicBlock *exitBlockX = compiler::utils::createLoop(
      &parentBlock, nullptr, llvm::ConstantInt::get(sizeTy, 0), numBytes,
      dmaIVs, compiler::utils::CreateLoopOpts{},
      [&](llvm::BasicBlock *block, llvm::Value *x,
          llvm::ArrayRef<llvm::Value *> ivsCurr,
          llvm::MutableArrayRef<llvm::Value *> ivsNext) {
        llvm::IRBuilder<> ir(block);
        llvm::Value *const currentDmaSrcPtr1DPhi = ivsCurr[0];
        llvm::Value *const currentDmaDstPtr1DPhi = ivsCurr[1];
        llvm::Value *load = ir.CreateLoad(i8Ty, currentDmaSrcPtr1DPhi);
        ir.CreateStore(load, currentDmaDstPtr1DPhi);
        ivsNext[0] = ir.CreateGEP(i8Ty, currentDmaSrcPtr1DPhi,
                                  llvm::ConstantInt::get(x->getType(), 1));
        ivsNext[1] = ir.CreateGEP(i8Ty, currentDmaDstPtr1DPhi,
                                  llvm::ConstantInt::get(x->getType(), 1));
        return block;
      });
  return exitBlockX;
}

llvm::BasicBlock *copy2D(llvm::Module &module, llvm::BasicBlock &parentBlock,
                         llvm::Value *dstPtr, llvm::Value *srcPtr,
                         llvm::Value *lineSizeBytes, llvm::Value *lineStrideDst,
                         llvm::Value *lineStrideSrc, llvm::Value *numLines) {
  const llvm::DataLayout &dataLayout = module.getDataLayout();
  llvm::Type *i8Ty = llvm::IntegerType::get(module.getContext(), 8);
  llvm::Type *sizeTy = llvm::IntegerType::get(
      module.getContext(), dataLayout.getPointerSizeInBits(0));

  assert(srcPtr->getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(srcPtr->getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");
  assert(dstPtr->getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(dstPtr->getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");

  llvm::Value *dmaIVs[] = {srcPtr, dstPtr};

  // This is a loop over the range of lines, calling a 1D copy on each line
  llvm::BasicBlock *exitBlock = compiler::utils::createLoop(
      &parentBlock, nullptr, llvm::ConstantInt::get(sizeTy, 0), numLines,
      dmaIVs, compiler::utils::CreateLoopOpts{},
      [&](llvm::BasicBlock *block, llvm::Value *,
          llvm::ArrayRef<llvm::Value *> ivsCurr,
          llvm::MutableArrayRef<llvm::Value *> ivsNext) {
        llvm::IRBuilder<> loopIr(block);
        llvm::Value *currentDmaSrcPtrPhi = ivsCurr[0];
        llvm::Value *currentDmaDstPtrPhi = ivsCurr[1];

        ivsNext[0] = loopIr.CreateGEP(i8Ty, currentDmaSrcPtrPhi, lineStrideSrc);
        ivsNext[1] = loopIr.CreateGEP(i8Ty, currentDmaDstPtrPhi, lineStrideDst);
        return copy1D(module, *block, currentDmaDstPtrPhi, currentDmaSrcPtrPhi,
                      lineSizeBytes);
      });

  return exitBlock;
}

void dma1D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argWidth = *argIter++;
  llvm::Argument &argEvent = *argIter++;

  auto &context = func->getContext();
  auto *exitBlock = llvm::BasicBlock::Create(context, "exit", func);
  auto *loopEntryBlock =
      llvm::BasicBlock::Create(context, "loop_entry", func, exitBlock);
  auto *entryBlock = llvm::BasicBlock::Create(func->getContext(), "entry", func,
                                              loopEntryBlock);

  compiler::utils::buildThreadCheck(entryBlock, loopEntryBlock, exitBlock, BI);

  llvm::BasicBlock *loopExitBlock =
      copy1D(*func->getParent(), *loopEntryBlock, &argDstDmaPointer,
             &argSrcDmaPointer, &argWidth);
  llvm::IRBuilder<> loopExitBuilder(loopExitBlock);
  loopExitBuilder.CreateBr(exitBlock);

  llvm::IRBuilder<> exitBuilder(exitBlock);
  exitBuilder.CreateRet(&argEvent);
}

void dma2D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argWidth = *argIter++;
  llvm::Argument &argDstStride = *argIter++;
  llvm::Argument &argSrcStride = *argIter++;
  llvm::Argument &argNumLines = *argIter++;
  llvm::Argument &argEvent = *argIter++;
  auto &context = func->getContext();

  auto *exitBlock = llvm::BasicBlock::Create(context, "exit", func);
  auto *loopEntryBlock =
      llvm::BasicBlock::Create(context, "loop_entry", func, exitBlock);
  auto *entryBlock = llvm::BasicBlock::Create(func->getContext(), "entry", func,
                                              loopEntryBlock);

  compiler::utils::buildThreadCheck(entryBlock, loopEntryBlock, exitBlock, BI);

  // Create a loop around 1D Dma read, adding strides each time.
  llvm::BasicBlock *loopExitBlock = copy2D(
      *func->getParent(), *loopEntryBlock, &argDstDmaPointer, &argSrcDmaPointer,
      &argWidth, &argDstStride, &argSrcStride, &argNumLines);

  llvm::IRBuilder<> loopExitBuilder(loopExitBlock);
  loopExitBuilder.CreateBr(exitBlock);

  llvm::IRBuilder<> exitBuilder(exitBlock);
  exitBuilder.CreateRet(&argEvent);
}

void dma3D(llvm::Function *func, compiler::utils::BuiltinInfo &BI) {
  llvm::Function::arg_iterator argIter = func->arg_begin();

  llvm::Argument &argDstDmaPointer = *argIter++;
  llvm::Argument &argSrcDmaPointer = *argIter++;
  llvm::Argument &argLineSize = *argIter++;
  llvm::Argument &argDstLineStride = *argIter++;
  llvm::Argument &argSrcLineStride = *argIter++;
  llvm::Argument &argNumLinesPerPlane = *argIter++;
  llvm::Argument &argDstPlaneStride = *argIter++;
  llvm::Argument &argSrcPlaneStride = *argIter++;
  llvm::Argument &argNumPlanes = *argIter++;
  llvm::Argument &argEvent = *argIter++;
  auto &module = *func->getParent();
  const llvm::DataLayout &dataLayout = module.getDataLayout();
  auto &context = func->getContext();
  llvm::Type *i8Ty = llvm::IntegerType::get(context, 8);
  llvm::Type *sizeTy =
      llvm::IntegerType::get(context, dataLayout.getPointerSizeInBits(0));

  auto *exitBlock = llvm::BasicBlock::Create(context, "exit", func);
  auto *loopEntryBlock =
      llvm::BasicBlock::Create(context, "loop_entry", func, exitBlock);
  auto *entryBlock = llvm::BasicBlock::Create(func->getContext(), "entry", func,
                                              loopEntryBlock);

  compiler::utils::buildThreadCheck(entryBlock, loopEntryBlock, exitBlock, BI);

  assert(argSrcDmaPointer.getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(argSrcDmaPointer.getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");
  assert(argDstDmaPointer.getType()->isPointerTy() &&
         multi_llvm::isOpaqueOrPointeeTypeMatches(
             llvm::cast<llvm::PointerType>(argDstDmaPointer.getType()), i8Ty) &&
         "Mux DMA builtins are always byte-accessed");

  llvm::Value *dmaIVs[] = {&argSrcDmaPointer, &argDstDmaPointer};

  // Create a loop around 1D Dma read, adding stride, local width each time.
  llvm::BasicBlock *loopExitBlock = compiler::utils::createLoop(
      loopEntryBlock, nullptr, llvm::ConstantInt::get(sizeTy, 0), &argNumPlanes,
      dmaIVs, compiler::utils::CreateLoopOpts{},
      [&](llvm::BasicBlock *block, llvm::Value *,
          llvm::ArrayRef<llvm::Value *> ivsCurr,
          llvm::MutableArrayRef<llvm::Value *> ivsNext) {
        llvm::IRBuilder<> loopIr(block);
        llvm::Value *currentDmaPlaneSrcPtrPhi = ivsCurr[0];
        llvm::Value *currentDmaPlaneDstPtrPhi = ivsCurr[1];

        ivsNext[0] = loopIr.CreateGEP(i8Ty, currentDmaPlaneSrcPtrPhi,
                                      &argSrcPlaneStride);
        ivsNext[1] = loopIr.CreateGEP(i8Ty, currentDmaPlaneDstPtrPhi,
                                      &argDstPlaneStride);

        return copy2D(*func->getParent(), *block, currentDmaPlaneDstPtrPhi,
                      currentDmaPlaneSrcPtrPhi, &argLineSize, &argDstLineStride,
                      &argSrcLineStride, &argNumLinesPerPlane);
      });

  llvm::IRBuilder<> loopExitBuilder(loopExitBlock);
  loopExitBuilder.CreateBr(exitBlock);

  llvm::IRBuilder<> exitBuilder(exitBlock);
  exitBuilder.CreateRet(&argEvent);
}

}  // namespace

llvm::PreservedAnalyses compiler::utils::ReplaceMuxDmaPass::run(
    llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  bool Changed = false;
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_read_1d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 1d read dma\n");
      dma1D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_write_1d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 1d write dma\n");
      dma1D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_read_2d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 2d read dma\n");
      dma2D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_write_2d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 2d write dma\n");
      dma2D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_read_3d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 3d read dma\n");
      dma3D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_write_3d)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;
      LLVM_DEBUG(llvm::dbgs() << "Replacing mux 3d write dma\n");
      dma3D(func, BI);
    }
  }
  if (llvm::Function *const func = M.getFunction(MuxBuiltins::dma_wait)) {
    // This pass may be run multiple times - make sure we don't define these
    // builtins twice.
    if (func->getBasicBlockList().empty()) {
      Changed = true;

      // create an IR builder with a single basic block in our function
      llvm::IRBuilder<> ir(
          llvm::BasicBlock::Create(func->getContext(), "", func));

      ir.CreateRetVoid();
    }
  }

  return Changed ? llvm::PreservedAnalyses::none()
                 : llvm::PreservedAnalyses::all();
}
