// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// Workaround LLVM x86 bug by removing 'byval' parameter attributes.

#ifndef HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED
#define HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief This pass removes all 'byval' parameter attributes from functions
/// when compiling for x86-64 targets.
///
/// It works around an LLVM x86-64 codegen bug
/// https://github.com/llvm/llvm-project/issues/34300 where byval parameters
/// are incorrectly lowered across call boundaries: calls pass by pointer,
/// callees expect by value.
class RemoveByValAttributesPass final
    : public llvm::PassInfoMixin<RemoveByValAttributesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace host

#endif  // HOST_REMOVE_BYVAL_ATTRIBUTES_PASS_H_INCLUDED
