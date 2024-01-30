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
/// @brief The RISC-V target's command buffer interface.

#ifndef RISCV_COMMAND_BUFFER_H_INCLUDED
#define RISCV_COMMAND_BUFFER_H_INCLUDED

#include <array>
#include <atomic>

#include "cargo/mutex.h"
#include "mux/mux.h"
#include "mux/utils/dynamic_array.h"
#include "riscv/buffer.h"
#include "riscv/device.h"
#include "riscv/fence.h"
#include "riscv/kernel.h"
#include "riscv/query_pool.h"

namespace riscv {
/// @addtogroup riscv
/// @{

struct sync_point_s final : public mux_sync_point_s {
  explicit sync_point_s(mux_command_buffer_t command_buffer);
};

struct command_buffer_s;

enum command_type_e : uint32_t {
  command_type_read_buffer,
  command_type_write_buffer,
  command_type_copy_buffer,
  command_type_fill_buffer,
  command_type_ndrange,
  command_type_user_callback,
  command_type_begin_query,
  command_type_end_query,
  command_type_reset_query_pool,
};

struct command_read_buffer_s {
  riscv::buffer_s *buffer;
  uint64_t offset;
  void *host_pointer;
  uint64_t size;

  void operator()(riscv::device_s *device, bool &error);
};

struct command_write_buffer_s {
  riscv::buffer_s *buffer;
  uint64_t offset;
  const void *host_pointer;
  uint64_t size;

  void operator()(riscv::device_s *device, bool &error);
};

struct command_copy_buffer_s {
  riscv::buffer_s *src_buffer;
  uint64_t src_offset;
  riscv::buffer_s *dst_buffer;
  uint64_t dst_offset;
  uint64_t size;

  void operator()(riscv::device_s *device, bool &error);
};

struct command_fill_buffer_s {
  riscv::buffer_s *buffer;
  uint64_t offset;
  uint64_t size;
  char pattern[128];
  uint64_t pattern_size;

  void operator()(riscv::device_s *device, bool &error);
};

struct command_ndrange_s {
  riscv::kernel_s *kernel;
  hal::hal_arg_t *kernel_args;
  mux_descriptor_info_t *descriptors;
  uint32_t num_kernel_args;
  uint8_t *pod_data;
  std::array<size_t, 3> global_size;
  std::array<size_t, 3> global_offset;
  std::array<size_t, 3> local_size;
  size_t dimensions;

  void operator()(riscv::queue_s *queue, bool &error);
};

struct command_user_callback_s {
  mux_command_user_callback_t user_function;
  void *user_data;

  void operator()(riscv::queue_s *queue,
                  riscv::command_buffer_s *command_buffer);
};

struct command_begin_query_s {
  riscv::query_pool_s *pool;
  uint32_t index;
  uint32_t count;

  [[nodiscard]] mux_query_duration_result_t operator()(
      riscv::device_s *device, mux_query_duration_result_t duration_query);
};

struct command_end_query_s {
  riscv::query_pool_s *pool;
  uint32_t index;
  uint32_t count;

  [[nodiscard]] mux_query_duration_result_t operator()(
      riscv::device_s *device, mux_query_duration_result_t duration_query);
};

struct command_reset_query_pool_s {
  riscv::query_pool_s *pool;
  uint32_t index;
  uint32_t count;

  void operator()();
};

struct command_s {
  command_s(command_read_buffer_s read_buffer)
      : type(command_type_read_buffer), read_buffer(read_buffer) {}

  command_s(command_write_buffer_s write_buffer)
      : type(command_type_write_buffer), write_buffer(write_buffer) {}

  command_s(command_copy_buffer_s copy_buffer)
      : type(command_type_copy_buffer), copy_buffer(copy_buffer) {}

  command_s(command_fill_buffer_s fill_buffer)
      : type(command_type_fill_buffer), fill_buffer(fill_buffer) {}

  command_s(command_ndrange_s ndrange)
      : type(command_type_ndrange), ndrange(ndrange) {}

  command_s(command_user_callback_s user_callback)
      : type(command_type_user_callback), user_callback(user_callback) {}

  command_s(command_begin_query_s begin_query)
      : type(command_type_begin_query), begin_query(begin_query) {}

  command_s(command_end_query_s end_query)
      : type(command_type_end_query), end_query(end_query) {}

  command_s(command_reset_query_pool_s reset_query_pool)
      : type(command_type_reset_query_pool),
        reset_query_pool(reset_query_pool) {}

  riscv::command_type_e type;
  union {
    struct riscv::command_read_buffer_s read_buffer;
    struct riscv::command_write_buffer_s write_buffer;
    struct riscv::command_copy_buffer_s copy_buffer;
    struct riscv::command_fill_buffer_s fill_buffer;
    struct riscv::command_ndrange_s ndrange;
    struct riscv::command_user_callback_s user_callback;
    struct riscv::command_begin_query_s begin_query;
    struct riscv::command_end_query_s end_query;
    struct riscv::command_reset_query_pool_s reset_query_pool;
  };
};

struct command_buffer_s final : public mux_command_buffer_s {
  explicit command_buffer_s(mux_device_t device,
                            mux_allocator_info_t allocator_info,
                            mux_fence_t fence);

  ~command_buffer_s();

  mux_result_t execute(riscv::queue_s *queue) CARGO_TS_REQUIRES(mutex);

  mux::small_vector<riscv::command_s, 16> commands CARGO_TS_GUARDED_BY(mutex);
  mux::small_vector<mux::dynamic_array<uint8_t>, 16> pod_data_allocs
      CARGO_TS_GUARDED_BY(mutex);
  mux::small_vector<mux::dynamic_array<hal::hal_arg_t>, 16> kernel_arg_allocs
      CARGO_TS_GUARDED_BY(mutex);
  mux::small_vector<mux::dynamic_array<mux_descriptor_info_t>, 16>
      kernel_descriptor_allocs CARGO_TS_GUARDED_BY(mutex);
  mux::small_vector<riscv::sync_point_s *, 4> sync_points
      CARGO_TS_GUARDED_BY(mutex);
  cargo::mutex mutex;

  // TODO: Move this explicit fence out the mux layer into the user i.e. CL and
  // VK (see CA-4270).
  mux_allocator_info_t allocator_info;
  fence_s *fence;
};

/// @}
}  // namespace riscv

#endif  // RISCV_COMMAND_BUFFER_H_INCLUDED
