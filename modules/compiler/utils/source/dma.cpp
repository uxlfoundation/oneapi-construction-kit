// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/dma.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/multi_llvm.h>

#include <array>

namespace compiler {
namespace utils {

llvm::Value *isThreadEQ(llvm::BasicBlock *bb, unsigned x, unsigned y,
                        unsigned z, llvm::Function &LocalIDFn) {
  llvm::IRBuilder<> builder(bb);
  LocalIDFn.setCallingConv(llvm::CallingConv::SPIR_FUNC);
  auto *const indexType = LocalIDFn.arg_begin()->getType();
  llvm::Value *result = llvm::ConstantInt::getTrue(bb->getContext());

  const std::array<unsigned, 3> threadIDs{x, y, z};
  for (unsigned i = 0; i < threadIDs.size(); ++i) {
    auto *const index = llvm::ConstantInt::get(indexType, i);
    auto *const localID = builder.CreateCall(&LocalIDFn, index);
    localID->setCallingConv(LocalIDFn.getCallingConv());

    auto *thread =
        llvm::ConstantInt::get(LocalIDFn.getReturnType(), threadIDs[i]);
    auto *const cmp = builder.CreateICmpEQ(localID, thread);
    result = (i == 0) ? cmp : builder.CreateAnd(result, cmp);
  }

  return result;
}

llvm::Value *isThreadZero(llvm::BasicBlock *BB, llvm::Function &LocalIDFn) {
  return isThreadEQ(BB, 0, 0, 0, LocalIDFn);
}

void buildThreadCheck(llvm::BasicBlock *entryBlock, llvm::BasicBlock *trueBlock,
                      llvm::BasicBlock *falseBlock, llvm::Function &LocalIDFn) {
  // only thread 0 in the work group should execute the DMA.
  llvm::IRBuilder<> entryBuilder(entryBlock);
  entryBuilder.CreateCondBr(isThreadZero(entryBlock, LocalIDFn), trueBlock,
                            falseBlock);
}

llvm::StructType *getOrCreateMuxDMAEventType(llvm::Module &m) {
  if (auto *eventType =
          multi_llvm::getStructTypeByName(m, MuxBuiltins::dma_event_type)) {
    return eventType;
  }

  return llvm::StructType::create(m.getContext(), MuxBuiltins::dma_event_type);
}
}  // namespace utils
}  // namespace compiler
