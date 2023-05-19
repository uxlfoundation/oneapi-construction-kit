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
/// Barriers pass.

#ifndef COMPILER_UTILS_HANDLE_BARRIERS_PASS_H_INCLUDED
#define COMPILER_UTILS_HANDLE_BARRIERS_PASS_H_INCLUDED

#include <compiler/utils/barrier_regions.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/vectorization_factor.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>

#include <string>

namespace llvm {
class DominatorTree;
}

namespace compiler {
namespace utils {

class BuiltinInfo;
class BarrierWithLiveVars;

struct HandleBarriersOptions {
  /// @brief Set to true if the pass should add extra alloca
  /// instructions to preserve the values of variables between barriers.
  bool IsDebug = false;
  /// @brief Set to true if the pass should forcibly omit scalar
  /// tail loops from wrapped vector kernels, even if the local work-group size
  /// is not known to be a multiple of the vectorization factor.
  bool ForceNoTail = false;
};

/// @brief The "handle barriers" pass.
///
/// The handle barriers pass assumes that:
///
/// * `__mux_get_local_size` is a function in the module (corresponding to
///   OpenCL's get_local_size builtin)
/// * If `_Z13get_global_idj` or `_Z12get_local_idj` functions are in the
///   module, then a corresponding `__mux_set_local_id` function is also in the
///   module, with the following function signature:
///   `void __mux_set_local_id(uint_t index, size_t value);`
/// * Any functions containing barriers have already been inlined into the
///   kernel. Run PrepareBarriersPass first to ensure this.
///
/// The handle barriers pass will query a kernel function for the
/// `reqd_work_group_size` metadata and optimize accordingly in the presence of
/// it.
///
///
/// Runs over all kernels with "kernel entry point" metadata. Work-item orders
/// are sourced from the "work item order" function metadata on each kernel.
class HandleBarriersPass final
    : public llvm::PassInfoMixin<HandleBarriersPass> {
 public:
  /// @brief Constructor.
  HandleBarriersPass(const HandleBarriersOptions &Options)
      : IsDebug(Options.IsDebug), ForceNoTail(Options.ForceNoTail) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  /// @brief Make the work-item-loop wrapper function.
  /// This creates a wrapper function that iterates over a work group, calling
  /// the kernel for each work item, respecting the semantics of any barriers
  /// present. The wrapped kernel may be a scalar kernel, a vectorized kernel,
  /// or both. When the wrapped kernel wraps both a vector and scalar kernel,
  /// all vectorized work items will be executed first, and the scalar tail
  /// last.
  ///
  /// The wrapper function is created as a new function suffixed by
  /// ".mux-barrier-wrapper". The original unwrapped kernel(s)s will be left in
  /// the Module, but marked as internal linkage so later passes can remove
  /// them if uncalled once inlined into the wrapper function.
  ///
  /// When wrapping only a scalar kernel, or only a vector kernel, pass the
  /// same Barrier object as both Barrier input parameters.
  ///
  /// @param[in] barrierMain the Barrier object of the main kernel function
  /// @param[in] barrierTail the Barrier object of the tail kernel function
  /// (may be nullptr).
  /// @param[in] baseName the base name to use on the new wrapper function
  /// @param[in] M the module the kernels live in
  /// @param[in] BI BuiltinInfo providing builtin information
  /// @return The new wrapper function
  llvm::Function *makeWrapperFunction(BarrierWithLiveVars &barrierMain,
                                      BarrierWithLiveVars *barrierTail,
                                      llvm::StringRef baseName, llvm::Module &M,
                                      BuiltinInfo &BI);

  const bool IsDebug;
  const bool ForceNoTail;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_HANDLE_BARRIERS_PASS_H_INCLUDED
