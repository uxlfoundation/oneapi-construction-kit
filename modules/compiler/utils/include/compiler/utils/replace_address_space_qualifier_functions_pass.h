// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace address space qualifier functions pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED
#define UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that turns address space qualifier conversion functions into
/// address space casts, for targets that have only a unified memory layout.
///
class ReplaceAddressSpaceQualifierFunctionsPass final
    : public llvm::PassInfoMixin<ReplaceAddressSpaceQualifierFunctionsPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);

  static bool isRequired() { return true; }
};

}  // namespace utils
}  // namespace compiler

#endif  // UTILS_REPLACE_ADDRESS_SPACE_QUALIFIER_FUNCTIONS_PASS_H_INCLUDED
