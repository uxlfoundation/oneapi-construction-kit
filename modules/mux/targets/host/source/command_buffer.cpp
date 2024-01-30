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

#include <host/buffer.h>
#include <host/command_buffer.h>
#include <host/host.h>
#include <host/image.h>
#include <host/kernel.h>
#include <host/query_pool.h>
#include <mux/utils/allocator.h>
#include <mux/utils/helpers.h>

#include <cstring>
#include <memory>
#include <new>

#include "mux/mux.h"

namespace {
// Returns the number of bytes which need allocated to hold all the packed args,
// and stores the offset into the allocation for each argument
size_t calcPackedArgsAllocSize(
    const mux::dynamic_array<mux_descriptor_info_t> &descriptors,
    mux::dynamic_array<size_t> &offsets) {
  size_t offset = 0;
  for (unsigned i = 0; i < descriptors.size(); i++) {
    const auto descriptor = descriptors[i];
    size_t size = 0;
    switch (descriptor.type) {
      case mux_descriptor_info_type_sampler:
        size = sizeof(size_t);
        break;
      case mux_descriptor_info_type_buffer:
      case mux_descriptor_info_type_null_buffer:
      case mux_descriptor_info_type_image:
        size = sizeof(void *);
        break;
      case mux_descriptor_info_type_plain_old_data: {
        size = descriptor.plain_old_data_descriptor.length;
      } break;
      case mux_descriptor_info_type_shared_local_buffer: {
        size = sizeof(size_t);
      } break;
    }
    offsets[i] = offset;
    offset += size;
  }
  return offset;
}

// Iterates through the argument descriptors and for each argument sets the
// location in the packed args location to the correct value.
void populatePackedArgs(
    uint8_t *packed_args_alloc,
    const mux::dynamic_array<mux_descriptor_info_t> &descriptors) {
  size_t offset = 0;
  for (unsigned i = 0; i < descriptors.size(); i++) {
    const auto descriptor = descriptors[i];
    switch (descriptor.type) {
      case mux_descriptor_info_type_buffer: {
        const mux_descriptor_info_buffer_s info = descriptor.buffer_descriptor;

        ::host::buffer_s *const host_buffer =
            static_cast<::host::buffer_s *>(info.buffer);

        uint8_t *const buffer_value =
            static_cast<uint8_t *>(host_buffer->data) + info.offset;

        std::memcpy(packed_args_alloc + offset, &buffer_value, sizeof(void *));
        offset += sizeof(void *);
      } break;
      case mux_descriptor_info_type_image: {
#ifdef HOST_IMAGE_SUPPORT
        mux_descriptor_info_image_s info = descriptor.image_descriptor;

        host::image_s *const host_image =
            static_cast<host::image_s *>(info.image);

        void *const image_ptr =
            libimg::HostGetImageKernelImagePtr(&host_image->image);

        std::memcpy(packed_args_alloc + offset, &image_ptr, sizeof(void *));
        offset += sizeof(void *);
#endif
      } break;
      case mux_descriptor_info_type_sampler: {
#ifdef HOST_IMAGE_SUPPORT
        mux_descriptor_info_sampler_s info = descriptor.sampler_descriptor;

        cl_addressing_mode addressing_mode = 0;
        switch (info.sampler.address_mode) {
          case mux_address_mode_none:
            addressing_mode = CL_ADDRESS_NONE;
            break;
          case mux_address_mode_repeat:
            addressing_mode = CL_ADDRESS_REPEAT;
            break;
          case mux_address_mode_repeat_mirror:
            addressing_mode = CL_ADDRESS_MIRRORED_REPEAT;
            break;
          case mux_address_mode_clamp:
            addressing_mode = CL_ADDRESS_CLAMP;
            break;
          case mux_address_mode_clamp_edge:
            addressing_mode = CL_ADDRESS_CLAMP_TO_EDGE;
            break;
        }

        cl_filter_mode filter_mode = 0;
        switch (info.sampler.filter_mode) {
          case mux_filter_mode_linear:
            filter_mode = CL_FILTER_NEAREST;
            break;
          case mux_filter_mode_nearest:
            filter_mode = CL_FILTER_LINEAR;
            break;
        }

        uint64_t sampler_ptr = libimg::HostCreateSampler(
            info.sampler.normalize_coords, addressing_mode, filter_mode);
        std::memcpy(packed_args_alloc + offset, &sampler_ptr, sizeof(size_t));
        offset += sizeof(size_t);
#endif
      } break;
      case mux_descriptor_info_type_plain_old_data: {
        const mux_descriptor_info_plain_old_data_s info =
            descriptor.plain_old_data_descriptor;

        std::memcpy(packed_args_alloc + offset, info.data, info.length);
        offset += info.length;
      } break;
      case mux_descriptor_info_type_shared_local_buffer: {
        mux_descriptor_info_shared_local_buffer_s info =
            descriptor.shared_local_buffer_descriptor;

        std::memcpy(packed_args_alloc + offset, &(info.size), sizeof(size_t));
        offset += sizeof(size_t);
      } break;
      case mux_descriptor_info_type_null_buffer: {
        void *nullvar = nullptr;
        std::memcpy(packed_args_alloc + offset, &nullvar, sizeof(void *));
        offset += sizeof(void *);
      } break;
    }
  }
}
}  // namespace

