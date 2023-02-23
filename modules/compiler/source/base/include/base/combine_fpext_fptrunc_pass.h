// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class CombineFPExtFPTruncPass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_COMBINE_FPEXT_FPTRUNC_PASS_H_INCLUDED
#define BASE_COMBINE_FPEXT_FPTRUNC_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Pass to combine FPExt and FPTrunc instructions that cancel each
/// other.
///
/// This is used after the printf replacement pass, because var-args printf
/// arguments may be expanded to double by clang even if the device doesn't
/// support doubles, so if the device doesn't support doubles, the printf pass
/// will fptrunc the parameters back to float. And this pass will find and
/// remove the matching fpext (added by clang) and fptrunc (added by the printf
/// pass) to get rid of the doubles.
struct CombineFPExtFPTruncPass final
    : public llvm::PassInfoMixin<CombineFPExtFPTruncPass> {
  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] F The function provided to the pass.
  /// @param[in,out] AM Manager providing analyses.
  ///
  /// @return Return whether or not the pass changed anything.
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

/// @}
}  // namespace compiler

#endif  // BASE_COMBINE_FPEXT_FPTRUNC_PASS_H_INCLUDED
