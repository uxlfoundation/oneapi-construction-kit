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
#include <host/device.h>
#include <host/host.h>
#include <host/image.h>
#include <host/memory.h>
#include <mux/utils/allocator.h>

#include <cstdlib>

namespace host {
image_s::image_s(mux_memory_requirements_s memory_requirements,
                 mux_image_type_e type, mux_image_format_e format,
                 uint32_t pixel_size, uint32_t width, uint32_t height,
                 uint32_t depth, uint32_t array_layers, uint64_t row_size,
                 uint64_t slice_size) {
  this->memory_requirements = memory_requirements;
  this->type = type;
  this->format = format;
  this->pixel_size = pixel_size;
  this->size = {width, height, depth};
  this->array_layers = array_layers;
  this->row_size = row_size;
  this->slice_size = slice_size;
  this->tiling = mux_image_tiling_linear;
}
}  // namespace host

#ifdef HOST_IMAGE_SUPPORT
namespace {
inline cl_image_format getImageFormat(mux_image_format_e format) {
  return {static_cast<cl_channel_order>(format & 0xffff),
          static_cast<cl_channel_type>((format & 0xffff0000) >> 16)};
}

inline mux_image_format_e setImageFormat(const cl_image_format &format) {
  return static_cast<mux_image_format_e>(
      format.image_channel_order | (format.image_channel_data_type << 16));
}

inline cl_image_desc getImageDesc(mux_image_type_e type, uint32_t width,
                                  uint32_t height, uint32_t depth,
                                  uint32_t array_layers, uint64_t row_size,
                                  uint64_t slice_size) {
  cl_mem_object_type imageType;
  switch (type) {
    case mux_image_type_1d:
      imageType =
          (array_layers) ? CL_MEM_OBJECT_IMAGE1D_ARRAY : CL_MEM_OBJECT_IMAGE1D;
      break;
    case mux_image_type_2d:
      imageType =
          (array_layers) ? CL_MEM_OBJECT_IMAGE2D_ARRAY : CL_MEM_OBJECT_IMAGE2D;
      break;
    case mux_image_type_3d:
      imageType = CL_MEM_OBJECT_IMAGE3D;
      break;
    default:
      abort();
  }
  const size_t rowPitch = static_cast<size_t>(row_size);
  const size_t slicePitch = static_cast<size_t>(slice_size);
  return {imageType, width,      height, depth, array_layers,
          rowPitch,  slicePitch, 0,      0,     {/*nullptr*/}};
}
}  // namespace
#endif

