// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Materialize missing builtins.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED
#define COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief The MakeFunctionNameUnique pass changes the name of a specified
/// function
///
/// The module pass `MakeFunctionNameUniquePass` is used to give distinct names
/// to scheduled kernels. This is necessary since a single kernel can be run
/// more than once across different work sizes and we want to be able
/// differentiate them.
///
/// Runs over all kernels with "kernel entry point" metadata.
class MakeFunctionNameUniquePass final
    : public llvm::PassInfoMixin<MakeFunctionNameUniquePass> {
 public:
  /// @brief Constructor.
  ///
  /// @param Name An LLVM StringRef of the name to replace the function name
  /// with.
  MakeFunctionNameUniquePass(llvm::StringRef Name) : UniqueName(Name.str()) {}

  // This pass is not an optimization - it must be run
  static bool isRequired() { return true; }

  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);

 private:
  std::string UniqueName;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_MAKE_FUNCTION_NAME_UNIQUE_PASS_H_INCLUDED
