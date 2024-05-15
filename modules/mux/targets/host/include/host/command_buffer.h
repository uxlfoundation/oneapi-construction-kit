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
/// Host's command buffer interface.

#ifndef HOST_COMMAND_BUFFER_H_INCLUDED
#define HOST_COMMAND_BUFFER_H_INCLUDED

#include <cargo/expected.h>

#include <array>
#include <mutex>

#include "host/fence.h"
#include "mux/mux.h"
#include "mux/utils/dynamic_array.h"
#include "mux/utils/small_vector.h"

namespace host {
/// @addtogroup host
/// @{

/// @brief Struct that owns kernel args and schedule information for an ND
/// range.
///
/// This struct later gets cast to `void*` and passed to the lambda that threads
/// in the threadpool execute to actually run the range.
struct ndrange_info_s {
  ndrange_info_s(mux::dynamic_array<uint8_t> &packed_args,
                 mux::dynamic_array<uint8_t *> &arg_addresses,
                 mux::dynamic_array<mux_descriptor_info_t> &descriptors,
                 std::array<size_t, 3> global_size,
                 std::array<size_t, 3> global_offset,
                 std::array<size_t, 3> local_size, size_t dimensions)
      : packed_args(std::move(packed_args)),
        arg_addresses(std::move(arg_addresses)),
        descriptors(std::move(descriptors)),
        global_size(global_size),
        global_offset(global_offset),
        local_size(local_size),
        dimensions(dimensions) {}

  /// @brief Packed descriptors.
  mux::dynamic_array<uint8_t> packed_args;

  /// @brief Addresses of arguments in packed descriptors.
  ///
  /// Recording this information is required when packedArgs is populated in
  /// order to look up the address of the nth argument without knowing the sizes
  /// of each of the previous arguments.
  mux::dynamic_array<uint8_t *> arg_addresses;

  /// @brief descriptors for each kernel argument
  mux::dynamic_array<mux_descriptor_info_t> descriptors;

  /// @brief Global size.
  std::array<size_t, 3> global_size;

  /// @brief Global offset.
  std::array<size_t, 3> global_offset;

  /// @brief Local size.
  std::array<size_t, 3> local_size;

  /// @brief Dimensions in the ND range.
  size_t dimensions;

  /// @Brief Create a deep copy of the ndrange command
  cargo::expected<std::unique_ptr<ndrange_info_s>, mux_result_t> clone(
      mux_allocator_info_t allocator_info) const;
};

/// @brief Implementation of mux sync-point
///
/// TODO CA-4364: Implement sync-point
struct sync_point_s final : public mux_sync_point_s {
  explicit sync_point_s(mux_command_buffer_t command_buffer);
};

enum command_type_e : uint32_t {
  command_type_read_buffer,
  command_type_write_buffer,
  command_type_copy_buffer,
  command_type_fill_buffer,
  command_type_read_image,
  command_type_write_image,
  command_type_fill_image,
  command_type_copy_image,
  command_type_copy_image_to_buffer,
  command_type_copy_buffer_to_image,
  command_type_ndrange,
  command_type_user_callback,
  command_type_begin_query,
  command_type_end_query,
  command_type_reset_query_pool,
  command_type_terminate
};

struct command_info_read_buffer_s {
  mux_buffer_t buffer;
  uint64_t offset;
  void *host_pointer;
  uint64_t size;
};

struct command_info_write_buffer_s {
  mux_buffer_t buffer;
  uint64_t offset;
  const void *host_pointer;
  uint64_t size;
};

struct command_info_copy_buffer_s {
  mux_buffer_t src_buffer;
  uint64_t src_offset;
  mux_buffer_t dst_buffer;
  uint64_t dst_offset;
  uint64_t size;
};

struct command_info_fill_buffer_s {
  mux_buffer_t buffer;
  uint64_t offset;
  uint64_t size;
  char pattern[128];
  uint64_t pattern_size;
};

struct command_info_read_image_s {
  mux_image_t image;
  mux_offset_3d_t offset;
  mux_extent_3d_t extent;
  uint64_t row_size;
  uint64_t slice_size;
  void *pointer;
};

struct command_info_write_image_s {
  mux_image_t image;
  mux_offset_3d_t offset;
  mux_extent_3d_t extent;
  uint64_t row_size;
  uint64_t slice_size;
  const void *pointer;
};

struct command_info_fill_image_s {
  mux_image_t image;
  mux_offset_3d_t offset;
  mux_extent_3d_t extent;
  char color[16];
};

struct command_info_copy_image_s {
  mux_image_t src_image;
  mux_image_t dst_image;
  mux_offset_3d_t src_offset;
  mux_offset_3d_t dst_offset;
  mux_extent_3d_t extent;
};

struct command_info_copy_image_to_buffer_s {
  mux_image_t src_image;
  mux_buffer_t dst_buffer;
  mux_offset_3d_t src_offset;
  uint64_t dst_offset;
  mux_extent_3d_t extent;
};

struct command_info_copy_buffer_to_image_s {
  mux_buffer_t src_buffer;
  mux_image_t dst_image;
  uint32_t src_offset;
  mux_offset_3d_t dst_offset;
  mux_extent_3d_t extent;
};

struct command_info_ndrange_s {
  mux_kernel_t kernel;
  ndrange_info_s *ndrange_info;
};

struct command_info_user_callback_s {
  mux_command_user_callback_t user_function;
  void *user_data;
};

struct command_info_begin_query_s {
  mux_query_pool_t pool;
  uint32_t index;
  uint32_t count;
};

struct command_info_end_query_s {
  mux_query_pool_t pool;
  uint32_t index;
  uint32_t count;
};

struct command_info_reset_query_pool_s {
  mux_query_pool_t pool;
  uint32_t index;
  uint32_t count;
};

struct command_info_terminate_s {};

struct command_info_s {
  command_info_s(command_info_read_buffer_s read_command)
      : type(command_type_read_buffer), read_command(read_command) {}

