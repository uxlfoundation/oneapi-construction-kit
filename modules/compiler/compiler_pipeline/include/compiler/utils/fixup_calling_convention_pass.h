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
/// Fix up calling conventions pass.

#ifndef COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED
#define COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED

#include <llvm/IR/CallingConv.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass for making sure the calling convention of the target
/// executable matches the system default.
///
/// To make sure that the calling convention of the target executable matches
/// the system default calling convention `FixupCallingConventionPass` can be
/// run as a module pass. `FixupCallingConventionPass` iterates over all the
/// functions in the executable module, and if that function is not an
/// intrinsic, updates the calling convention of the function and all its call
/// instruction callees.
///
/// Runs over all kernels with "kernel" metadata.
///
/// @param cc An LLVM Calling Convention ID to be applied to all non-intrinsic
/// functions in the module.
class FixupCallingConventionPass final
    : public llvm::PassInfoMixin<FixupCallingConventionPass> {
 public:
  FixupCallingConventionPass(llvm::CallingConv::ID CC) : CC(CC) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  llvm::CallingConv::ID CC;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_FIXUP_CALLING_CONVENTION_PASS_H_INCLUDED
