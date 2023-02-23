// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// LLVM DMA pass utility functions.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_DMA_H_INCLUDED
#define COMPILER_UTILS_DMA_H_INCLUDED

#include <llvm/ADT/Twine.h>
#include <llvm/IR/IRBuilder.h>

#include <functional>

namespace llvm {
class BasicBlock;
class Module;
class Value;
}  // namespace llvm

namespace compiler {
namespace utils {

class BuiltinInfo;

/// @addtogroup utils
/// @{

/// @brief Helper function to check the local ID of the current thread.
///
/// @param[in] bb Basic block to generate the check in.
/// @param[in] x The local id in the x dimension to compare against.
/// @param[in] y The local id in the y dimension to compare against.
/// @param[in] z The local id in the z dimension to compare against.
/// @param[in] BI BuiltinInfo used to get/declare a builtin to get the local
/// work-item ID
///
/// @return A true Value if the local ID equals that passed via the index
/// arguments, false otherwise.
llvm::Value *isThreadEQ(llvm::BasicBlock *bb, unsigned x, unsigned y,
                        unsigned z, compiler::utils::BuiltinInfo &BI);

/// @brief Helper function to check if the local ID of the current thread is {0,
/// 0, 0}.
///
/// @param[in] bb Basic block to generate the check in.
/// @param[in] BI BuiltinInfo used to get/declare a builtin to get the local
/// work-item ID
///
/// @return A true Value if the local ID is {0, 0, 0} / false otherwise.
llvm::Value *isThreadZero(llvm::BasicBlock *bb,
                          compiler::utils::BuiltinInfo &BI);

/// @brief Insert 'thread-checking' logic in the entry block, so that control
/// branches to the 'true' block when the current work-item in the first in the
/// work-group (e.g. ID zero in all dimensions) or to the 'false' block for
/// other work-items
///
/// @param[in] entryBlock Block to insert the 'thread-checking' logic
/// @param[in] trueBlock Block to execute only on the first work-item
/// @param[in] falseBlock Block to execute on all other work-items
/// @param[in] BI BuiltinInfo used to get/declare a builtin to get the local
/// work-item ID
void buildThreadCheck(llvm::BasicBlock *entryBlock, llvm::BasicBlock *trueBlock,
                      llvm::BasicBlock *falseBlock,
                      compiler::utils::BuiltinInfo &BI);

/// @brief Gets or creates the __mux_dma_event_t type.
///
/// This type may be declared by other passes hence we "get or create it".
///
/// @param[in] m LLVM Module to get or create the type in.
///
/// @return The opaque struct declaration of the __mux_dma_event_t type.
llvm::StructType *getOrCreateMuxDMAEventType(llvm::Module &m);

/// @}
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_DMA_H_INCLUDED
