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

#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/device.h>
#include <cl/image.h>
#include <cl/validate.h>
#include <libimg/host.h>
#include <libimg/validate.h>

namespace cl {
namespace validate {

cl_int FillBufferArguments(cl_command_queue command_queue, cl_mem buffer,
                           const void *pattern, size_t pattern_size,
                           size_t offset, size_t size) {
  OCL_CHECK(!buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(command_queue->context != buffer->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(!pattern, return CL_INVALID_VALUE);

  OCL_CHECK(offset > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(offset + size > buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(pattern_size == 0, return CL_INVALID_VALUE);

  bool valid = false;
  for (uint32_t i = 1; i <= 128; i *= 2) {
    OCL_CHECK(pattern_size == i, valid = true; break);
  }
  OCL_CHECK(!valid, return CL_INVALID_VALUE);

  OCL_CHECK(offset % pattern_size != 0, return CL_INVALID_VALUE);
  OCL_CHECK(size % pattern_size != 0, return CL_INVALID_VALUE);

  return CL_SUCCESS;
}

cl_int CopyBufferRectArguments(cl_command_queue command_queue,
                               cl_mem src_buffer, cl_mem dst_buffer,
                               const size_t *src_origin,
                               const size_t *dst_origin, const size_t *region,
                               size_t src_row_pitch, size_t src_slice_pitch,
                               size_t dst_row_pitch, size_t dst_slice_pitch) {
  OCL_CHECK(!src_buffer || !dst_buffer, return CL_INVALID_MEM_OBJECT);
  cl_context src_context = src_buffer->context;
  cl_context dst_context = dst_buffer->context;
  OCL_CHECK(!src_origin || !dst_origin || !region, return CL_INVALID_VALUE);
  OCL_CHECK(command_queue->context != src_context, return CL_INVALID_CONTEXT);
  OCL_CHECK(command_queue->context != dst_context, return CL_INVALID_CONTEXT);

  OCL_CHECK(region[0] == 0 || region[1] == 0 || region[2] == 0,
            return CL_INVALID_VALUE);

  // It's valid for these to be zero as passed to the entry point, but by the
  // time we're validating them here they should have been assigned values
  // derived from `region`.
  OCL_CHECK(src_row_pitch == 0 || src_slice_pitch == 0 || dst_row_pitch == 0 ||
                dst_slice_pitch == 0,
            return CL_INVALID_VALUE);

  OCL_CHECK((((src_origin[2] + region[2] - 1) * src_slice_pitch) +
             ((src_origin[1] + region[1] - 1) * src_row_pitch) +
             (src_origin[0] + region[0])) > src_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK((((dst_origin[2] + region[2] - 1) * dst_slice_pitch) +
             ((dst_origin[1] + region[1] - 1) * dst_row_pitch) +
             (dst_origin[0] + region[0])) > dst_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(region[0] * region[1] * region[2] > src_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(region[0] * region[1] * region[2] > dst_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK((src_origin[0] + region[0]) * (src_origin[1] + region[1]) *
                    (src_origin[2] + region[2]) >
                src_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK((dst_origin[0] + region[0]) * (dst_origin[1] + region[1]) *
                    (dst_origin[2] + region[2]) >
                dst_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(src_row_pitch != 0 && src_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK(dst_row_pitch != 0 && dst_row_pitch < region[0],
            return CL_INVALID_VALUE);
  OCL_CHECK(src_slice_pitch != 0 && src_slice_pitch < region[1] * src_row_pitch,
            return CL_INVALID_VALUE);
  OCL_CHECK(dst_slice_pitch != 0 && dst_slice_pitch < region[1] * dst_row_pitch,
            return CL_INVALID_VALUE);
  OCL_CHECK(src_slice_pitch != 0 && src_slice_pitch % src_row_pitch != 0,
            return CL_INVALID_VALUE);
  OCL_CHECK(dst_slice_pitch != 0 && dst_slice_pitch % dst_row_pitch != 0,
            return CL_INVALID_VALUE);
  OCL_CHECK(src_buffer == dst_buffer && (src_row_pitch != dst_row_pitch ||
                                         src_slice_pitch != dst_slice_pitch),
            return CL_INVALID_VALUE);

  return CL_SUCCESS;
}

cl_int CopyBufferArguments(cl_command_queue command_queue, cl_mem src_buffer,
                           cl_mem dst_buffer, size_t src_offset,
                           size_t dst_offset, size_t size) {
  OCL_CHECK(!src_buffer || !dst_buffer, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(dst_buffer->context != src_buffer->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(command_queue->context != src_buffer->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(src_offset > src_buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(dst_offset > dst_buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(size > src_buffer->size || size > dst_buffer->size,
            return CL_INVALID_VALUE);
  OCL_CHECK(src_offset + size > src_buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(dst_offset + size > dst_buffer->size, return CL_INVALID_VALUE);
  OCL_CHECK(size == 0, return CL_INVALID_VALUE);
  size_t src_start = src_offset;
  size_t dst_start = dst_offset;
  size_t src_buffer_size = 0;
  size_t dst_buffer_size = 0;
  if (src_buffer == dst_buffer) {
    src_buffer_size = src_buffer->size;
    dst_buffer_size = dst_buffer->size;
  } else if (src_buffer == dst_buffer->optional_parent) {
    // dst has a parent, so must be a sub-buffer, add the sub-buffer's offset.
    auto dst_sub_buffer = static_cast<cl_mem_buffer>(dst_buffer);
    dst_start += dst_sub_buffer->offset;
    src_buffer_size = src_buffer->size;
    dst_buffer_size = dst_buffer->optional_parent->size;
  } else if (src_buffer->optional_parent == dst_buffer) {
    // src has a parent, so must be a sub-buffer, add the sub-buffer's offset.
    auto src_sub_buffer = static_cast<cl_mem_buffer>(src_buffer);
    src_start += src_sub_buffer->offset;
    src_buffer_size = src_buffer->optional_parent->size;
    dst_buffer_size = dst_buffer->size;
  } else if (src_buffer->optional_parent && dst_buffer->optional_parent &&
             (src_buffer->optional_parent == dst_buffer->optional_parent)) {
    // src and dst have parents, so must be sub-buffers, add the offsets.
    auto src_sub_buffer = static_cast<cl_mem_buffer>(src_buffer);
    auto dst_sub_buffer = static_cast<cl_mem_buffer>(dst_buffer);
    src_start += src_sub_buffer->offset;
    dst_start += dst_sub_buffer->offset;
    src_buffer_size = src_buffer->optional_parent->size;
    dst_buffer_size = dst_buffer->optional_parent->size;
  }
  if (src_buffer_size && dst_buffer_size) {
// Our copy is within regions of the same buffer, check that there is no
// overlap between the source and destination regions.  Note that we treat
// ranges here as right-open intervals, i.e [START, END).
#define IN_RANGE(X, START, END) (((X) >= (START)) && ((X) < (END)))
#define RANGES_OVERLAP(STARTX, ENDX, STARTY, ENDY) \
  (IN_RANGE(STARTY, STARTX, ENDX) || IN_RANGE((ENDY) - 1, STARTX, ENDX))
    OCL_CHECK(RANGES_OVERLAP(src_start, src_start + size, dst_start,
                             dst_start + size),
              return CL_MEM_COPY_OVERLAP);
#undef IN_RANGE
#undef RANGES_OVERLAP
  }

  return CL_SUCCESS;
}

cl_int FillImageArguments(cl_command_queue command_queue, cl_mem image_,
                          const void *fill_color, const size_t *origin,
                          const size_t *region) {
  OCL_CHECK(!command_queue->device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!image_, return CL_INVALID_MEM_OBJECT);

  auto image = static_cast<cl_mem_image>(image_);
  OCL_CHECK(command_queue->context != image->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(!fill_color, return CL_INVALID_VALUE);
  OCL_CHECK(!origin, return CL_INVALID_VALUE);
  OCL_CHECK(!region, return CL_INVALID_VALUE);

  const size_t origin_offset = libimg::HostGetImageOriginOffset(
      image->image_format, image->image_desc, origin);

  const size_t region_size = libimg::HostGetImageRegionSize(
      image->image_format, image->image_desc.image_type, region);
  OCL_CHECK(image->size < origin_offset || image->size < region_size ||
                image->size < (origin_offset + region_size),
            return CL_INVALID_VALUE);
  cl_int error =
      libimg::ValidateOriginAndRegion(image->image_desc, origin, region);
  OCL_CHECK(error, return error);

  auto device = command_queue->device;
  error = libimg::ValidateImageSize(
      image->image_desc, device->image2d_max_width, device->image2d_max_height,
      device->image3d_max_width, device->image3d_max_height,
      device->image3d_max_depth, device->image_max_array_size,
      device->image_max_buffer_size);
  OCL_CHECK(error, return error);

  // TODO: This check should go through the device.
  error = libimg::ValidateImageFormat(image->image_format);
  OCL_CHECK(error, return error);

  return CL_SUCCESS;
}

cl_int CopyImageArguments(cl_command_queue command_queue, cl_mem src_image_,
                          cl_mem dst_image_, const size_t *src_origin,
                          const size_t *dst_origin, const size_t *region) {
  OCL_CHECK(!command_queue->device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!src_image_ || !dst_image_, return CL_INVALID_MEM_OBJECT);
  auto src_image = static_cast<cl_mem_image>(src_image_);
  auto dst_image = static_cast<cl_mem_image>(dst_image_);
  OCL_CHECK(!src_origin || !dst_origin || !region, return CL_INVALID_VALUE);
  OCL_CHECK(command_queue->context != src_image->context ||
                command_queue->context != dst_image->context,
            return CL_INVALID_CONTEXT);
  cl_int error = libimg::ValidateImageFormat(src_image->image_format);
  OCL_CHECK(error, return error);
  error = libimg::ValidateImageFormat(dst_image->image_format);
  OCL_CHECK(error, return error);
  error = libimg::ValidateImageFormatMismatch(src_image->image_format,
                                              dst_image->image_format);
  OCL_CHECK(error, return error);
  error = libimg::ValidateOriginAndRegion(src_image->image_desc, src_origin,
                                          region);
  OCL_CHECK(error, return error);
  error = libimg::ValidateOriginAndRegion(dst_image->image_desc, dst_origin,
                                          region);
  OCL_CHECK(error, return error);
  auto device = command_queue->device;
  error = libimg::ValidateImageSize(
      src_image->image_desc, device->image2d_max_width,
      device->image2d_max_height, device->image3d_max_width,
      device->image3d_max_height, device->image3d_max_depth,
      device->image_max_array_size, device->image_max_buffer_size);
  OCL_CHECK(error, return error);
  error = libimg::ValidateImageSize(
      dst_image->image_desc, device->image2d_max_width,
      device->image2d_max_height, device->image3d_max_width,
      device->image3d_max_height, device->image3d_max_depth,
      device->image_max_array_size, device->image_max_buffer_size);
  OCL_CHECK(error, return error);
  if (src_image_ == dst_image_) {
    error = libimg::ValidateNoOverlap(src_image->image_desc, src_origin,
                                      dst_origin, region);
    OCL_CHECK(error, return error);
  }

  return CL_SUCCESS;
}

cl_int CopyImageToBufferArguments(cl_command_queue command_queue,
                                  cl_mem src_image_, cl_mem dst_buffer_,
                                  const size_t *src_origin,
                                  const size_t *region, size_t dst_offset) {
  OCL_CHECK(!src_image_, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!dst_buffer_, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(command_queue->context != src_image_->context ||
                command_queue->context != dst_buffer_->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(!src_origin || !region, return CL_INVALID_VALUE);

  auto src_image = static_cast<cl_mem_image>(src_image_);
  auto dst_buffer = static_cast<cl_mem_buffer>(dst_buffer_);

  const cl_image_format &src_image_format = src_image->image_format;
  const cl_image_desc &src_image_desc = src_image->image_desc;

  const size_t src_origin_offset = libimg::HostGetImageOriginOffset(
      src_image_format, src_image_desc, src_origin);

  const size_t src_image_size = src_image->size;
  const size_t dst_buffer_size = dst_buffer->size;
  const size_t region_size = libimg::HostGetImageRegionSize(
      src_image_format, src_image_desc.image_type, region);

  OCL_CHECK(src_image_size < src_origin_offset, return CL_INVALID_VALUE);
  OCL_CHECK(src_image_size < (src_origin_offset + region_size),
            return CL_INVALID_VALUE);
  OCL_CHECK(dst_buffer_size < dst_offset, return CL_INVALID_VALUE);
  OCL_CHECK(dst_buffer_size < (dst_offset + region_size),
            return CL_INVALID_VALUE);

  cl_int error =
      libimg::ValidateOriginAndRegion(src_image_desc, src_origin, region);
  OCL_CHECK(error, return error);

  auto device = command_queue->device;
  if (dst_buffer->optional_parent) {
    OCL_CHECK(0 != (dst_buffer->offset % device->mem_base_addr_align),
              return CL_MISALIGNED_SUB_BUFFER_OFFSET);
  }

  error = libimg::ValidateImageSize(
      src_image_desc, device->image2d_max_width, device->image2d_max_height,
      device->image3d_max_width, device->image3d_max_height,
      device->image3d_max_depth, device->image_max_array_size,
      device->image_max_buffer_size);
  OCL_CHECK(error, return error);

  error = libimg::ValidateImageFormat(src_image_format);
  OCL_CHECK(error, return error);

  return CL_SUCCESS;
}

cl_int CopyBufferToImageArguments(cl_command_queue command_queue,
                                  cl_mem src_buffer_, cl_mem dst_image_,
                                  size_t src_offset, const size_t *dst_origin,
                                  const size_t *region) {
  auto device = command_queue->device;
  OCL_CHECK(!device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!src_buffer_ || !dst_image_, return CL_INVALID_MEM_OBJECT);
  auto src_buffer = static_cast<cl_mem_buffer>(src_buffer_);
  auto dst_image = static_cast<cl_mem_image>(dst_image_);
  OCL_CHECK(command_queue->context != src_buffer->context ||
                command_queue->context != dst_image->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(!dst_origin || !region, return CL_INVALID_VALUE);

  const size_t region_size = libimg::HostGetImageRegionSize(
      dst_image->image_format, dst_image->image_desc.image_type, region);
  OCL_CHECK(src_buffer->size < src_offset ||
                src_buffer->size < src_offset + region_size,
            return CL_INVALID_VALUE);
  const size_t dst_origin_offset = libimg::HostGetImageOriginOffset(
      dst_image->image_format, dst_image->image_desc, dst_origin);
  OCL_CHECK(dst_image->size < dst_origin_offset ||
                dst_image->size < dst_origin_offset + region_size,
            return CL_INVALID_VALUE);
  cl_int error = libimg::ValidateOriginAndRegion(dst_image->image_desc,
                                                 dst_origin, region);
  OCL_CHECK(error, return error);
  if (src_buffer->optional_parent) {
    OCL_CHECK(0 != (src_buffer->offset % device->mem_base_addr_align),
              return CL_MISALIGNED_SUB_BUFFER_OFFSET);
  }
  error = libimg::ValidateImageSize(
      dst_image->image_desc, device->image2d_max_width,
      device->image2d_max_height, device->image3d_max_width,
      device->image3d_max_height, device->image3d_max_depth,
      device->image_max_array_size, device->image_max_buffer_size);
  OCL_CHECK(error, return error);
  error = libimg::ValidateImageFormat(dst_image->image_format);
  OCL_CHECK(error, return error);

  return CL_SUCCESS;
}

}  // namespace validate
}  // namespace cl
