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
/// Replace async copies pass.

#ifndef COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass that defines the async copy builtins of OpenCL C in terms of
/// __mux builtins. Declarations for the mux builtins are introduced by the
/// pass as they are required and it is expected that Mux targets implement
/// these builtins using hardware specific DMA functionality.
class ReplaceAsyncCopiesPass final
    : public llvm::PassInfoMixin<ReplaceAsyncCopiesPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_ASYNC_COPIES_PASS_H_INCLUDED
