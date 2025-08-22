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
/// @brief Compiler related internal ocl header file.

#ifndef BASE_PRINTF_REPLACEMENT_PASS_H_INCLUDED
#define BASE_PRINTF_REPLACEMENT_PASS_H_INCLUDED

#include <builtins/printf.h>
#include <compiler/limits.h>
#include <llvm/IR/PassManager.h>

namespace llvm {
class Type;
class CallInst;
} // namespace llvm

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Pass replacing the printf calls by code that loads the printf
/// arguments in a buffer. The buffer is also added as an argument to functions
/// and kernels, see DESIGN.md for more details.
struct PrintfReplacementPass final
    : public llvm::PassInfoMixin<PrintfReplacementPass> {
  using PrintfDescriptorVecTy = std::vector<builtins::printf::descriptor>;
  /// @brief Constructor
  ///
  /// @param[out] descriptors An optional vector to be filled with descriptors
  /// of the printf calls that have been replaced by this pass.
  /// @param[in] buffer_size The required size of the printf buffer.
  PrintfReplacementPass(PrintfDescriptorVecTy *descriptors = nullptr,
                        size_t buffer_size = PRINTF_BUFFER_SIZE);

  /// @brief The entry point to the PrintfReplacementPass.
  ///
  /// This function finds all calls to the OpenCL printf function, and attempts
  /// to scalarized them.  If a call to printf contains an illegal OpenCL printf
  /// format string, then the call is removed, and all of its uses are replaced
  /// by the error code -1.  Else, the call will be replaced with a scalarized
  /// version.
  ///
  /// @param[in,out] module The module to run the pass on.
  /// @param[in,out] am The analysis manager providing analyses.
  ///
  /// @return Whether or not the pass changed anything in the module.
  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &am);

private:
  /// @brief Function that replace a printf call with a call to a custom
  /// function that loads the printf arguments into the printf buffer.
  ///
  /// @param[in,out] module The module.
  /// @param[in,out] ci The printf call instruction to rewrite.
  /// @param[in,out] printf_func The printf function.
  /// @param[in] get_group_id The `get_group_id` function.
  /// @param[in] get_num_groups The `get_num_groups` function.
  /// @param[in] printf_calls The list of printf calls accumulated by this pass
  void rewritePrintfCall(llvm::Module &module, llvm::CallInst *ci,
                         llvm::Function *printf_func,
                         llvm::Function *get_group_id,
                         llvm::Function *get_num_groups,
                         PrintfDescriptorVecTy &printf_calls);

  PrintfDescriptorVecTy *printf_calls_out_ptr;
  size_t printf_buffer_size;

  bool double_support = true;
};

/// @}
} // namespace compiler

#endif // BASE_PRINTF_REPLACEMENT_PASS_H_INCLUDED