  command_info_s(command_info_write_buffer_s write_command)
      : type(command_type_write_buffer), write_command(write_command) {}

  command_info_s(command_info_copy_buffer_s copy_command)
      : type(command_type_copy_buffer), copy_command(copy_command) {}

  command_info_s(command_info_fill_buffer_s fill_command)
      : type(command_type_fill_buffer), fill_command(fill_command) {}

  command_info_s(command_info_read_image_s read_command)
      : type(command_type_read_image), read_image_command(read_command) {}

  command_info_s(command_info_write_image_s write_command)
      : type(command_type_write_image), write_image_command(write_command) {}

  command_info_s(command_info_fill_image_s fill_command)
      : type(command_type_fill_image), fill_image_command(fill_command) {}

  command_info_s(command_info_copy_image_s copy_command)
      : type(command_type_copy_image), copy_image_command(copy_command) {}

  command_info_s(command_info_copy_image_to_buffer_s copy_command)
      : type(command_type_copy_image_to_buffer),
        copy_image_to_buffer_command(copy_command) {}

  command_info_s(command_info_copy_buffer_to_image_s copy_command)
      : type(command_type_copy_buffer_to_image),
        copy_buffer_to_image_command(copy_command) {}

  command_info_s(command_info_ndrange_s ndrange_command)
      : type(command_type_ndrange), ndrange_command(ndrange_command) {}

  command_info_s(command_info_user_callback_s user_callback_command)
      : type(command_type_user_callback),
        user_callback_command(user_callback_command) {}

  command_info_s(command_info_begin_query_s begin_query_command)
      : type(command_type_begin_query),
        begin_query_command(begin_query_command) {}

  command_info_s(command_info_end_query_s end_query_command)
      : type(command_type_end_query), end_query_command(end_query_command) {}

  command_info_s(command_info_reset_query_pool_s reset_query_pool_command)
      : type(command_type_reset_query_pool),
        reset_query_pool_command(reset_query_pool_command) {}

  command_info_s(command_info_terminate_s command)
      : type(command_type_terminate), terminate_command(command) {}

  host::command_type_e type;

  // We use a union to minimize the number of allocations that need to be made.
  union {
    struct host::command_info_read_buffer_s read_command;
    struct host::command_info_write_buffer_s write_command;
    struct host::command_info_copy_buffer_s copy_command;
    struct host::command_info_fill_buffer_s fill_command;
    struct host::command_info_read_image_s read_image_command;
    struct host::command_info_write_image_s write_image_command;
    struct host::command_info_fill_image_s fill_image_command;
    struct host::command_info_copy_image_s copy_image_command;
    struct host::command_info_copy_image_to_buffer_s
        copy_image_to_buffer_command;
    struct host::command_info_copy_buffer_to_image_s
        copy_buffer_to_image_command;
    struct host::command_info_ndrange_s ndrange_command;
    struct host::command_info_user_callback_s user_callback_command;
    struct host::command_info_begin_query_s begin_query_command;
    struct host::command_info_end_query_s end_query_command;
    struct host::command_info_reset_query_pool_s reset_query_pool_command;
    struct host::command_info_terminate_s terminate_command;
  };
};

struct command_buffer_s final : public mux_command_buffer_s {
  explicit command_buffer_s(mux_device_t device,
                            mux_allocator_info_t allocator_info,
                            mux_fence_t fence);

  ~command_buffer_s();

  mux::small_vector<host::command_info_s, 16> commands;
  mux::small_vector<std::unique_ptr<host::ndrange_info_s>, 4> ndranges;
  mux::small_vector<host::sync_point_s *, 4> sync_points;
  std::mutex mutex;
  mux::small_vector<mux_semaphore_t, 8> signal_semaphores;
  void (*user_function)(mux_command_buffer_t command_buffer, mux_result_t error,
                        void *const user_data);
  void *user_data;
  fence_s *fence;
  mux_allocator_info_t allocator_info;
};

/// @}
}  // namespace host

#endif  // HOST_COMMAND_BUFFER_H_INCLUDED
