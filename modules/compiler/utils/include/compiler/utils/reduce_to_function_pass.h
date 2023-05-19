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
/// Reduce-to-function pass

#ifndef COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED
#define COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

/// @brief A pass which removes dead functions not used by the target kernel.
///
/// The LLVM module when passed to scheduled kernel can contain multiple kernel
/// functions present in the device side program, however by this stage of
/// compilation we are only interested in running a single kernel. In order to
/// improve the speed of subsequent passes and reduce code size, this pass
/// removes dead functions not used by the target kernel. On pass creation
/// `ReduceToFunction` takes a string list of functions names to preserve, which
/// will include the name of our enqueued kernel and potentially some internal
/// functions needed for later passes, like DMA preload.
///
/// Runs over all kernels with "kernel" metadata.
class ReduceToFunctionPass final
    : public llvm::PassInfoMixin<ReduceToFunctionPass> {
 public:
  ReduceToFunctionPass() {}
  ReduceToFunctionPass(const llvm::ArrayRef<llvm::StringRef> &RefNames) {
    llvm::transform(RefNames, std::back_inserter(Names),
                    [](llvm::StringRef N) { return N.str(); });
  }

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  llvm::SmallVector<std::string, 4> Names;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REDUCE_TO_FUNCTION_PASS_H_INCLUDED
