// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace mux DMA util pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_MUX_DMA_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_MUX_DMA_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace llvm {
class ModulePass;
}

namespace compiler {
namespace utils {
/// @brief The replace mux DMA pass
///
/// This provides a default solution for replacing DMA mux intrinsics. These
/// DMA intrinsics are ``__mux_dma_read_1D`, `__mux_dma_write_1D`,
/// `__mux_dma_read_2D`, `__mux_dma_write_2D`, `__mux_dma_read_3D`,
/// `__mux_dma_write_3D`. These routines are not intended to be efficient for a
/// particular architecture and are really a placeholder for customers until
/// they are ready to replace these functions with DMA calls. They are
/// essentially a memcpy with `__mux_dma_wait` doing nothing.
class ReplaceMuxDmaPass final : public llvm::PassInfoMixin<ReplaceMuxDmaPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_MUX_DMA_PASS_H_INCLUDED
