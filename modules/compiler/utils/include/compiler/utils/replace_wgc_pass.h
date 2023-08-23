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
/// Replace work-group collectives pass.

#ifndef COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief Provides a default implementation of the work-group collective
/// builtins using local memory as an accumulator. Targets with no hardware
/// support for work-group collectives may use this pass to provide a software
/// emulation.
///
/// This pass introduces barrier calls into the work-group collective
/// definitions so must be run before the PrepareBarriersPass and
/// HandleBarriersPass on any target making use of these passes. This pass also
/// introduces global variables into the module in the __local address space and
/// therefore must be run before the ReplaceLocalModuleScopeVariablesPass on any
/// target making use of that pass.
///
/// The constructor for this pass takes a boolean value indicating whether or
/// not only Work Group Collective Scan operations should be processed. This is
/// because the vectorizer and the Handle Barriers Pass are now able to work
/// with reductions and broadcasts *as-is*. Pass false here if you do not intend
/// to use the Handle Barriers Pass, so working implementations of these
/// builtins are still generated. Pass true if you are using the Handle Barriers
/// Pass, since its own implementations are more efficient.
class ReplaceWGCPass final : public llvm::PassInfoMixin<ReplaceWGCPass> {
 public:
  ReplaceWGCPass(bool scansOnly = false) : scans_only(scansOnly) {}
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  /// @brief when set to true, only work group collective scans will be
  /// replaced.
  bool scans_only = false;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_WGC_PASS_H_INCLUDED
