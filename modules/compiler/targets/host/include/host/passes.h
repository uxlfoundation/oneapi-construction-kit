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
/// Host's LLVM passes interface.

#ifndef HOST_PASSES_H_INCLUDED
#define HOST_PASSES_H_INCLUDED

#include <base/module.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/optional.h>
#include <llvm/Pass.h>
#include <stddef.h>

#include <array>
#include <unordered_map>

namespace llvm {
class FunctionPass;
class ModulePass;
class StringRef;
class TargetMachine;
}  // namespace llvm

namespace host {
/// @addtogroup host
/// @{

class HostTarget;
struct KernelMetadata;

llvm::ModulePass *createMakeFunctionNameUniquePass(
    llvm::StringRef &name, llvm::StringRef &unique_name);
llvm::ModulePass *createFixupCallingConventionPass();
llvm::FunctionPass *createRemoveLifetimeIntrinsicsPass();
llvm::ModulePass *createReduceToFunctionPass(
    const llvm::SmallVectorImpl<llvm::StringRef> &names);

/// @brief Converts a kernel metadata array into a vector of bytes according to
/// host's kernel metadata format (see host documentation)
///
/// @param kernels Array view to a list of `KernelMetadata` structs to be
/// converted.
cargo::expected<cargo::small_vector<uint8_t, 64>, compiler::Result>
createKernelsMetadata(const cargo::array_view<KernelMetadata> &kernels);

/// @brief Helper function to emit a binary from the given module.
///
/// @param module Module to emit the binary for.
/// @param target_machine Target machine that will create the emit binary pass.
cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result> emitBinary(
    llvm::Module *module, llvm::TargetMachine *target_machine);

/// @brief Applies optimization passes to
/// either all kernels in a module, or a single kernel, removing all the other
/// kernels.
///
/// @note Assumes finalizer->mutex is already locked.
///
/// @param[in] options Compiler build flags.
/// @param[in] PB PassBuilder to aid in pass pipeline construction
/// @param[in] snapshots A vector of `SnapshotDetails` objects containing
/// snapshot state.
/// @param[in] unique_prefix (Optional) prefix for the generated function names
/// to avoid linker conflicts.
/// @return Result ModulePassManager containing passes
llvm::ModulePassManager hostGetKernelPasses(
    compiler::Options options, llvm::PassBuilder &PB,
    cargo::array_view<compiler::BaseModule::SnapshotDetails> snapshots,
    cargo::optional<llvm::StringRef> unique_prefix = cargo::nullopt);

}  // namespace host

#endif  // HOST_PASSES_H_INCLUDED