mux_result_t hostCreateImage(mux_device_t device, mux_image_type_e type,
                             mux_image_format_e format, uint32_t width,
                             uint32_t height, uint32_t depth,
                             uint32_t array_layers, uint64_t row_size,
                             uint64_t slice_size,
                             mux_allocator_info_t allocator_info,
                             mux_image_t *out_image) {
#ifdef HOST_IMAGE_SUPPORT
  (void)device;
  mux::allocator allocator(allocator_info);

  const cl_image_format imageFormat = getImageFormat(format);
  const cl_image_desc imageDesc = getImageDesc(
      type, width, height, depth, array_layers, row_size, slice_size);

  const uint64_t storageSize =
      libimg::HostGetImageStorageSize(imageFormat, imageDesc);

  // NOTE: Set the required alignment per image format, this may be too
  // granular however it is not possible to simply use a course value such at 16
  // because of CL_MEM_USE_HOST_PTR and the alignment requirements of the OpenCL
  // specification.
  uint32_t alignment = 4;
  switch (format) {
    // 1 byte
    case mux_image_format_A8_sint:
    case mux_image_format_A8_snorm:
    case mux_image_format_A8_uint:
    case mux_image_format_A8_unorm:
    case mux_image_format_INTENSITY8_snorm:
    case mux_image_format_INTENSITY8_unorm:
    case mux_image_format_LUMINANCE8_snorm:
    case mux_image_format_LUMINANCE8_unorm:
    case mux_image_format_R8_sint:
    case mux_image_format_R8_snorm:
    case mux_image_format_R8_uint:
    case mux_image_format_R8_unorm:
      alignment = 1;
      break;

    // 2 bytes
    case mux_image_format_A16_sfloat:
    case mux_image_format_A16_sint:
    case mux_image_format_A16_snorm:
    case mux_image_format_A16_uint:
    case mux_image_format_A16_unorm:
    case mux_image_format_INTENSITY16_sfloat:
    case mux_image_format_INTENSITY16_snorm:
    case mux_image_format_INTENSITY16_unorm:
    case mux_image_format_LUMINANCE16_sfloat:
    case mux_image_format_LUMINANCE16_snorm:
    case mux_image_format_LUMINANCE16_unorm:
    case mux_image_format_R16_sfloat:
    case mux_image_format_R16_sint:
    case mux_image_format_R16_snorm:
    case mux_image_format_R16_uint:
    case mux_image_format_R16_unorm:
    case mux_image_format_R5G5B5_unorm_pack16:
    case mux_image_format_R5G5B5x1_unorm_pack16:
    case mux_image_format_R5G6B5_unorm_pack16:
    case mux_image_format_R5G6B5x0_unorm_pack16:
    case mux_image_format_R8A8_sint:
    case mux_image_format_R8A8_snorm:
    case mux_image_format_R8A8_uint:
    case mux_image_format_R8A8_unorm:
    case mux_image_format_R8G8_sint:
    case mux_image_format_R8G8_snorm:
    case mux_image_format_R8G8_uint:
    case mux_image_format_R8G8_unorm:
    case mux_image_format_R8x8_sint:
    case mux_image_format_R8x8_snorm:
    case mux_image_format_R8x8_uint:
    case mux_image_format_R8x8_unorm:
    case mux_image_format_R8G8Bx_sint:
    case mux_image_format_R8G8Bx_snorm:
    case mux_image_format_R8G8Bx_uint:
    case mux_image_format_R8G8Bx_unorm:
      alignment = 2;
      break;

    // 4 bytes
    case mux_image_format_A32_sfloat:
    case mux_image_format_A32_sint:
    case mux_image_format_A32_uint:
    case mux_image_format_A8R8G8B8_sint:
    case mux_image_format_A8R8G8B8_snorm:
    case mux_image_format_A8R8G8B8_uint:
    case mux_image_format_A8R8G8B8_unorm:
    case mux_image_format_B8G8R8A8_sint:
    case mux_image_format_B8G8R8A8_snorm:
    case mux_image_format_B8G8R8A8_uint:
    case mux_image_format_B8G8R8A8_unorm:
    case mux_image_format_INTENSITY32_sfloat:
    case mux_image_format_LUMINANCE32_sfloat:
    case mux_image_format_R10G10B10_unorm_pack32:
    case mux_image_format_R10G10B10x2_unorm_pack32:
    case mux_image_format_R16A16_sfloat:
    case mux_image_format_R16A16_sint:
    case mux_image_format_R16A16_snorm:
    case mux_image_format_R16A16_uint:
    case mux_image_format_R16A16_unorm:
    case mux_image_format_R16G16_sfloat:
    case mux_image_format_R16G16_sint:
    case mux_image_format_R16G16_snorm:
    case mux_image_format_R16G16_uint:
    case mux_image_format_R16G16_unorm:
    case mux_image_format_R16x16_sfloat:
    case mux_image_format_R16x16_sint:
    case mux_image_format_R16x16_snorm:
    case mux_image_format_R16x16_uint:
    case mux_image_format_R16x16_unorm:
    case mux_image_format_R32_sfloat:
    case mux_image_format_R32_sint:
    case mux_image_format_R32_uint:
    case mux_image_format_R8G8B8A8_sint:
    case mux_image_format_R8G8B8A8_snorm:
    case mux_image_format_R8G8B8A8_uint:
    case mux_image_format_R8G8B8A8_unorm:
      alignment = 4;
      break;

    // 8 bytes, 3 channel formats are promoted due to three element vectors
    // being the size of 4 element vectors in OpenCL.
    case mux_image_format_R16G16B16_sfloat:
    case mux_image_format_R16G16B16_sint:
    case mux_image_format_R16G16B16_snorm:
    case mux_image_format_R16G16B16_unorm:
    case mux_image_format_R16G16B16A16_sfloat:
    case mux_image_format_R16G16B16A16_sint:
    case mux_image_format_R16G16B16A16_snorm:
    case mux_image_format_R16G16B16A16_uint:
    case mux_image_format_R16G16B16A16_unorm:
    case mux_image_format_R16G16B16B16_uint:
    case mux_image_format_R32A32_sfloat:
    case mux_image_format_R32A32_sint:
    case mux_image_format_R32A32_uint:
    case mux_image_format_R32G32_sfloat:
    case mux_image_format_R32G32_sint:
    case mux_image_format_R32G32_uint:
    case mux_image_format_R32x32_sfloat:
    case mux_image_format_R32x32_sint:
    case mux_image_format_R32x32_uint:
      alignment = 8;
      break;

    // 16 bytes, 3 channel formats are promoted due to three element vectors
    // being the size of 4 element vectors in OpenCL.
    case mux_image_format_R32G32B32_sfloat:
    case mux_image_format_R32G32B32_sint:
    case mux_image_format_R32G32B32_uint:
    case mux_image_format_R32G32B32A32_sfloat:
    case mux_image_format_R32G32B32A32_sint:
    case mux_image_format_R32G32B32A32_uint:
      alignment = 16;
      break;
  }

  // TODO: Also report host::memory_s::HEAP_ANY
  const mux_memory_requirements_s memoryRequirements = {
      storageSize, alignment, host::memory_s::HEAP_IMAGE};

  const uint64_t pixelSize = libimg::HostGetPixelSize(imageFormat);

  auto image = allocator.create<host::image_s>(
      memoryRequirements, type, format, pixelSize, width, height, depth,
      array_layers, row_size, slice_size);
  if (nullptr == image) {
    return mux_error_out_of_memory;
  }

  // NOTE: Initialize libimg::HostImage to store the relevant image description,
  // we do not use libimg::HostCreateImage as we can not attach the storage
  // until ::hostBinaryImageMemory time, libimg::HostInitializeImage will
  // instead set the storage type to external so we can bind later.
  libimg::HostInitializeImage(imageFormat, imageDesc, &image->image);

  *out_image = image;

  return mux_success;
#else
  (void)device;
  (void)type;
  (void)format;
  (void)width;
  (void)height;
  (void)depth;
  (void)array_layers;
  (void)row_size;
  (void)slice_size;
  (void)allocator_info;
  (void)*out_image;

  return mux_error_feature_unsupported;
#endif
}