namespace host {
command_buffer_s::command_buffer_s(mux_device_t device,
                                   mux_allocator_info_t allocator_info,
                                   mux_fence_t fence)
    : commands(allocator_info),
      ndranges(allocator_info),
      sync_points(allocator_info),
      signal_semaphores(allocator_info),
      fence(static_cast<host::fence_s *>(fence)),
      allocator_info(allocator_info) {
  this->device = device;
}

command_buffer_s::~command_buffer_s() {
  hostDestroyFence(device, fence, allocator_info);
}

sync_point_s::sync_point_s(mux_command_buffer_t command_buffer) {
  this->command_buffer = command_buffer;
}

cargo::expected<std::unique_ptr<ndrange_info_s>, mux_result_t>
ndrange_info_s::clone(mux_allocator_info_t allocator_info) const {
  mux::allocator allocator(allocator_info);

  mux::dynamic_array<mux_descriptor_info_t> clone_descriptors{allocator};
  if (clone_descriptors.alloc(descriptors.size())) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }

  // Make a copy of the descriptor so that they are owned by the cloned
  // command
  std::copy(std::begin(descriptors), std::end(descriptors),
            std::begin(clone_descriptors));

  // Setup a new packed args allocation for this cloned command
  mux::dynamic_array<size_t> clone_offsets{allocator};
  if (clone_offsets.alloc(descriptors.size())) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }

  // Allocate data for packed arguments and record offset into it for each
  // argument
  const uint64_t packed_args_alloc_size =
      calcPackedArgsAllocSize(clone_descriptors, clone_offsets);
  uint8_t *const packed_args_allocation =
      static_cast<uint8_t *>(allocator.alloc(packed_args_alloc_size, 1));
  if (nullptr == packed_args_allocation) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }

  // Store address of each argument
  mux::dynamic_array<uint8_t *> clone_arg_addresses{allocator};
  if (clone_arg_addresses.alloc(descriptors.size())) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }
  for (size_t i = 0; i < descriptors.size(); i++) {
    clone_arg_addresses[i] = packed_args_allocation + clone_offsets[i];
  }

  // Populate packed args struct by copying original. We do this rather
  // than recreating from the descriptors, as for POD descriptors the data
  // pointer will no longer be valid if the kernel argument has since been
  // overwritten with clSetKernelArg, freeing the original
  // _cl_kernel::argument.
  std::memcpy(packed_args_allocation, packed_args, packed_args_alloc_size);

  return std::make_unique<host::ndrange_info_s>(
      packed_args_allocation, clone_arg_addresses, clone_descriptors,
      global_size, global_offset, local_size, dimensions);
}
}  // namespace host

mux_result_t hostCreateCommandBuffer(mux_device_t device,
                                     mux_callback_info_t callback_info,
                                     mux_allocator_info_t allocator_info,
                                     mux_command_buffer_t *out_command_buffer) {
  mux::allocator allocator(allocator_info);
  (void)callback_info;

  mux_fence_t fence;
  if (mux_success != hostCreateFence(device, allocator_info, &fence)) {
    return mux_error_out_of_memory;
  }

  auto command_buffer =
      allocator.create<host::command_buffer_s>(device, allocator_info, fence);

  if (nullptr == command_buffer) {
    return mux_error_out_of_memory;
  }

  *out_command_buffer = command_buffer;

  return mux_success;
}

