// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Replace mux math declarations pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_REPLACE_MUX_MATH_DECLS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_MUX_MATH_DECLS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {
/// @brief The pass replaces the following mux builtins:
/// * `__mux_isftz`
/// * `__mux_usefast`
/// * `__mux_isembeddedprofile`
///
/// * Looks for a function called `__mux_isftz`, if found defines the body of
///   that function to return _IsFTZ_.
/// * Looks for a function called `__mux_usefast`, if found defines the body
///   of that function to return _UseFast_.
/// * Looks for a function called `__mux_isembeddedprofile`, if found defines
///   the body of that function to return _IsEmbeddedProfile_.
///
/// This pass should be called after the builtins provided via a
/// `core_finalizer_t` are linked into a `core_executable_t`.
///
class ReplaceMuxMathDeclsPass final
    : public llvm::PassInfoMixin<ReplaceMuxMathDeclsPass> {
 public:
  /// @brief Constructor
  /// @param Fast Whether to use faster, less accurate maths algorithms. @param
  ReplaceMuxMathDeclsPass(bool Fast) : UseFast(Fast) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  const bool UseFast;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_MUX_MATH_DECLS_PASS_H_INCLUDED
