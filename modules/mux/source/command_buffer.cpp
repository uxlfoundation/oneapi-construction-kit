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

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

namespace {
bool waitlistIsInvalid(uint32_t num_sync_points_in_wait_list,
                       const mux_sync_point_t *sync_point_wait_list) {
  if (!num_sync_points_in_wait_list ^ !sync_point_wait_list) {
    return true;
  }

  for (uint32_t i = 0; i < num_sync_points_in_wait_list; i++) {
    if (mux::objectIsInvalid(sync_point_wait_list[i])) {
      return true;
    }
  }
  return false;
}
}  // anonymous namespace

mux_result_t muxCreateCommandBuffer(mux_device_t device,
                                    mux_callback_info_t callback_info,
                                    mux_allocator_info_t allocator_info,
                                    mux_command_buffer_t *out_command_buffer) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (nullptr == out_command_buffer) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error = muxSelectCreateCommandBuffer(
      device, callback_info, allocator_info, out_command_buffer);

  if (mux_success == error) {
    mux::setId<mux_object_id_command_buffer>(device->info->id,
                                             *out_command_buffer);
  }

  return error;
}

mux_result_t muxFinalizeCommandBuffer(mux_command_buffer_t command_buffer) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);
  if (nullptr == command_buffer) {
    return mux_error_null_out_parameter;
  }

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  return muxSelectFinalizeCommandBuffer(command_buffer);
}

mux_result_t muxCloneCommandBuffer(mux_device_t device,
                                   mux_allocator_info_t allocator_info,
                                   mux_command_buffer_t command_buffer,
                                   mux_command_buffer_t *out_command_buffer) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return mux_error_null_allocator_callback;
  }

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (nullptr == out_command_buffer) {
    return mux_error_null_out_parameter;
  }

  const mux_result_t error = muxSelectCloneCommandBuffer(
      device, allocator_info, command_buffer, out_command_buffer);

  if (mux_success == error) {
    mux::setId<mux_object_id_command_buffer>(device->info->id,
                                             *out_command_buffer);
  }

  return error;
}

void muxDestroyCommandBuffer(mux_device_t device,
                             mux_command_buffer_t command_buffer,
                             mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(device)) {
    return;
  }

  if (mux::objectIsInvalid(command_buffer)) {
    return;
  }

  if (mux::allocatorInfoIsInvalid(allocator_info)) {
    return;
  }

  muxSelectDestroyCommandBuffer(device, command_buffer, allocator_info);
}

mux_result_t muxResetCommandBuffer(mux_command_buffer_t command_buffer) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  return muxSelectResetCommandBuffer(command_buffer);
}

