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

#include "riscv/command_buffer.h"

#include "mux/mux.h"
#include "riscv/fence.h"
#include "utils/system.h"

namespace riscv {
void command_read_buffer_s::operator()(riscv::device_s *device, bool &error) {
  if (!device->hal_device->mem_read(host_pointer, buffer->targetPtr + offset,
                                    size)) {
    error = true;
  }
  device->profiler.update_counters(*device->hal_device);
}

void command_write_buffer_s::operator()(riscv::device_s *device, bool &error) {
  if (!device->hal_device->mem_write(buffer->targetPtr + offset, host_pointer,
                                     size)) {
    error = true;
  }
  device->profiler.update_counters(*device->hal_device);
}

void command_copy_buffer_s::operator()(riscv::device_s *device, bool &error) {
  if (!device->hal_device->mem_copy(dst_buffer->targetPtr + dst_offset,
                                    src_buffer->targetPtr + src_offset, size)) {
    error = true;
  }
  device->profiler.update_counters(*device->hal_device);
}

void command_fill_buffer_s::operator()(riscv::device_s *device, bool &error) {
  if (!device->hal_device->mem_fill(buffer->targetPtr + offset, pattern,
                                    pattern_size, size)) {
    error = true;
  }
  device->profiler.update_counters(*device->hal_device);
}

void command_ndrange_s::operator()(riscv::queue_s *queue, bool &error) {
  auto device = static_cast<riscv::device_s *>(queue->device);
  hal::hal_device_t *hal_device = device->hal_device;
  assert(kernel && hal_device);
  // ensure the elf file is loaded
  if (kernel->object_code.empty()) {
    error = true;
    return;
  }
  hal::hal_program_t program = hal_device->program_load(
      kernel->object_code.data(), kernel->object_code.size());
  if (program == hal::hal_invalid_program) {
    error = true;
    return;
  }
  // decide on which kernel to execute
  mux::hal::kernel_variant_s variant;
  if (mux_success !=
      kernel->getKernelVariantForWGSize(local_size[0], local_size[1],
                                        local_size[2], &variant)) {
    error = true;
    return;
  }
  // find the kernel entry point
  auto hal_kernel =
      hal_device->program_find_kernel(program, variant.variant_name.data());
  if (hal_kernel == hal::hal_invalid_kernel) {
    error = true;
    return;
  }
  // copy across the ndrange to run
  const hal::hal_ndrange_t hal_ndrange = {
      {global_offset[0], global_offset[1], global_offset[2]},
      {global_size[0], global_size[1], global_size[2]},
      {local_size[0], local_size[1], local_size[2]}};
  // execute the kernel
  bool success =
      hal_device->kernel_exec(program, hal_kernel, &hal_ndrange, kernel_args,
                              num_kernel_args, dimensions);
  hal_device->program_free(program);
  if (!success) {
    error = true;
  }
  device->profiler.update_counters(*device->hal_device, kernel->name.data());
}

void command_user_callback_s::operator()(
    riscv::queue_s *queue, riscv::command_buffer_s *command_buffer) {
  user_function(queue, command_buffer, user_data);
}

[[nodiscard]] mux_query_duration_result_t command_begin_query_s::operator()(
    riscv::device_s *device, mux_query_duration_result_t duration_query) {
  if (pool->type == mux_query_type_duration) {
    return static_cast<riscv::query_pool_s *>(pool)->getDurationQueryAt(index);
  } else {
    // Begin counter query; start accumulating all counters
    device->hal_device->counter_set_enabled(true);
    static_cast<riscv::query_pool_s *>(pool)->counter_accumulator_id =
        device->profiler.start_accumulating();
  }
  return duration_query;
}

[[nodiscard]] mux_query_duration_result_t command_end_query_s::operator()(
    riscv::device_s *device, mux_query_duration_result_t duration_query) {
  if (pool->type == mux_query_type_duration) {
    auto end_duration_query =
        static_cast<riscv::query_pool_s *>(pool)->getDurationQueryAt(index);
    if (duration_query == end_duration_query) {
      return nullptr;
    }
  } else {
    // End counter query; end accumulating all counters.
    device->profiler.stop_accumulating(
        static_cast<riscv::query_pool_s *>(pool)->counter_accumulator_id);
  }
  return duration_query;
}

void command_reset_query_pool_s::operator()() {
  if (pool->type == mux_query_type_duration) {
    auto query_pool = static_cast<riscv::query_pool_s *>(pool);
    query_pool->reset(sizeof(mux_query_duration_result_s) * index,
                      sizeof(mux_query_duration_result_s) * count);
  }
}

sync_point_s::sync_point_s(mux_command_buffer_t command_buffer) {
  this->command_buffer = command_buffer;
}

command_buffer_s::command_buffer_s(mux_device_t device,
                                   mux_allocator_info_t allocator_info,
                                   mux_fence_t fence)
    : commands(allocator_info),
      pod_data_allocs(allocator_info),
      kernel_arg_allocs(allocator_info),
      kernel_descriptor_allocs(allocator_info),
      sync_points(allocator_info),
      allocator_info(allocator_info),
      fence(static_cast<riscv::fence_s *>(fence)) {
  this->device = device;
}

command_buffer_s::~command_buffer_s() {
  riscv::fence_s::destroy(device, fence, mux::allocator(allocator_info));
}

mux_result_t command_buffer_s::execute(riscv::queue_s *queue) {
  riscv::device_s *riscv_device = static_cast<riscv::device_s *>(device);
  mux_query_duration_result_t duration_query = nullptr;

  for (riscv::command_s &command : commands) {
    uint64_t start = 0;
    if (duration_query) {
      start = utils::timestampNanoSeconds();
    }

    bool error = false;

    switch (command.type) {
      case riscv::command_type_read_buffer:
        command.read_buffer(riscv_device, error);
        break;
      case riscv::command_type_write_buffer:
        command.write_buffer(riscv_device, error);
        break;
      case riscv::command_type_fill_buffer:
        command.fill_buffer(riscv_device, error);
        break;
      case riscv::command_type_copy_buffer:
        command.copy_buffer(riscv_device, error);
        break;
      case riscv::command_type_ndrange:
        command.ndrange(queue, error);
        break;
      case riscv::command_type_user_callback:
        command.user_callback(queue, this);
        break;
      case riscv::command_type_begin_query:
        duration_query = command.begin_query(riscv_device, duration_query);
        break;
      case riscv::command_type_end_query:
        duration_query = command.end_query(riscv_device, duration_query);
        break;
      case riscv::command_type_reset_query_pool:
        command.reset_query_pool();
        break;
      default:
        return mux_error_fence_failure;
    }

    if (duration_query) {
      auto end = utils::timestampNanoSeconds();
      duration_query->start = start;
      duration_query->end = end;
    }

    // TODO: Act on error - see CA-3979
    if (error) {
      return mux_error_fence_failure;
    }
  }

  return mux_success;
}
}  // namespace riscv

