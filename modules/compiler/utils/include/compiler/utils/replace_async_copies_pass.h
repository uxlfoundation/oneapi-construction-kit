// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace async copies pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that defines the async copy builtins of OpenCL C in terms of
/// __mux builtins. Declarations for the mux builtins are introduced by the
/// pass as they are required and it is expected that Mux targets implement
/// these builtins using hardware specific DMA functionality.
class ReplaceAsyncCopiesPass final
    : public llvm::PassInfoMixin<ReplaceAsyncCopiesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED
