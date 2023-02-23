// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// A pass that replaces calls to llvm.memcpy.*, llvm.memset.* and
/// llvm.memmove.* with calls to a generated loop.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that replaces calls to llvm.memcpy.*, llvm.memset.* and
/// llvm.memmove with calls to a generated loop.
/// @note This pass can be used for targets which are not able to generate
/// backend code for these intrinsics or do not link with a library which
/// supports this. Note that memove only supports same address spaces for the
/// pointer arguments
class ReplaceMemIntrinsicsPass final
    : public llvm::PassInfoMixin<ReplaceMemIntrinsicsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_MEM_INTRINSICS_PASS_H_INCLUDED