void hostDestroyImage(mux_device_t device, mux_image_t image,
                      mux_allocator_info_t allocator_info) {
  (void)device;
#ifdef HOST_IMAGE_SUPPORT
  mux::allocator allocator(allocator_info);
  auto hostImage = static_cast<host::image_s *>(image);
  allocator.destroy(hostImage);
#else
  (void)image;
  (void)allocator_info;
#endif
}

mux_result_t hostBindImageMemory(mux_device_t device, mux_memory_t memory,
                                 mux_image_t image, uint64_t offset) {
  (void)device;
#ifdef HOST_IMAGE_SUPPORT

  auto hostMemory = static_cast<host::memory_s *>(memory);
  auto hostImage = static_cast<host::image_s *>(image);

  // NOTE: The alignment of the the provided device memory and offset may be
  // incorrect at this point, but the spec does not require
  // an error message for this case, so no extra checking is required
  char *pointer = static_cast<char *>(hostMemory->data) + offset;

  // NOTE: Bind the device memory
  libimg::HostAttachImageStorage(&hostImage->image, pointer);

  return mux_success;
#else
  (void)memory;
  (void)image;
  (void)offset;

  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostGetSupportedImageFormats(mux_device_t device,
                                          mux_image_type_e image_type,
                                          mux_allocation_type_e allocation_type,
                                          uint32_t count,
                                          mux_image_format_e *out_formats,
                                          uint32_t *out_count) {
  (void)device;

#ifdef HOST_IMAGE_SUPPORT
  if (nullptr == out_formats && nullptr == out_count) {
    // NOTE: Exit early as we don't need to do anything.
    return mux_success;
  }

  cl_mem_flags imgFlags = CL_MEM_READ_WRITE;
  switch (allocation_type) {
    case mux_allocation_type_alloc_host:
      imgFlags |= CL_MEM_ALLOC_HOST_PTR;
      break;
    case mux_allocation_type_alloc_device:
      break;
  }

  cl_mem_object_type imgType;
  switch (image_type) {
    default:  // NOTE: We should never hit this case.
    case mux_image_type_1d:
      imgType = CL_MEM_OBJECT_IMAGE1D;
      break;
    case mux_image_type_2d:
      imgType = CL_MEM_OBJECT_IMAGE2D;
      break;
    case mux_image_type_3d:
      imgType = CL_MEM_OBJECT_IMAGE3D;
      break;
  }

  uint32_t imgCount;
  auto error = libimg::HostGetSupportedImageFormats(imgFlags, imgType, 0,
                                                    nullptr, &imgCount);
  // NOTE: Ensure that 128 is large enough for the list of image formats, this
  // should never happen unless the libimg implementation changes..
  if (128 < imgCount) {
    return mux_error_internal;
  }

  if (error) {
    return mux_error_failure;
  }

  if (out_count) {
    *out_count = imgCount;
  }

  if (out_formats) {
    cl_image_format imgFormats[128];
    error = libimg::HostGetSupportedImageFormats(imgFlags, imgType, count,
                                                 imgFormats, nullptr);
    if (error) {
      return mux_error_failure;
    }

    for (uint32_t index = 0; index < count; ++index) {
      out_formats[index] = setImageFormat(imgFormats[index]);
    }
  }

  return mux_success;
#else
  (void)device;
  (void)image_type;
  (void)allocation_type;
  (void)count;
  (void)*out_formats;
  (void)*out_count;

  return mux_error_feature_unsupported;
#endif
}