mux_result_t riscvCreateCommandBuffer(
    mux_device_t device, mux_callback_info_t callback_info,
    mux_allocator_info_t allocator_info,
    mux_command_buffer_t *out_command_buffer) {
  mux::allocator allocator(allocator_info);
  (void)callback_info;

  mux_fence_t fence;
  if (riscvCreateFence(device, allocator_info, &fence)) {
    return mux_error_out_of_memory;
  }

  auto command_buffer =
      allocator.create<riscv::command_buffer_s>(device, allocator_info, fence);

  if (nullptr == command_buffer) {
    return mux_error_out_of_memory;
  }

  *out_command_buffer = command_buffer;

  return mux_success;
}

mux_result_t riscvCommandReadBuffer(mux_command_buffer_t command_buffer,
                                    mux_buffer_t buffer, uint64_t offset,
                                    void *riscv_pointer, uint64_t size,
                                    uint32_t, const mux_sync_point_t *,
                                    mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.emplace_back(
          riscv::command_read_buffer_s{static_cast<riscv::buffer_s *>(buffer),
                                       offset, riscv_pointer, size})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandReadBufferRegions(mux_command_buffer_t command_buffer,
                                           mux_buffer_t buffer,
                                           void *riscv_pointer,
                                           mux_buffer_region_info_t *regions,
                                           uint64_t regions_length, uint32_t,
                                           const mux_sync_point_t *,
                                           mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  char *data = reinterpret_cast<char *>(riscv_pointer);

  if (riscv->commands.reserve(riscv->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  /* Assign more space */
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += riscv->commands.size();

    if (riscv->commands.reserve(cmd_count)) {
      return mux_error_out_of_memory;
    }
  }

  /* Convert the 3D shapes into 1D slices */
  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    for (uint64_t z = 0; z < r.region.z; ++z) {
      const size_t dst_slice_offset = (r.dst_origin.z + z) * r.dst_desc.y;
      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;

      for (uint64_t y = 0; y < r.region.y; ++y) {
        const size_t dst_row_offset = (r.dst_origin.y + y) * r.dst_desc.x;
        const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;
        const size_t size = r.region.x;

        const uint64_t dst_offset =
            dst_slice_offset + dst_row_offset + r.dst_origin.x;
        const uint64_t src_offset =
            src_slice_offset + src_row_offset + r.src_origin.x;

        if (riscv->commands.emplace_back(riscv::command_read_buffer_s{
                static_cast<riscv::buffer_s *>(buffer), src_offset,
                data + dst_offset, size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandWriteBuffer(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer, uint64_t offset,
    const void *riscv_pointer, uint64_t size,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4282
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.emplace_back(
          riscv::command_write_buffer_s{static_cast<riscv::buffer_s *>(buffer),
                                        offset, riscv_pointer, size})) {
    return mux_error_out_of_memory;
  }
  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandWriteBufferRegions(mux_command_buffer_t command_buffer,
                                            mux_buffer_t buffer,
                                            const void *riscv_pointer,
                                            mux_buffer_region_info_t *regions,
                                            uint64_t regions_length, uint32_t,
                                            const mux_sync_point_t *,
                                            mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  const char *data = reinterpret_cast<const char *>(riscv_pointer);

  if (riscv->commands.reserve(riscv->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  /* Assign more space */
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += riscv->commands.size();

    if (riscv->commands.reserve(cmd_count)) {
      return mux_error_out_of_memory;
    }
  }

  /* Convert the 3D shapes into 1D slices */
  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    for (uint64_t z = 0; z < r.region.z; ++z) {
      const size_t dst_slice_offset = (r.dst_origin.z + z) * r.dst_desc.y;
      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;

      for (uint64_t y = 0; y < r.region.y; ++y) {
        const size_t dst_row_offset = (r.dst_origin.y + y) * r.dst_desc.x;
        const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;
        const size_t size = r.region.x;

        const uint64_t dst_offset =
            dst_slice_offset + dst_row_offset + r.dst_origin.x;
        const uint64_t src_offset =
            src_slice_offset + src_row_offset + r.src_origin.x;

        if (riscv->commands.emplace_back(riscv::command_write_buffer_s{
                static_cast<riscv::buffer_s *>(buffer), src_offset,
                data + dst_offset, size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                    mux_buffer_t src_buffer,
                                    uint64_t src_offset,
                                    mux_buffer_t dst_buffer,
                                    uint64_t dst_offset, uint64_t size,
                                    uint32_t, const mux_sync_point_t *,
                                    mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  // lastly copy the new command onto the end of the buffer
  if (riscv->commands.emplace_back(riscv::command_copy_buffer_s{
          static_cast<riscv::buffer_s *>(src_buffer), src_offset,
          static_cast<riscv::buffer_s *>(dst_buffer), dst_offset, size})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandCopyBufferRegions(mux_command_buffer_t command_buffer,
                                           mux_buffer_t src_buffer,
                                           mux_buffer_t dst_buffer,
                                           mux_buffer_region_info_t *regions,
                                           uint64_t regions_length, uint32_t,
                                           const mux_sync_point_t *,
                                           mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.reserve(riscv->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  // Assign more space
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += riscv->commands.size();

    if (riscv->commands.reserve(cmd_count)) {
      return mux_error_out_of_memory;
    }
  }

  // Convert the 3D shapes into 1D slices
  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    for (uint64_t z = 0; z < r.region.z; ++z) {
      const size_t dst_slice_offset = (r.dst_origin.z + z) * r.dst_desc.y;
      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;

      for (uint64_t y = 0; y < r.region.y; ++y) {
        const size_t dst_row_offset = (r.dst_origin.y + y) * r.dst_desc.x;
        const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;
        const size_t size = r.region.x;

        const uint64_t dst_offset =
            dst_slice_offset + dst_row_offset + r.dst_origin.x;
        const uint64_t src_offset =
            src_slice_offset + src_row_offset + r.src_origin.x;

        if (riscv->commands.emplace_back(riscv::command_copy_buffer_s{
                static_cast<riscv::buffer_s *>(src_buffer), src_offset,
                static_cast<riscv::buffer_s *>(dst_buffer), dst_offset,
                size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandFillBuffer(mux_command_buffer_t command_buffer,
                                    mux_buffer_t buffer, uint64_t offset,
                                    uint64_t size, const void *pattern_pointer,
                                    uint64_t pattern_size, uint32_t,
                                    const mux_sync_point_t *,
                                    mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  riscv::command_fill_buffer_s fill_buffer;
  fill_buffer.buffer = static_cast<riscv::buffer_s *>(buffer);
  fill_buffer.offset = offset;
  fill_buffer.size = size;
  std::memcpy(fill_buffer.pattern, pattern_pointer, pattern_size);
  fill_buffer.pattern_size = pattern_size;

  if (riscv->commands.emplace_back(std::move(fill_buffer))) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandReadImage(mux_command_buffer_t command_buffer,
                                   mux_image_t image, mux_offset_3d_t offset,
                                   mux_extent_3d_t extent, uint64_t row_size,
                                   uint64_t slice_size, void *pointer,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)image;
  (void)offset;
  (void)extent;
  (void)row_size;
  (void)slice_size;
  (void)pointer;

  return mux_error_feature_unsupported;
}

mux_result_t riscvCommandWriteImage(
    mux_command_buffer_t command_buffer, mux_image_t image,
    mux_offset_3d_t offset, mux_extent_3d_t extent, uint64_t row_size,
    uint64_t slice_size, const void *pointer,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)image;
  (void)offset;
  (void)extent;
  (void)row_size;
  (void)slice_size;
  (void)pointer;

  return mux_error_feature_unsupported;
}

mux_result_t riscvCommandFillImage(mux_command_buffer_t command_buffer,
                                   mux_image_t image, const void *color,
                                   uint32_t color_size, mux_offset_3d_t offset,
                                   mux_extent_3d_t extent,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)image;
  (void)color;
  (void)color_size;
  (void)offset;
  (void)extent;

  return mux_error_feature_unsupported;
}

mux_result_t riscvCommandCopyImage(mux_command_buffer_t command_buffer,
                                   mux_image_t src_image, mux_image_t dst_image,
                                   mux_offset_3d_t src_offset,
                                   mux_offset_3d_t dst_offset,
                                   mux_extent_3d_s extent,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)src_image;
  (void)dst_image;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;

  return mux_error_feature_unsupported;
}

mux_result_t riscvCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)src_image;
  (void)dst_buffer;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;

  return mux_error_feature_unsupported;
}

mux_result_t riscvCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;
  (void)sync_point;

  (void)command_buffer;
  (void)src_buffer;
  (void)dst_image;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;

  return mux_error_feature_unsupported;
}

namespace {
// Returns the number of bytes needed for the POD allocation
uint32_t calcPodDataSize(
    const mux::dynamic_array<mux_descriptor_info_t> &descriptors) {
  // All of the POD data is stored in a vector. This is so the HAL
  // does not need to take a copy and manage the memory. The specialized
  // kernel will live as long as the arguments are needed.
  // Work out the size needed here
  uint32_t size_pod_data = 0;
  for (const auto &descriptor : descriptors) {
    if (descriptor.type == mux_descriptor_info_type_plain_old_data) {
      size_pod_data += descriptor.plain_old_data_descriptor.length;
    }
  }
  return size_pod_data;
}

// Iterates through the argument descriptors and uses them to initalize the HAL
// argument structs. For POD arguments, the POD allocation is also setup to
// point to the correct locations.
void setHALArgs(mux::dynamic_array<uint8_t> &pod_data,
                const hal::hal_device_info_t *hal_device_info,
                const mux::dynamic_array<mux_descriptor_info_t> &descriptors,
                mux::dynamic_array<hal::hal_arg_t> &kernel_args) {
  uint32_t write_point = 0;
  for (uint64_t i = 0; i < descriptors.size(); i++) {
    const auto &descriptor = descriptors[i];
    switch (descriptor.type) {
      case mux_descriptor_info_type_buffer: {
        mux_descriptor_info_buffer_s info = descriptor.buffer_descriptor;
        riscv::buffer_s *const riscv_buffer =
            static_cast<riscv::buffer_s *>(info.buffer);
        hal::hal_arg_t arg;
        arg.kind = hal::hal_arg_address;
        arg.space = hal::hal_space_global;
        arg.address = riscv_buffer->targetPtr + info.offset;
        // size is the size of a device pointer
        arg.size = hal_device_info->word_size / 8;
        kernel_args[i] = arg;
      } break;
      case mux_descriptor_info_type_plain_old_data: {
        mux_descriptor_info_plain_old_data_s info =
            descriptor.plain_old_data_descriptor;
        hal::hal_arg_t arg;
        arg.kind = hal::hal_arg_value;
        arg.space = hal::hal_space_global;
        arg.size = info.length;
        std::memcpy(pod_data.data() + write_point, info.data, info.length);
        arg.pod_data = pod_data.data() + write_point;
        write_point += info.length;
        kernel_args[i] = arg;

      } break;
      case mux_descriptor_info_type_shared_local_buffer: {
        mux_descriptor_info_shared_local_buffer_s info =
            descriptor.shared_local_buffer_descriptor;
        hal::hal_arg_t arg;
        arg.kind = hal::hal_arg_address;
        arg.space = hal::hal_space_local;
        arg.size = info.size;
        kernel_args[i] = arg;
      } break;
      case mux_descriptor_info_type_null_buffer: {
        hal::hal_arg_t arg;
        arg.kind = hal::hal_arg_address;
        arg.space = hal::hal_space_global;
        arg.address = hal::hal_nullptr;
        arg.size = 0;
        kernel_args[i] = arg;
      } break;
    }
  }
}
}  // anonymous namespace

mux_result_t riscvCommandNDRange(mux_command_buffer_t command_buffer,
                                 mux_kernel_t kernel,
                                 mux_ndrange_options_t options, uint32_t,
                                 const mux_sync_point_t *,
                                 mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);
  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  auto *riscv_device = static_cast<riscv::device_s *>(riscv->device);
  auto *hal_device = riscv_device->hal_device;
  const hal::hal_device_info_t *hal_device_info = hal_device->get_info();

  mux::allocator allocator(riscv->allocator_info);
  mux::dynamic_array<mux_descriptor_info_t> descriptors{allocator};
  if (descriptors.alloc(options.descriptors_length)) {
    return mux_error_out_of_memory;
  }

  // Make a copy of the descriptor so that their lifetime extends beyond this
  // function call.
  cargo::array_view<mux_descriptor_info_t> option_descriptors(
      options.descriptors, options.descriptors_length);
  std::copy(std::begin(option_descriptors), std::end(option_descriptors),
            std::begin(descriptors));

  const uint32_t size_pod_data = calcPodDataSize(descriptors);
  mux::dynamic_array<uint8_t> pod_data{allocator};
  if (size_pod_data != 0 && pod_data.alloc(size_pod_data)) {
    return mux_error_out_of_memory;
  }

  mux::dynamic_array<hal::hal_arg_t> kernel_args{allocator};
  if (kernel_args.alloc(options.descriptors_length)) {
    return mux_error_out_of_memory;
  }

  std::array<size_t, 3> global_size;
  std::array<size_t, 3> global_offset;
  std::array<size_t, 3> local_size;
  for (size_t i = 0; i < 3; i++) {
    const bool in_range = i < options.dimensions;
    global_size[i] = in_range ? options.global_size[i] : 1;
    global_offset[i] = in_range ? options.global_offset[i] : 0;
    local_size[i] = options.local_size[i];
  }

  setHALArgs(pod_data, hal_device_info, descriptors, kernel_args);

  if (riscv->commands.push_back(riscv::command_ndrange_s{
          static_cast<riscv::kernel_s *>(kernel), kernel_args.data(),
          descriptors.data(), static_cast<uint32_t>(options.descriptors_length),
          pod_data.data(), global_size, global_offset, local_size,
          options.dimensions})) {
    return mux_error_out_of_memory;
  }

  if (riscv->pod_data_allocs.push_back(std::move(pod_data))) {
    return mux_error_out_of_memory;
  }

  if (riscv->kernel_arg_allocs.push_back(std::move(kernel_args))) {
    return mux_error_out_of_memory;
  }

  if (riscv->kernel_descriptor_allocs.push_back(std::move(descriptors))) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvUpdateDescriptors(mux_command_buffer_t command_buffer,
                                    mux_command_id_t command_id,
                                    uint64_t num_args, uint64_t *arg_indices,
                                    mux_descriptor_info_t *descriptors) {
  // Get the command to update.
  auto *riscv_command_buffer =
      static_cast<riscv::command_buffer_s *>(command_buffer);
  cargo::lock_guard<cargo::mutex> lock{riscv_command_buffer->mutex};
  auto &nd_range_to_update = riscv_command_buffer->commands[command_id];

  // Check the command being updated is actually an ND range.
  if (nd_range_to_update.type != riscv::command_type_ndrange) {
    return mux_error_invalid_value;
  }

  // Patch its arguments.
  auto nd_range_command = nd_range_to_update.ndrange;
  hal::hal_arg_t *args = nd_range_command.kernel_args;
  for (unsigned i = 0; i < num_args; ++i) {
    auto arg_descriptor = descriptors[i];
    auto index = arg_indices[i];
    hal::hal_arg_t &arg = args[index];
    switch (arg_descriptor.type) {
      default:
        return mux_error_invalid_value;
      case mux_descriptor_info_type_buffer: {
        mux_descriptor_info_buffer_s info = descriptors[i].buffer_descriptor;
        auto *const riscv_buffer = static_cast<riscv::buffer_s *>(info.buffer);

        arg.address = riscv_buffer->targetPtr + info.offset;
      } break;
      case mux_descriptor_info_type_plain_old_data: {
        mux_descriptor_info_plain_old_data_s info =
            descriptors[i].plain_old_data_descriptor;
        std::memcpy((void *)arg.pod_data, info.data, arg.size);
      } break;
      case mux_descriptor_info_type_shared_local_buffer: {
        mux_descriptor_info_shared_local_buffer_s info =
            descriptors[i].shared_local_buffer_descriptor;
        arg.size = info.size;
      } break;
      case mux_descriptor_info_type_null_buffer: {
        arg.size = 0;
        arg.address = hal::hal_nullptr;
      } break;
    }
  }

  return mux_success;
}

mux_result_t riscvCommandUserCallback(mux_command_buffer_t command_buffer,
                                      mux_command_user_callback_t user_function,
                                      void *user_data, uint32_t,
                                      const mux_sync_point_t *,
                                      mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.emplace_back(
          riscv::command_user_callback_s{user_function, user_data})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandBeginQuery(mux_command_buffer_t command_buffer,
                                    mux_query_pool_t query_pool,
                                    uint32_t query_index, uint32_t query_count,
                                    uint32_t, const mux_sync_point_t *,
                                    mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.emplace_back(riscv::command_begin_query_s{
          static_cast<riscv::query_pool_s *>(query_pool), query_index,
          query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandEndQuery(mux_command_buffer_t command_buffer,
                                  mux_query_pool_t query_pool,
                                  uint32_t query_index, uint32_t query_count,
                                  uint32_t, const mux_sync_point_t *,
                                  mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  auto found =
      std::find_if(riscv->commands.begin(), riscv->commands.end(),
                   [&](riscv::command_s &info) {
                     return info.type == riscv::command_type_begin_query &&
                            info.begin_query.pool == query_pool &&
                            info.begin_query.index == query_index &&
                            info.begin_query.count == query_count;
                   });
  if (found == riscv->commands.end()) {
    return mux_error_invalid_value;
  }

  if (riscv->commands.emplace_back(riscv::command_end_query_s{
          static_cast<riscv::query_pool_s *>(query_pool), query_index,
          query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvCommandResetQueryPool(mux_command_buffer_t command_buffer,
                                        mux_query_pool_t query_pool,
                                        uint32_t query_index,
                                        uint32_t query_count, uint32_t,
                                        const mux_sync_point_t *,
                                        mux_sync_point_t *sync_point) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  if (riscv->commands.emplace_back(riscv::command_reset_query_pool_s{
          static_cast<riscv::query_pool_s *>(query_pool), query_index,
          query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(riscv->allocator_info);
    auto out_sync_point = allocator.create<riscv::sync_point_s>(riscv);
    if (nullptr == out_sync_point ||
        riscv->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t riscvResetCommandBuffer(mux_command_buffer_t command_buffer) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);

  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);

  riscv->commands.clear();

  return mux_success;
}

mux_result_t riscvFinalizeCommandBuffer(mux_command_buffer_t command_buffer) {
  auto riscv = static_cast<riscv::command_buffer_s *>(command_buffer);
  cargo::lock_guard<cargo::mutex> lock(riscv->mutex);
  if (nullptr == command_buffer) {
    return mux_error_null_out_parameter;
  }
  // Finalizing a command buffer is a nop on riscv.
  return mux_success;
}

namespace {
// Create a deep copy of a ndrange command and add it as a command to the cloned
// command-buffer.
mux_result_t cloneNDRangeCommand(mux_allocator_info_t allocator_info,
                                 const riscv::command_ndrange_s &original,
                                 riscv::command_buffer_s *cloned_command_buffer)
    CARGO_TS_REQUIRES(cloned_command_buffer->mutex) {
  mux::allocator allocator(allocator_info);
  mux::dynamic_array<mux_descriptor_info_t> descriptors{allocator};
  if (descriptors.alloc(original.num_kernel_args)) {
    return mux_error_out_of_memory;
  }

  // Make a copy of the descriptor so that they are owned by the cloned
  // command
  cargo::array_view<mux_descriptor_info_t> original_descriptors(
      original.descriptors, original.num_kernel_args);
  std::copy(std::begin(original_descriptors), std::end(original_descriptors),
            std::begin(descriptors));

  const uint32_t size_pod_data = calcPodDataSize(descriptors);
  mux::dynamic_array<uint8_t> pod_data{allocator};
  if (size_pod_data != 0 && pod_data.alloc(size_pod_data)) {
    return mux_error_out_of_memory;
  }
  mux::dynamic_array<hal::hal_arg_t> kernel_args{allocator};
  if (kernel_args.alloc(original.num_kernel_args)) {
    return mux_error_out_of_memory;
  }

  // Copy HAL arguments
  cargo::array_view<hal::hal_arg_t> original_args(original.kernel_args,
                                                  original.num_kernel_args);
  std::copy(std::begin(original_args), std::end(original_args),
            std::begin(kernel_args));

  if (cloned_command_buffer->commands.push_back(riscv::command_ndrange_s{
          original.kernel, kernel_args.data(), descriptors.data(),
          original.num_kernel_args, pod_data.data(), original.global_size,
          original.global_offset, original.local_size, original.dimensions})) {
    return mux_error_out_of_memory;
  }

  // Store allocations so that they are freed when the cloned
  // command-buffer is destroyed
  if (cloned_command_buffer->pod_data_allocs.push_back(std::move(pod_data))) {
    return mux_error_out_of_memory;
  }
  if (cloned_command_buffer->kernel_arg_allocs.push_back(
          std::move(kernel_args))) {
    return mux_error_out_of_memory;
  }
  if (cloned_command_buffer->kernel_descriptor_allocs.push_back(
          std::move(descriptors))) {
    return mux_error_out_of_memory;
  }
  return mux_success;
}
}  // anonymous namespace

mux_result_t riscvCloneCommandBuffer(mux_device_t device,
                                     mux_allocator_info_t allocator_info,
                                     mux_command_buffer_t command_buffer,
                                     mux_command_buffer_t *out_command_buffer) {
  mux::allocator allocator(allocator_info);
  auto fence = riscv::fence_s::create<riscv::fence_s>(device, allocator);
  if (!fence) {
    return fence.error();
  }

  auto cloned_command_buffer = allocator.create<riscv::command_buffer_s>(
      device, allocator_info, fence.value());
  if (nullptr == cloned_command_buffer) {
    return mux_error_out_of_memory;
  }

  // Deep copy ndrange kernel commands so that we can update them with the
  // MuxUpdateDescriptors() without affecting the original command-buffer.
  auto *riscv_command_buffer =
      static_cast<riscv::command_buffer_s *>(command_buffer);
  cargo::lock_guard<cargo::mutex> lock_original{riscv_command_buffer->mutex};
  cargo::lock_guard<cargo::mutex> lock_clone{cloned_command_buffer->mutex};

  for (auto &command : riscv_command_buffer->commands) {
    if (command.type != riscv::command_type_ndrange) {
      if (cloned_command_buffer->commands.push_back(command)) {
        return mux_error_out_of_memory;
      }
    } else {
      if (auto error = cloneNDRangeCommand(allocator_info, command.ndrange,
                                           cloned_command_buffer)) {
        return error;
      }
    }
  }
  *out_command_buffer = cloned_command_buffer;

  return mux_success;
}

void riscvDestroyCommandBuffer(mux_device_t device,
                               mux_command_buffer_t command_buffer,
                               mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  auto *riscv_command_buffer =
      static_cast<riscv::command_buffer_s *>(command_buffer);
  {
    cargo::lock_guard<cargo::mutex> lock{riscv_command_buffer->mutex};
    for (auto sync_point : riscv_command_buffer->sync_points) {
      allocator.destroy(sync_point);
    }
  }
  // The lock guard can't hold a reference to the command-buffer's mutex when
  // the destructor is called as this results in a use after free while
  // attempting to unlock the destroy mutex at scope exit.
  allocator.destroy(riscv_command_buffer);
}
