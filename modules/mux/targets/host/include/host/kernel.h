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
/// Host's kernel interface.

#ifndef HOST_KERNEL_H_INCLUDED
#define HOST_KERNEL_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <mux/mux.h>
#include <mux/utils/allocator.h>

#include <memory>
#include <string>

namespace host {
/// @addtogroup host
/// @{

struct schedule_info_s final {
  size_t global_size[3];
  size_t global_offset[3];
  size_t local_size[3];
  size_t slice;
  size_t total_slices;
  uint32_t work_dim;
};

struct kernel_variant_s {
  /// @brief Pointer type that points to the executable binary (symbol) that
  /// runs on the host CPU.
  typedef void (*entry_hook_t)(void *packed_args, schedule_info_s *schedule);

  kernel_variant_s() = default;

  explicit kernel_variant_s(std::string name, entry_hook_t hook,
                            size_t local_memory_used, uint32_t min_work_width,
                            uint32_t pref_work_width, uint32_t sub_group_size);
  /// @brief Name of the kernel.
  ///
  /// For built-in kernels, this is one of the built-in kernels available on
  /// host. For pre-compiled binaries, this is one of the kernels available in
  /// the binary. For kernels from source, this is one of the kernels in the
  /// source.
  std::string name;
  /// @brief Pointer to this kernel's symbol (binary that runs on the host CPU).
  entry_hook_t hook;

  size_t local_memory_used;
  uint32_t min_work_width = 0;
  uint32_t pref_work_width = 0;
  uint32_t sub_group_size = 0;
};

struct kernel_s final : public mux_kernel_s {
  /// @brief Create a kernel with a built-in kernel.
  ///
  /// @param[in] device Mux device.
  /// @param[in] name Name of the requested built-in kernel.
  /// @param[in] name_length Length of the @p name string.
  /// @param[in] hook Address of the builtin kernel function.
  kernel_s(mux_device_t device, mux::allocator allocator, const char *name,
           size_t name_length, kernel_variant_s::entry_hook_t hook);

  /// @brief Create a kernel from a pre-compiled binary.
  ///
  /// @param[in] variant_data woof
  kernel_s(mux_device_t device, mux_allocator_info_t allocator,
           cargo::small_vector<kernel_variant_s, 4> &&variant_data);

  mux_result_t getKernelVariantForWGSize(size_t local_size_x,
                                         size_t local_size_y,
                                         size_t local_size_z,
                                         kernel_variant_s *out_variant_data);

  /// @brief If the kernel is a built-in kernel.
  bool is_builtin_kernel;

  /// @brief The allocator used to create this kernel, used to allocate packed
  /// args when specialization info is provided.
  mux_allocator_info_t allocator_info;

  cargo::small_vector<kernel_variant_s, 4> variant_data;
};

/// @}
}  // namespace host

#endif  // HOST_KERNEL_H_INCLUDED
