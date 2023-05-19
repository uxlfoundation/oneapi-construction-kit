// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
///
/// Replace mux math declarations pass

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
