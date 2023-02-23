// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ADD_SCHEDULING_PARAMETERS_PASS_H_INCLUDED
#define COMPILER_UTILS_ADD_SCHEDULING_PARAMETERS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

class AddSchedulingParametersPass final
    : public llvm::PassInfoMixin<AddSchedulingParametersPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ADD_SCHEDULING_PARAMETERS_PASS_H_INCLUDED