mux_result_t hostCommandReadBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   void *host_pointer, uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_read_buffer_s{
          buffer, offset, host_pointer, size})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandReadBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  char *data = reinterpret_cast<char *>(host_pointer);

  if (host->commands.reserve(host->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  /* Assign more space */
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += host->commands.size();

    if (host->commands.reserve(cmd_count)) {
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

        if (host->commands.emplace_back(host::command_info_read_buffer_s{
                buffer, src_offset, data + dst_offset, size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandWriteBuffer(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer, uint64_t offset,
    const void *host_pointer, uint64_t size,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_write_buffer_s{
          buffer, offset, host_pointer, size})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandWriteBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    const void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  const char *data = reinterpret_cast<const char *>(host_pointer);

  if (host->commands.reserve(host->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  /* Assign more space */
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += host->commands.size();

    if (host->commands.reserve(cmd_count)) {
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

        if (host->commands.emplace_back(host::command_info_write_buffer_s{
                buffer, src_offset, data + dst_offset, size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t src_buffer, uint64_t src_offset,
                                   mux_buffer_t dst_buffer, uint64_t dst_offset,
                                   uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  // lastly copy the new command onto the end of the buffer
  if (host->commands.emplace_back(host::command_info_copy_buffer_s{
          src_buffer, src_offset, dst_buffer, dst_offset, size})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandCopyBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_buffer_t dst_buffer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.reserve(host->commands.size() + regions_length)) {
    return mux_error_out_of_memory;
  }

  // Assign more space
  {
    size_t cmd_count = 0;

    for (uint64_t i = 0; i < regions_length; ++i) {
      const auto &r = regions[i];
      cmd_count += (r.region.z * r.region.y);
    }

    cmd_count += host->commands.size();

    if (host->commands.reserve(cmd_count)) {
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

        if (host->commands.emplace_back(host::command_info_copy_buffer_s{
                src_buffer, src_offset, dst_buffer, dst_offset, size})) {
          return mux_error_out_of_memory;
        }
      }
    }
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandFillBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   uint64_t size, const void *pattern_pointer,
                                   uint64_t pattern_size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  host::command_info_fill_buffer_s fill_buffer;
  fill_buffer.buffer = buffer;
  fill_buffer.offset = offset;
  fill_buffer.size = size;
  std::memcpy(fill_buffer.pattern, pattern_pointer, pattern_size);
  fill_buffer.pattern_size = pattern_size;

  if (host->commands.emplace_back(std::move(fill_buffer))) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandReadImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent, uint64_t row_size,
                                  uint64_t slice_size, void *pointer,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  // TODO CA-4283
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_read_image_s{
          image, offset, extent, row_size, slice_size, pointer})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)image;
  (void)offset;
  (void)extent;
  (void)row_size;
  (void)slice_size;
  (void)pointer;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandWriteImage(mux_command_buffer_t command_buffer,
                                   mux_image_t image, mux_offset_3d_t offset,
                                   mux_extent_3d_t extent, uint64_t row_size,
                                   uint64_t slice_size, const void *pointer,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_write_image_s{
          image, offset, extent, row_size, slice_size, pointer})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)image;
  (void)offset;
  (void)extent;
  (void)row_size;
  (void)slice_size;
  (void)pointer;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandFillImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, const void *color,
                                  uint32_t color_size, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  host::command_info_fill_image_s fill_image;
  fill_image.image = image;
  fill_image.offset = offset;
  fill_image.extent = extent;
  std::memcpy(fill_image.color, color, color_size);

  if (host->commands.emplace_back(std::move(fill_image))) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)image;
  (void)color;
  (void)color_size;
  (void)offset;
  (void)extent;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandCopyImage(mux_command_buffer_t command_buffer,
                                  mux_image_t src_image, mux_image_t dst_image,
                                  mux_offset_3d_t src_offset,
                                  mux_offset_3d_t dst_offset,
                                  mux_extent_3d_s extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_copy_image_s{
          src_image, dst_image, src_offset, dst_offset, extent})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)src_image;
  (void)dst_image;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_copy_image_to_buffer_s{
          src_image, dst_buffer, src_offset, dst_offset, extent})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)src_image;
  (void)dst_buffer;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

#ifdef HOST_IMAGE_SUPPORT
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_copy_buffer_to_image_s{
          src_buffer, dst_image, src_offset, dst_offset, extent})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
#else
  (void)command_buffer;
  (void)src_buffer;
  (void)dst_image;
  (void)src_offset;
  (void)dst_offset;
  (void)extent;
  (void)sync_point;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCommandNDRange(mux_command_buffer_t command_buffer,
                                mux_kernel_t kernel,
                                mux_ndrange_options_t options,
                                uint32_t num_sync_points_in_wait_list,
                                const mux_sync_point_t *sync_point_wait_list,
                                mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);
  const std::lock_guard<std::mutex> lock(host->mutex);

  auto host_kernel = static_cast<host::kernel_s *>(kernel);
  mux::allocator allocator(host_kernel->allocator_info);

  std::array<size_t, 3> global_size;
  std::array<size_t, 3> global_offset;
  std::array<size_t, 3> local_size;

  for (size_t i = 0; i < 3; i++) {
    const bool in_range = i < options.dimensions;
    global_size[i] = in_range ? options.global_size[i] : 1;
    global_offset[i] = in_range ? options.global_offset[i] : 0;
    local_size[i] = options.local_size[i];
  }

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

  mux::dynamic_array<size_t> offsets{allocator};
  if (offsets.alloc(descriptors.size())) {
    return mux_error_out_of_memory;
  }

  // Allocate memory for the packed kernel arguments and record the offset
  // into the allocation for each argument
  const uint64_t packed_args_alloc_size =
      calcPackedArgsAllocSize(descriptors, offsets);
  uint8_t *const packed_args_allocation =
      static_cast<uint8_t *>(allocator.alloc(packed_args_alloc_size, 1));
  if (nullptr == packed_args_allocation) {
    return mux_error_out_of_memory;
  }

  // Store the address in packed args allocation of each argument
  mux::dynamic_array<uint8_t *> arg_addresses{allocator};
  if (arg_addresses.alloc(descriptors.size())) {
    return mux_error_out_of_memory;
  }
  for (size_t i = 0; i < descriptors.size(); i++) {
    arg_addresses[i] = packed_args_allocation + offsets[i];
  }

  // Store necessary argument information in the packed args allocation
  populatePackedArgs(packed_args_allocation, descriptors);

  if (host->ndranges.emplace_back(std::make_unique<host::ndrange_info_s>(
          packed_args_allocation, arg_addresses, descriptors, global_size,
          global_offset, local_size, options.dimensions))) {
    return mux_error_out_of_memory;
  }

  if (host->commands.push_back(
          host::command_info_ndrange_s{kernel, host->ndranges.back().get()})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostUpdateDescriptors(mux_command_buffer_t command_buffer,
                                   mux_command_id_t command_id,
                                   uint64_t num_args, uint64_t *arg_indices,
                                   mux_descriptor_info_t *descriptors) {
  // Get the command to update.
  auto *host = static_cast<host::command_buffer_s *>(command_buffer);
  auto &nd_range_to_update = host->commands[command_id];

  // Check the command being updated is actually an ND range.
  if (nd_range_to_update.type != host::command_type_ndrange) {
    return mux_error_invalid_value;
  }

  // Patch its arguments.
  for (unsigned i = 0; i < num_args; ++i) {
    auto index = arg_indices[i];
    uint8_t *const arg_address =
        nd_range_to_update.ndrange_command.ndrange_info->arg_addresses[index];
    auto arg_descriptor = descriptors[i];
    switch (arg_descriptor.type) {
      default:
        return mux_error_invalid_value;
      case mux_descriptor_info_type_buffer: {
        const mux_descriptor_info_buffer_s info =
            descriptors[i].buffer_descriptor;

        auto *host_buffer = static_cast<host::buffer_s *>(info.buffer);

        auto *buffer_value =
            static_cast<uint8_t *>(host_buffer->data) + info.offset;

        std::memcpy(arg_address, &buffer_value, sizeof(void *));
      } break;
      case mux_descriptor_info_type_plain_old_data: {
        const mux_descriptor_info_plain_old_data_s info =
            descriptors[i].plain_old_data_descriptor;

        std::memcpy(arg_address, info.data, info.length);
      } break;
      case mux_descriptor_info_type_shared_local_buffer: {
        mux_descriptor_info_shared_local_buffer_s info =
            descriptors[i].shared_local_buffer_descriptor;

        std::memcpy(arg_address, &(info.size), sizeof(size_t));
      } break;
      case mux_descriptor_info_type_null_buffer: {
        void *null = nullptr;
        std::memcpy(arg_address, &null, sizeof(void *));
      } break;
    }
  }
  return mux_success;
}

mux_result_t hostCommandUserCallback(
    mux_command_buffer_t command_buffer,
    mux_command_user_callback_t user_function, void *user_data,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(
          host::command_info_user_callback_s{user_function, user_data})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandBeginQuery(mux_command_buffer_t command_buffer,
                                   mux_query_pool_t query_pool,
                                   uint32_t query_index, uint32_t query_count,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_begin_query_s{
          query_pool, query_index, query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandEndQuery(mux_command_buffer_t command_buffer,
                                 mux_query_pool_t query_pool,
                                 uint32_t query_index, uint32_t query_count,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  auto found =
      std::find_if(host->commands.begin(), host->commands.end(),
                   [&](host::command_info_s &info) {
                     return info.type == host::command_type_begin_query &&
                            info.begin_query_command.pool == query_pool &&
                            info.begin_query_command.index == query_index &&
                            info.begin_query_command.count == query_count;
                   });
  if (found == host->commands.end()) {
    return mux_error_invalid_value;
  }

  if (host->commands.emplace_back(host::command_info_end_query_s{
          query_pool, query_index, query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostCommandResetQueryPool(
    mux_command_buffer_t command_buffer, mux_query_pool_t query_pool,
    uint32_t query_index, uint32_t query_count,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  // TODO CA-4364
  (void)num_sync_points_in_wait_list;
  (void)sync_point_wait_list;

  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  if (host->commands.emplace_back(host::command_info_reset_query_pool_s{
          query_pool, query_index, query_count})) {
    return mux_error_out_of_memory;
  }

  if (sync_point) {
    mux::allocator allocator(host->allocator_info);
    auto out_sync_point = allocator.create<host::sync_point_s>(host);
    if (nullptr == out_sync_point ||
        host->sync_points.push_back(out_sync_point)) {
      return mux_error_out_of_memory;
    }
    *sync_point = out_sync_point;
  }

  return mux_success;
}

mux_result_t hostResetCommandBuffer(mux_command_buffer_t command_buffer) {
  auto host = static_cast<host::command_buffer_s *>(command_buffer);

  const std::lock_guard<std::mutex> lock(host->mutex);

  host->commands.clear();

  return mux_success;
}

mux_result_t hostFinalizeCommandBuffer(mux_command_buffer_t command_buffer) {
  if (nullptr == command_buffer) {
    return mux_error_null_out_parameter;
  }
  // Finalizing a command buffer is a nop on host.
  return mux_success;
}

mux_result_t hostCloneCommandBuffer(mux_device_t device,
                                    mux_allocator_info_t allocator_info,
                                    mux_command_buffer_t command_buffer,
                                    mux_command_buffer_t *out_command_buffer) {
  // Create a new command buffer.
  mux::allocator allocator(allocator_info);
  auto *fence = allocator.create<host::fence_s>(device);
  if (!fence) {
    return mux_error_out_of_memory;
  }

  auto cloned_command_buffer =
      allocator.create<host::command_buffer_s>(device, allocator_info, fence);
  if (nullptr == cloned_command_buffer) {
    return mux_error_out_of_memory;
  }

  // Deep copy ndrange kernel commands so that we can update them with the
  // MuxUpdateDescriptors() without affecting the original command-buffer.
  auto *host_command_buffer =
      static_cast<host::command_buffer_s *>(command_buffer);
  const std::lock_guard<std::mutex> lock_original(host_command_buffer->mutex);
  const std::lock_guard<std::mutex> lock_clone{cloned_command_buffer->mutex};
  for (const auto &command : host_command_buffer->commands) {
    if (command.type != host::command_type_ndrange) {
      if (cloned_command_buffer->commands.push_back(command)) {
        return mux_error_out_of_memory;
      }
    } else {
      const host::command_info_ndrange_s &ndrange_command =
          command.ndrange_command;
      const auto &ndrange_info = ndrange_command.ndrange_info;

      auto cloned_ndrange_info = ndrange_info->clone(allocator_info);
      if (!cloned_ndrange_info) {
        return cloned_ndrange_info.error();
      }

      if (cloned_command_buffer->ndranges.emplace_back(
              std::move(*cloned_ndrange_info))) {
        return mux_error_out_of_memory;
      }

      if (cloned_command_buffer->commands.push_back(
              host::command_info_ndrange_s{
                  ndrange_command.kernel,
                  cloned_command_buffer->ndranges.back().get()})) {
        return mux_error_out_of_memory;
      }
    }
  }

  *out_command_buffer = cloned_command_buffer;

  return mux_success;
}

void hostDestroyCommandBuffer(mux_device_t device,
                              mux_command_buffer_t command_buffer,
                              mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);

  auto host = static_cast<host::command_buffer_s *>(command_buffer);
  for (auto &range : host->ndranges) {
    allocator.free(range->packed_args);
  }

  for (auto sync_point : host->sync_points) {
    allocator.destroy(sync_point);
  }

  allocator.destroy(host);
}
