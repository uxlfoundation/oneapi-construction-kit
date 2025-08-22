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
/// @brief Add Metadata Pass.
///
/// NOTE: This pass should be run after
/// compiler::utils::ComputeLocalMemoryUsagePass() so that the correct value for
/// local_memory is encoded into the serialized metadata. If the pass is not run
/// or run after this pass has completed, the value of local_memory_usage will
/// be encoded as 0.

#ifndef COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED
#define COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED

#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_hooks.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

template <class AnalysisTy, class HandlerTy>
class AddMetadataPass
    : public llvm::PassInfoMixin<AddMetadataPass<AnalysisTy, HandlerTy>> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    md_hooks hooks = getElfMetadataWriteHooks();
    HandlerTy handler;
    if (!handler.init(&hooks, &M)) {
      return llvm::PreservedAnalyses::none();
    }

    llvm::FunctionAnalysisManager &FAM =
        MAM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();

    for (auto &Fn : M.functions()) {
      if (!isKernelEntryPt(Fn)) {
        continue;
      }
      auto KernelInfo = FAM.getResult<AnalysisTy>(Fn);
      if (!handler.write(KernelInfo)) {
        return llvm::PreservedAnalyses::none();
      }
    }

    if (!handler.finalize()) {
      return llvm::PreservedAnalyses::none();
    }

    return llvm::PreservedAnalyses::all();
  }
};
} // namespace utils
} // namespace compiler

#endif // COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED
