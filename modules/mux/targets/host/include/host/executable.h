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
/// Host's executable interface.

#ifndef HOST_EXECUTABLE_H_INCLUDED
#define HOST_EXECUTABLE_H_INCLUDED

#include <memory>
#include <unordered_map>
#include <vector>

#include "host/utils/jit_kernel.h"
#include "loader/elf.h"
#include "loader/mapper.h"
#include "mux/mux.h"
#include "mux/utils/dynamic_array.h"
#include "mux/utils/small_vector.h"

namespace host {
/// @addtogroup host
/// @{
/// @brief Stores the hook and metadata for binary kernels
struct binary_kernel_s {
  /// @brief Callable hook for running the kernel.
  uint64_t hook;
  /// @brief Compiler-generated name for the kernel.
  std::string kernel_name;
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
  /// * If zero, denotes either no sub-groups or a 'degenerate' sub-group
  /// (i.e., the size of the work-group at enqueue time).
  uint32_t sub_group_size;
};

using kernel_variant_map =
    std::unordered_map<std::string, std::vector<::host::binary_kernel_s>>;

struct executable_s final : public mux_executable_s {
  /// @brief Create an executable from a single binary kernel outwith an ELF
  /// file.
  ///
  /// @param[in] device Mux device.
  /// @param[in] jit_kernel The single JIT binary kernel to be stored in this
  /// executable.
  executable_s(mux_device_t device, utils::jit_kernel_s jit_kernel,
               mux::allocator allocator);

  /// @brief Create an executable from a pre-compiled binary.
  ///
  /// @param[in] device Mux device.
  /// @param[in] elf_contents Contents of ELF file.
  /// @param[in] allocated_pages Allocated pages for loaded binary.
  /// @param[in] binary_kernels Binary kernel map.
  executable_s(mux_device_t device, mux::dynamic_array<uint64_t> elf_contents,
               mux::small_vector<loader::PageRange, 4> allocated_pages,
               host::kernel_variant_map binary_kernels);

  /// @brief Deleted copy constructor.
  ///
  /// This is because 'kernels' may contain a pointer to the data contained
  /// inside 'jit_kernel_name' which may become stale due to lack of string
  /// pointer stability when moving or copying (caused by SSO).
  executable_s(const executable_s &) = delete;

  /// @brief Deleted move constructor.
  ///
  /// See the copy constructor for an explanation as to why executable_s is not
  /// movable or copyable.
  executable_s(executable_s &&) = delete;

  /// @brief Deleted copy assignment operator.
  ///
  /// See the copy constructor for an explanation as to why executable_s is not
  /// movable or copyable.
  executable_s &operator=(const executable_s &) = delete;

  /// @brief Deleted move assignment operator.
  ///
  /// See the copy constructor for an explanation as to why executable_s is not
  /// movable or copyable.
  executable_s &operator=(executable_s &&) = delete;

  /// @brief If this executable contains a JIT kernel, this stores the name of
  /// that kernel.
  std::string jit_kernel_name;

  /// @brief ELF binary this executable was created from.
  mux::dynamic_array<uint64_t> elf_contents;

  /// @brief Pages allocated by our ELF loader for the binary.
  ///
  /// Kept around here for lifetime reasons, our executable shouldn't outlive
  /// these.
  mux::small_vector<loader::PageRange, 4> allocated_pages;

  /// @brief Map of kernel names to binary kernels contained in this executable.
  kernel_variant_map kernels;
};

/// @}
}  // namespace host

#endif  // HOST_EXECUTABLE_H_INCLUDED
