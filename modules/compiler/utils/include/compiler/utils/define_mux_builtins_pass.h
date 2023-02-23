// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_DEFINE_MUX_BUILTINS_PASS_H_INCLUDED
#define COMPILER_UTILS_DEFINE_MUX_BUILTINS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

class DefineMuxBuiltinsPass final
    : public llvm::PassInfoMixin<DefineMuxBuiltinsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_DEFINE_MUX_BUILTINS_PASS_H_INCLUDED