mux_result_t muxCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t src_buffer, uint64_t src_offset,
                                  mux_buffer_t dst_buffer, uint64_t dst_offset,
                                  uint64_t size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(src_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(dst_buffer)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if ((size + src_offset) > src_buffer->memory_requirements.size) {
    return mux_error_invalid_value;
  }

  if ((size + dst_offset) > dst_buffer->memory_requirements.size) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandCopyBuffer(
      command_buffer, src_buffer, src_offset, dst_buffer, dst_offset, size,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandCopyBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_buffer_t dst_buffer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(src_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(dst_buffer)) {
    return mux_error_invalid_value;
  }

  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    if (0 == r.region.x) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.y) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.z) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.src_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.dst_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.src_desc.y) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.dst_desc.y) {
      return mux_error_invalid_value;
    }

    // check the last 1D row of the last slice
    {
      const size_t z = r.region.z - 1;
      const size_t y = r.region.y - 1;

      const size_t dst_slice_offset = (r.dst_origin.z + z) * r.dst_desc.y;
      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;

      const size_t dst_row_offset = (r.dst_origin.y + y) * r.dst_desc.x;
      const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;

      const size_t dst_offset =
          dst_slice_offset + dst_row_offset + r.dst_origin.x;
      const size_t src_offset =
          src_slice_offset + src_row_offset + r.src_origin.x;

      const size_t size = r.region.x;

      if ((src_offset + size) > src_buffer->memory_requirements.size) {
        return mux_error_invalid_value;
      }

      if ((dst_offset + size) > dst_buffer->memory_requirements.size) {
        return mux_error_invalid_value;
      }
    }
  }

  for (uint64_t i = 0; i < regions_length; ++i) {
    const auto &a = regions[i];

    for (uint64_t j = 0; j < regions_length; ++j) {
      // skip self check
      if (i == j) {
        continue;
      }

      const auto &b = regions[j];

      // single axis theorum - No overlap if any 1D projection fails
      if ((b.src_origin.x - a.src_origin.x) > a.region.x) {
        continue;
      }

      if ((b.src_origin.y - a.src_origin.y) > a.region.y) {
        continue;
      }

      if ((b.src_origin.z - a.src_origin.z) > a.region.z) {
        continue;
      }

      return mux_error_invalid_value;
    }

    for (uint64_t j = 0; j < regions_length; ++j) {
      // skip self check
      if (i == j) {
        continue;
      }

      const auto &b = regions[j];

      // single axis theorum - No overlap if any 1D projection fails
      if ((b.dst_origin.x - a.dst_origin.x) > a.region.x) {
        continue;
      }

      if ((b.dst_origin.y - a.dst_origin.y) > a.region.y) {
        continue;
      }

      if ((b.dst_origin.z - a.dst_origin.z) > a.region.z) {
        continue;
      }

      return mux_error_invalid_value;
    }
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandCopyBufferRegions(
      command_buffer, src_buffer, dst_buffer, regions, regions_length,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandFillBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t buffer, uint64_t offset,
                                  uint64_t size, const void *pattern_pointer,
                                  uint64_t pattern_size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if ((size + offset) > buffer->memory_requirements.size) {
    return mux_error_invalid_value;
  }

  if (nullptr == pattern_pointer) {
    return mux_error_invalid_value;
  }

  if (0 == pattern_size) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandFillBuffer(
      command_buffer, buffer, offset, size, pattern_pointer, pattern_size,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandReadBuffer(mux_command_buffer_t command_buffer,
                                  mux_buffer_t buffer, uint64_t offset,
                                  void *host_pointer, uint64_t size,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if ((size + offset) > buffer->memory_requirements.size) {
    return mux_error_invalid_value;
  }

  if (nullptr == host_pointer) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandReadBuffer(
      command_buffer, buffer, offset, host_pointer, size,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandReadBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (nullptr == host_pointer) {
    return mux_error_invalid_value;
  }

  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    if (0 == r.region.x) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.y) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.z) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.src_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.dst_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.src_desc.y) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.dst_desc.y) {
      return mux_error_invalid_value;
    }

    // check the last 1D row of the last slice
    {
      const size_t z = r.region.z - 1;
      const size_t y = r.region.y - 1;

      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;
      const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;
      const size_t src_offset =
          src_slice_offset + src_row_offset + r.src_origin.x;

      const size_t size = r.region.x;

      if ((src_offset + size) > buffer->memory_requirements.size) {
        return mux_error_invalid_value;
      }
    }
  }

  for (uint64_t i = 0; i < regions_length; ++i) {
    const auto &a = regions[i];

    for (uint64_t j = 0; j < regions_length; ++j) {
      // skip self check
      if (i == j) {
        continue;
      }

      const auto &b = regions[j];

      // single axis theorum - No overlap if any 1D projection fails
      if ((b.src_origin.x - a.dst_origin.x) > a.region.x) {
        continue;
      }

      if ((b.src_origin.y - a.dst_origin.y) > a.region.y) {
        continue;
      }

      if ((b.src_origin.z - a.dst_origin.z) > a.region.z) {
        continue;
      }

      return mux_error_invalid_value;
    }
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandReadBufferRegions(
      command_buffer, buffer, host_pointer, regions, regions_length,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandWriteBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   const void *host_pointer, uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (0 == size) {
    return mux_error_invalid_value;
  }

  if ((size + offset) > buffer->memory_requirements.size) {
    return mux_error_invalid_value;
  }

  if (nullptr == host_pointer) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandWriteBuffer(
      command_buffer, buffer, offset, host_pointer, size,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandWriteBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    const void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(buffer)) {
    return mux_error_invalid_value;
  }

  if (nullptr == host_pointer) {
    return mux_error_invalid_value;
  }

  for (uint64_t i = 0; i < regions_length; i++) {
    const auto &r = regions[i];

    if (0 == r.region.x) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.y) {
      return mux_error_invalid_value;
    }

    if (0 == r.region.z) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.src_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.x > r.dst_desc.x) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.src_desc.y) {
      return mux_error_invalid_value;
    }

    if (r.region.y > r.dst_desc.y) {
      return mux_error_invalid_value;
    }

    // check the last 1D row of the last slice
    {
      const size_t z = r.region.z - 1;
      const size_t y = r.region.y - 1;

      const size_t src_slice_offset = (r.src_origin.z + z) * r.src_desc.y;
      const size_t src_row_offset = (r.src_origin.y + y) * r.src_desc.x;
      const size_t src_offset =
          src_slice_offset + src_row_offset + r.src_origin.x;

      const size_t size = r.region.x;

      if ((src_offset + size) > buffer->memory_requirements.size) {
        return mux_error_invalid_value;
      }
    }
  }

  for (uint64_t i = 0; i < regions_length; ++i) {
    const auto &a = regions[i];

    for (uint64_t j = 0; j < regions_length; ++j) {
      // skip self check
      if (i == j) {
        continue;
      }

      const auto &b = regions[j];

      // single axis theorum - No overlap if any 1D projection fails
      if ((b.dst_origin.x - a.dst_origin.x) > a.region.x) {
        continue;
      }

      if ((b.dst_origin.y - a.dst_origin.y) > a.region.y) {
        continue;
      }

      if ((b.dst_origin.z - a.dst_origin.z) > a.region.z) {
        continue;
      }

      return mux_error_invalid_value;
    }
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandWriteBufferRegions(
      command_buffer, buffer, host_pointer, regions, regions_length,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandReadImage(mux_command_buffer_t command_buffer,
                                 mux_image_t image, mux_offset_3d_t offset,
                                 mux_extent_3d_t extent, uint64_t row_size,
                                 uint64_t slice_size, void *pointer,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(image)) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = image->size;
  if (image->array_layers) {
    switch (image->type) {
      default:
        break;
      case mux_image_type_1d:
        size.y = image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = image->array_layers;
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (offset.x + extent.x)) || (size.y < (offset.y + extent.y)) ||
      (size.z < (offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (nullptr == pointer) {
    return mux_error_null_out_parameter;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandReadImage(
      command_buffer, image, offset, extent, row_size, slice_size, pointer,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandWriteImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent, uint64_t row_size,
                                  uint64_t slice_size, const void *pointer,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(image)) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = image->size;
  if (image->array_layers) {
    switch (image->type) {
      default:
        break;
      case mux_image_type_1d:
        size.y = image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = image->array_layers;
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (offset.x + extent.x)) || (size.y < (offset.y + extent.y)) ||
      (size.z < (offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (nullptr == pointer) {
    return mux_error_null_out_parameter;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandWriteImage(
      command_buffer, image, offset, extent, row_size, slice_size, pointer,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandFillImage(mux_command_buffer_t command_buffer,
                                 mux_image_t image, const void *color,
                                 uint32_t color_size, mux_offset_3d_t offset,
                                 mux_extent_3d_t extent,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(image)) {
    return mux_error_invalid_value;
  }

  if (nullptr == color) {
    return mux_error_invalid_value;
  }

  if (0 == color_size) {
    return mux_error_invalid_value;
  }

  // the maximum color size is the maximum pixel type, 4 x 32 bit floating point
  // elements
  if (16 < color_size) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = image->size;
  if (image->array_layers) {
    switch (image->type) {
      case mux_image_type_1d:
        size.y = image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = image->array_layers;
        break;
      default:
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (offset.x + extent.x)) || (size.y < (offset.y + extent.y)) ||
      (size.z < (offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandFillImage(
      command_buffer, image, color, color_size, offset, extent,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandCopyImage(mux_command_buffer_t command_buffer,
                                 mux_image_t src_image, mux_image_t dst_image,
                                 mux_offset_3d_t src_offset,
                                 mux_offset_3d_t dst_offset,
                                 mux_extent_3d_s extent,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(src_image)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(dst_image)) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = src_image->size;
  if (src_image->array_layers) {
    switch (src_image->type) {
      case mux_image_type_1d:
        size.y = src_image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = src_image->array_layers;
        break;
      default:
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (src_offset.x + extent.x)) ||
      (size.y < (src_offset.y + extent.y)) ||
      (size.z < (src_offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  size = dst_image->size;
  if (dst_image->array_layers) {
    switch (dst_image->type) {
      case mux_image_type_1d:
        size.y = dst_image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = dst_image->array_layers;
        break;
      default:
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (dst_offset.x + extent.x)) ||
      (size.y < (dst_offset.y + extent.y)) ||
      (size.z < (dst_offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandCopyImage(
      command_buffer, src_image, dst_image, src_offset, dst_offset, extent,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(src_image)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(dst_buffer)) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = src_image->size;
  if (src_image->array_layers) {
    switch (src_image->type) {
      case mux_image_type_1d:
        size.y = src_image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = src_image->array_layers;
        break;
      default:
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (src_offset.x + extent.x)) ||
      (size.y < (src_offset.y + extent.y)) ||
      (size.z < (src_offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (dst_buffer->memory_requirements.size <
      (dst_offset + (src_image->pixel_size * extent.x * extent.y * extent.z))) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandCopyImageToBuffer(
      command_buffer, src_image, dst_buffer, src_offset, dst_offset, extent,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(src_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(dst_image)) {
    return mux_error_invalid_value;
  }

  // Check that the combined offset and extent does not exceed the buffer size.
  if (src_buffer->memory_requirements.size <
      (src_offset + (dst_image->pixel_size * extent.x * extent.y * extent.z))) {
    return mux_error_invalid_value;
  }

  mux_extent_3d_t size = dst_image->size;
  if (dst_image->array_layers) {
    switch (dst_image->type) {
      case mux_image_type_1d:
        size.y = dst_image->array_layers;
        break;
      case mux_image_type_2d:
        size.z = dst_image->array_layers;
        break;
      default:
        break;
    }
  }

  // Check that the combined offset and extent does not exceed the image size.
  if ((size.x < (dst_offset.x + extent.x)) ||
      (size.y < (dst_offset.y + extent.y)) ||
      (size.z < (dst_offset.z + extent.z))) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandCopyBufferToImage(
      command_buffer, src_buffer, dst_image, src_offset, dst_offset, extent,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandNDRange(mux_command_buffer_t command_buffer,
                               mux_kernel_t kernel,
                               mux_ndrange_options_t options,
                               uint32_t num_sync_points_in_wait_list,
                               const mux_sync_point_t *sync_point_wait_list,
                               mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(kernel)) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandNDRange(
      command_buffer, kernel, options, num_sync_points_in_wait_list,
      sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxUpdateDescriptors(mux_command_buffer_t command_buffer,
                                  mux_command_id_t command_id,
                                  uint64_t num_args, uint64_t *arg_indices,
                                  mux_descriptor_info_s *descriptors) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (0 == num_args) {
    return mux_error_invalid_value;
  }

  if (nullptr == arg_indices) {
    return mux_error_invalid_value;
  }

  if (nullptr == descriptors) {
    return mux_error_invalid_value;
  }

  return muxSelectUpdateDescriptors(command_buffer, command_id, num_args,
                                    arg_indices, descriptors);
}

mux_result_t muxCommandUserCallback(
    mux_command_buffer_t command_buffer,
    void (*user_function)(mux_queue_t queue,
                          mux_command_buffer_t command_buffer,
                          void *const user_data),
    void *user_data, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> guard(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (nullptr == user_function) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandUserCallback(
      command_buffer, user_function, user_data, num_sync_points_in_wait_list,
      sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandBeginQuery(mux_command_buffer_t command_buffer,
                                  mux_query_pool_t query_pool,
                                  uint32_t query_index, uint32_t query_count,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(query_pool)) {
    return mux_error_invalid_value;
  }

  if (query_index >= query_pool->count) {
    return mux_error_invalid_value;
  }

  if (query_count + query_index > query_pool->count) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandBeginQuery(
      command_buffer, query_pool, query_index, query_count,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandEndQuery(mux_command_buffer_t command_buffer,
                                mux_query_pool_t query_pool,
                                uint32_t query_index, uint32_t query_count,
                                uint32_t num_sync_points_in_wait_list,
                                const mux_sync_point_t *sync_point_wait_list,
                                mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(query_pool)) {
    return mux_error_invalid_value;
  }

  if (query_index >= query_pool->count) {
    return mux_error_invalid_value;
  }

  if (query_count + query_index > query_pool->count) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandEndQuery(
      command_buffer, query_pool, query_index, query_count,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}

mux_result_t muxCommandResetQueryPool(
    mux_command_buffer_t command_buffer, mux_query_pool_t query_pool,
    uint32_t query_index, uint32_t query_count,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(command_buffer)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(query_pool)) {
    return mux_error_invalid_value;
  }

  if (query_index >= query_pool->count ||
      (query_count + query_index) > query_pool->count) {
    return mux_error_invalid_value;
  }

  if (waitlistIsInvalid(num_sync_points_in_wait_list, sync_point_wait_list)) {
    return mux_error_invalid_value;
  }

  const mux_result_t error = muxSelectCommandResetQueryPool(
      command_buffer, query_pool, query_index, query_count,
      num_sync_points_in_wait_list, sync_point_wait_list, sync_point);
  if (mux_success == error && nullptr != sync_point) {
    mux::setId<mux_object_id_sync_point>(command_buffer->device->info->id,
                                         *sync_point);
  }

  return error;
}
