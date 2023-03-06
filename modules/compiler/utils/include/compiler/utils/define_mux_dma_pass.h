// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Define mux DMA util pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_DEFINE_MUX_DMA_PASS_H_INCLUDED
#define COMPILER_UTILS_DEFINE_MUX_DMA_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace llvm {
class ModulePass;
}

namespace compiler {
namespace utils {

/// @brief Defines the bodies of ComputeMux DMA builtins
class DefineMuxDmaPass final : public llvm::PassInfoMixin<DefineMuxDmaPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_DEFINE_MUX_DMA_PASS_H_INCLUDED
