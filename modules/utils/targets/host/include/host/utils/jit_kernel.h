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

#ifndef HOST_UTILS_JIT_KERNEL_INCLUDED
#define HOST_UTILS_JIT_KERNEL_INCLUDED

#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>

#include <cstdint>
#include <string>

namespace host {
namespace utils {
/// @brief Contains the hook and metadata for JIT compiled kernels
struct jit_kernel_s {
  /// @brief Kernel name.
  std::string name;
  /// @brief Callable hook for running the kernel.
  uint64_t hook;
  /// @brief Total size of local memory buffers used by the kernel.
  uint32_t local_memory_used;
  /// @brief Factor of the minimum number of work-items the kernel may safely
  /// execute.
  uint32_t min_work_width;
  /// @brief Factor of the preferred number of work-items the kernel wishes to
  /// execute.
  uint32_t pref_work_width;
  /// @brief The size of the sub-group this kernel supports.
  ///
  /// Note that the last sub-group in a work-group may be smaller than this
  /// value.
  /// * If one, denotes a trivial sub-group.
  /// * If zero, denotes a 'degenerate' sub-group (i.e., the size of the
  /// work-group at enqueue time).
  uint32_t sub_group_size;
};

/// @brief Detects whether this binary buffer contains a JIT kernel hook and
/// its metadata.
///
/// @param binary The source binary data.
/// @param binary_length The length of the source binary (in bytes).
/// @return `true` if the binary is a JIT kernel, `false` otherwise.
bool isJITKernel(const void *binary, uint64_t binary_length);

/// @brief Creates an new instance of `jit_kernel_s` from the data contained
/// within the binary buffer.
///
/// @param binary The source binary data.
/// @param binary_length The length of the source binary (in bytes).
/// @return An instance of `jit_kernel_s`, or `cargo::nullopt` if the binary is
/// invalid.
cargo::optional<jit_kernel_s> deserializeJITKernel(const void *binary,
                                                   uint64_t binary_length);

/// @brief Returns the size of a binary buffer that can contain a serialized
/// `jit_kernel_s`.
///
/// @return `true` if the binary is a JIT kernel, `false` otherwise.
size_t getSizeForJITKernel();

/// @brief Serializes a `jit_kernel_s` to a buffer.
///
/// @param jit_kernel A pointer to a JIT kernel to write to `buffer`
/// @param buffer A buffer that is at least `getSizeForJITKernel()` bytes long.
void serializeJITKernel(const jit_kernel_s *jit_kernel, uint8_t *buffer);
}  // namespace utils
}  // namespace host

#endif  // HOST_UTILS_JIT_KERNEL_INCLUDED
