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

#ifndef MUX_UTILS_HELPERS_H_INCLUDED
#define MUX_UTILS_HELPERS_H_INCLUDED

#include <cargo/string_view.h>
#include <mux/mux.h>

#include <cstdint>

namespace mux {
/// @addtogroup mux_utils
/// @{

/// @brief Find the first supported heap from `mux_memory_requirements_s`.
///
/// Counts the consecutive zero bits (trailing) on the right of an integer and
/// add one to get the first supported memory heap required by a
/// `mux_buffer_t` or `mux_image_t` object.
///
/// @param supported_heaps The `supported_heaps` member of
/// `mux_memory_requirements_s` to get the first supported heap from.
///
/// @return Returns the number of zero bits on the right.
uint32_t findFirstSupportedHeap(uint32_t supported_heaps);

/// @brief Helper function to detect profile when compiling OpenCL C source.
///
/// @param[in] device_info Mux device info to detect OpenCL profile for.
//
/// @return String profile of the Mux device, either "FULL_PROFILE" or
/// "EMBEDDED_PROFILE".
cargo::string_view detectOpenCLProfile(mux_device_info_t device_info);

/// @brief Memory allocator for Mux.
///
/// Default memory allocator that is implemented in terms of the platform's
/// system calls.
///
/// @param[in] user_data User data that would normally be passed to custom
/// allocator - is ignored by mux::alloc.
/// @param[in] size Size in bytes of memory to be allocated.
/// @param[in] alignment Alignment of allocation.
///
/// @return Address of allocated memory.
void *alloc(void *user_data, size_t size, size_t alignment);

/// @brief Memory deallocator for Mux.
///
/// Default memory deallocator that is implemented in terms of the platform's
/// system calls. Matches the call made by mux::alloc.
///
/// @param[in] user_data User data that would normally be passed to custom
/// allocator - is ignored by mux::alloc.
/// @param[in] pointer Address of allocation to free.
void free(void *user_data, void *pointer);

/// @brief Synchronize mux memory between devices.
///
/// @param[in] src_device The mux device the src memory belongs to.
/// @param[in] dst_device The mux device the dst memory belongs to.
/// @param[in] src_memory The mux memory of the src buffer.
/// @param[in] dst_memory The mux memory of the dst buffer.
/// @param[in] src_host_ptr An optional host pointer that has already been
/// mapped to the device src memory.
/// @param[in] dst_host_ptr An optional host pointer that has already been
/// mapped to the device dst memory.
/// @param[in] offset The offset into the source and destination buffers to
/// perform the synchronization at.
/// @param[in] size The size of the memory to synchronize.
///
/// @return Error code indicating result of the synchronization operation.
mux_result_t synchronizeMemory(mux_device_t src_device, mux_device_t dst_device,
                               mux_memory_t src_memory, mux_memory_t dst_memory,
                               void *src_host_ptr, void *dst_host_ptr,
                               uint64_t offset, uint64_t size);

/// @}
}  // namespace mux

#endif  // MUX_UTILS_HELPERS_H_INCLUDED
