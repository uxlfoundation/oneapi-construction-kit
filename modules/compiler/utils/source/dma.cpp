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
                        unsigned z, BuiltinInfo &BI) {
  llvm::IRBuilder<> builder(bb);
  auto *const getLocalID = BI.getOrDeclareMuxBuiltin(
      eMuxBuiltinGetLocalId, *bb->getParent()->getParent());
  getLocalID->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  auto *const indexType = getLocalID->arg_begin()->getType();
  llvm::Value *result = llvm::ConstantInt::getTrue(bb->getContext());

  const std::array<unsigned, 3> threadIDs{x, y, z};
  for (unsigned i = 0; i < threadIDs.size(); ++i) {
    auto *const index = llvm::ConstantInt::get(indexType, i);
    auto *const localID = builder.CreateCall(getLocalID, index);
    localID->setCallingConv(getLocalID->getCallingConv());

    auto *thread =
        llvm::ConstantInt::get(getLocalID->getReturnType(), threadIDs[i]);
    auto *const cmp = builder.CreateICmpEQ(localID, thread);
    result = (i == 0) ? cmp : builder.CreateAnd(result, cmp);
  }

  return result;
}

llvm::Value *isThreadZero(llvm::BasicBlock *BB, BuiltinInfo &BI) {
  return isThreadEQ(BB, 0, 0, 0, BI);
}

void buildThreadCheck(llvm::BasicBlock *entryBlock, llvm::BasicBlock *trueBlock,
                      llvm::BasicBlock *falseBlock, BuiltinInfo &BI) {
  // only thread 0 in the work group should execute the DMA.
  llvm::IRBuilder<> entryBuilder(entryBlock);
  entryBuilder.CreateCondBr(isThreadZero(entryBlock, BI), trueBlock,
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
