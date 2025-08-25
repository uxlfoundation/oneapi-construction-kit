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
/// @brief Functionality for implementing OpenCL device side printf.

#ifndef CL_PRINTF_H_INCLUDED
#define CL_PRINTF_H_INCLUDED

#include <CL/cl.h>
#include <builtins/printf.h>
#include <mux/mux.hpp>

#include <array>
#include <vector>

/// @brief Allocate Mux memory and bind buffer for printf output based on local
/// and global execution size of a kernel.
///
/// @param[in] device Device to allocate memory for
/// @param[in] local_work_size Local size of the ND-Range
/// @param[in] global_work_size Global size of the ND-Range
/// @param[out] num_groups Total number of workgroups in the ND-Range
/// @param[out] buffer_group_size Bytes per workgroup in the allocated buffer
/// @param[out] printf_memory Mux memory allocated by the function
/// @param[out] printf_buffer Mux buffer bound to Mux memory object.
///
/// @return Returns CL_SUCCESS, or an OpenCL error code on failure.
cl_int createPrintfBuffer(
    cl_device_id device,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &local_work_size,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_work_size,
    size_t &num_groups, size_t &buffer_group_size, mux_memory_t &printf_memory,
    mux_buffer_t &printf_buffer);

/// @brief Structure passed to callback performing printf on host.
struct printf_info_t final {
  /// @brief OpenCL device which performed print
  cl_device_id device;
  /// @brief Mux memory where print data has been written
  mux_memory_t memory;
  /// @brief Mux buffer bound to memory
  mux_buffer_t buffer;
  /// @brief Size in bytes of the printf buffer per work-group chunk
  size_t buffer_group_size;
  /// @brief Offset into the buffer chunk for each work-group to start printing
  /// from
  std::vector<uint32_t> group_offsets;
  /// @brief Details of printf calls in the kernel program
  std::vector<builtins::printf::descriptor> &printf_calls;
  /// @brief Destructor for freeing mux allocated resources
  ~printf_info_t();
};

/// @brief Record a user callback command to the Mux command-buffer to perform
/// host printing from Mux buffer used for device-side printf.
///
/// This overload with a raw pointer printf_info_t frees the heap allocated
/// data in the callback.
///
/// @param[in] command_buffer Command-Buffer to record callback command to
/// @param[in] printf_info Heap allocated struct containing info needed in
/// callback
///
/// @return mux_success on completion, or a Mux error code on failure.
mux_result_t createPrintfCallback(mux_command_buffer_t command_buffer,
                                  printf_info_t *printf_info);

/// @brief Record a user callback command to the Mux command-buffer to perform
/// host printing from Mux buffer used for device-side printf.
///
/// This overload with a smart pointer printf_info_t *does not* free the heap
/// allocated data in the callback.
///
/// @param[in] command_buffer Command-Buffer to record callback command to
/// @param[in] printf_info Heap allocated struct containing info needed in
/// callback
///
/// @return mux_success on completion, or a Mux error code on failure.
mux_result_t createPrintfCallback(
    mux_command_buffer_t command_buffer,
    const std::unique_ptr<printf_info_t> &printf_info);
#endif  // CL_PRINTF_H_INCLUDED
