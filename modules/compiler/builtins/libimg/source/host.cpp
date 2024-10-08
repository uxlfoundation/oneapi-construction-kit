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

// Hello future programmer who is using this library in a context where the
// 'cargo' library is not available.  We include this header to use
// cargo::bit_cast, you could just consider copying that function into this
// file?  Or maybe it is now the far future and there is a std::bit_cast.
#include <cargo/utility.h>

#include <libimg/host.h>
#include <libimg/shared.h>
#include <libimg/validate.h>

#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>

// Assuming that max pixel vector can be float4 or int4 or uint4.
const uint64_t alignment_padding = sizeof(cl_float4) - 1u;

static uint64_t HostImageAlignedImageSize(uint64_t header_size,
                                          uint64_t raw_data_size) {
  return header_size + raw_data_size + alignment_padding;
}

template <typename T>
static T *HostImageAlignAddress(T *unaligned_raw_data) {
  uintptr_t aligned_raw_data = reinterpret_cast<uintptr_t>(unaligned_raw_data);
  aligned_raw_data += (sizeof(cl_float4) - 1u);
  aligned_raw_data &= ~(sizeof(cl_float4) - 1u);
  T *raw_data = reinterpret_cast<T *>(aligned_raw_data);

  return raw_data;
}

// Returns pointer to raw data storage if image would store the data kInternal.
static uint8_t *HostImageRawDataStorageInternalAddress(
    libimg::HostImage *image) {
  // No validation to prevent infinite recursive loop when called from
  // IsHostImageValid.
  uint8_t *host_image = reinterpret_cast<uint8_t *>(image);
  uint8_t *unaligned_raw_data = host_image + sizeof(*image);

  return HostImageAlignAddress(unaligned_raw_data);
}

libimg::HostSampler libimg::HostCreateSampler(
    const cl_bool normalized_coordinates,
    const cl_addressing_mode addressing_mode,
    const cl_filter_mode filter_mode) {
  uint32_t sampler_value = 0u;
  switch (normalized_coordinates) {
    default:
      sampler_value |= 0;
      break;
    case CL_FALSE:
      sampler_value |= CLK_NORMALIZED_COORDS_FALSE;
      break;
    case CL_TRUE:
      sampler_value |= CLK_NORMALIZED_COORDS_TRUE;
      break;
  }

  switch (addressing_mode) {
    default:
      sampler_value |= 0;
      break;
    case CL_ADDRESS_NONE:
      sampler_value |= CLK_ADDRESS_NONE;
      break;
    case CL_ADDRESS_CLAMP_TO_EDGE:
      sampler_value |= CLK_ADDRESS_CLAMP_TO_EDGE;
      break;
    case CL_ADDRESS_CLAMP:
      sampler_value |= CLK_ADDRESS_CLAMP;
      break;
    case CL_ADDRESS_REPEAT:
      sampler_value |= CLK_ADDRESS_REPEAT;
      break;
    case CL_ADDRESS_MIRRORED_REPEAT:
      sampler_value |= CLK_ADDRESS_MIRRORED_REPEAT;
      break;
  }

  switch (filter_mode) {
    default:
      sampler_value |= 0;
      break;
    case CL_FILTER_NEAREST:
      sampler_value |= CLK_FILTER_NEAREST;
      break;
    case CL_FILTER_LINEAR:
      sampler_value |= CLK_FILTER_LINEAR;
      break;
  }
  return sampler_value;
}

size_t libimg::HostGetImageOriginOffset(const cl_image_format &format,
                                        const cl_image_desc &desc,
                                        const size_t origin[3]) {
  const size_t pixel_size = libimg::HostGetPixelSize(format);
  const size_t row_position = origin[0] * pixel_size;
  switch (desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      return row_position;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    case CL_MEM_OBJECT_IMAGE2D:
      return row_position + (desc.image_row_pitch * origin[1]);
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE3D:
      return row_position + (desc.image_row_pitch * origin[1]) +
             (desc.image_slice_pitch * origin[2]);
    default:
      IMG_UNREACHABLE("Not an image mem object type!");
      return 0;
  }
}

size_t libimg::HostGetImageRegionSize(const cl_image_format &format,
                                      const cl_mem_object_type type,
                                      const size_t region[3]) {
  const size_t row_size = region[0] * libimg::HostGetPixelSize(format);
  switch (type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      return row_size;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    case CL_MEM_OBJECT_IMAGE2D:
      return row_size * region[1];
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE3D:
      return row_size * region[1] * region[2];
    default:
      IMG_UNREACHABLE("Not an image mem object type!");
      break;
  }

  return 0;
}

size_t libimg::HostGetPixelSize(const cl_image_format &image_format) {
  size_t component_count = 0;
  switch (image_format.image_channel_order) {
    case CLK_R:          // Fall through.
    case CLK_A:          // Fall through.
    case CLK_Rx:         // Fall through.
    case CLK_LUMINANCE:  // Fall through.
    case CLK_INTENSITY:  // Fall through.
    case CLK_RGB:        // Fall through, special case for masked formats.
    case CLK_RGBx:       // Fall through, special case for masked formats.
      component_count = 1;
      break;
    case CLK_RG:   // Fall through.
    case CLK_RA:   // Fall through.
    case CLK_RGx:  // Fall through.
      component_count = 2;
      break;
    case CLK_RGBA:  // Fall through.
    case CLK_BGRA:  // Fall through.
    case CLK_ARGB:  // Fall through.
      component_count = 4;
      break;
    default:
      IMG_UNREACHABLE("Unknown channel order.");
      break;
  }

  size_t component_size = 0;
  switch (image_format.image_channel_data_type) {
    case CLK_SNORM_INT8:   // Fall through.
    case CLK_UNORM_INT8:   // Fall through.
    case CLK_SIGNED_INT8:  // Fall through.
    case CLK_UNSIGNED_INT8:
      component_size = sizeof(cl_uchar);
      break;
    case CLK_SNORM_INT16:     // Fall through.
    case CLK_UNORM_INT16:     // Fall through.
    case CLK_SIGNED_INT16:    // Fall through.
    case CLK_UNSIGNED_INT16:  // Fall through.
    case CLK_HALF_FLOAT:
      component_size = sizeof(cl_ushort);
      break;
    case CLK_UNORM_SHORT_565:  // Fall through.
    case CLK_UNORM_SHORT_555:
      component_size = sizeof(cl_ushort);
      break;
    case CLK_SIGNED_INT32:    // Fall through.
    case CLK_UNSIGNED_INT32:  // Fall through.
    case CLK_FLOAT:
      component_size = sizeof(cl_uint);
      break;
    case CLK_UNORM_INT_101010:
      component_size = sizeof(cl_uint);
      break;
    default:
      IMG_UNREACHABLE("Unknown channel type.");
      break;
  }

  return component_size * component_count;
}

void libimg::HostSetImagePitches(const cl_image_format &image_format,
                                 const cl_image_desc &image_desc,
                                 const size_t region[3], size_t *host_row_pitch,
                                 size_t *host_slice_pitch) {
  IMG_ASSERT(region, "region must not be null!");
  IMG_ASSERT(region[0], "region element must not be 0.");
  IMG_ASSERT(host_row_pitch, "host_row_pitch must not be null!");
  IMG_ASSERT(host_slice_pitch, "host_slice_pitch must not be null!");

  size_t row_pitch = *host_row_pitch;
  if (0 == row_pitch) {
    const size_t pixel_size = HostGetPixelSize(image_format);
    row_pitch = pixel_size * region[0];
  }

  size_t slice_pitch = *host_slice_pitch;
  if (0 == slice_pitch) {
    switch (image_desc.image_type) {
      case CL_MEM_OBJECT_IMAGE1D:         // Fall through.
      case CL_MEM_OBJECT_IMAGE1D_BUFFER:  // Fall through.
      case CL_MEM_OBJECT_IMAGE2D: {
        break;
      }
      case CL_MEM_OBJECT_IMAGE1D_ARRAY: {
        slice_pitch = row_pitch;
        break;
      }
      case CL_MEM_OBJECT_IMAGE3D:  // Fall through
      case CL_MEM_OBJECT_IMAGE2D_ARRAY: {
        slice_pitch = row_pitch * region[1];
        break;
      }
      default: {
        IMG_UNREACHABLE("unknown or invalid image type.");
      }
    }
  }

  *host_row_pitch = row_pitch;
  *host_slice_pitch = slice_pitch;
}

size_t libimg::HostGetImageStorageSize(const cl_image_format &image_format,
                                       const cl_image_desc &image_desc) {
  const size_t pixel_size = libimg::HostGetPixelSize(image_format);

  size_t row_pitch = image_desc.image_row_pitch;
  if (0 == row_pitch) {
    row_pitch = pixel_size * image_desc.image_width;
  }

  size_t slice_pitch = image_desc.image_slice_pitch;
  if (0 == slice_pitch) {
    slice_pitch = row_pitch * image_desc.image_height;
  }

  size_t storageSize = 0;
  switch (image_desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:  // Fallthrough
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      storageSize = row_pitch;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      storageSize = row_pitch * image_desc.image_array_size;
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      storageSize = row_pitch * image_desc.image_height;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      storageSize = slice_pitch * image_desc.image_depth;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      storageSize = slice_pitch * image_desc.image_array_size;
      break;
    default:
      IMG_UNREACHABLE("Invalid OpenCL image type!");
  }

  return storageSize;
}

uint64_t libimg::HostGetImageAllocationSize(const cl_mem_flags flags,
                                            const cl_image_format &image_format,
                                            const cl_image_desc &image_desc) {
  const uint64_t headerSize = sizeof(libimg::HostImage);

  if (CL_MEM_OBJECT_IMAGE1D_BUFFER == image_desc.image_type ||
      CL_MEM_USE_HOST_PTR == (flags & CL_MEM_USE_HOST_PTR)) {
    // NOTE: The image is using external storage so we do not need to allocate
    // additional memory to storage the image data.
    return headerSize;
  }

  const uint64_t storageSize =
      libimg::HostGetImageStorageSize(image_format, image_desc);

  return HostImageAlignedImageSize(headerSize, storageSize);
}

cl_int libimg::HostGetSupportedImageFormats(const cl_mem_flags flags,
                                            const cl_mem_object_type image_type,
                                            const cl_uint num_entries,
                                            cl_image_format *image_formats,
                                            cl_uint *num_image_formats) {
  // These parameters are currently unused and are provided for future use so we
  // can match the CL API
  (void)flags;
  (void)image_type;
  (void)num_entries;

  // NOTE: Table of all OpenCL image formats, used as reference to determine
  // which image formats the devices support, the count member is used to keep
  // track of the number of devices an image format is supported on.
  static const cl_image_format formats[] = {
      {CL_R, CL_SNORM_INT8},
      {CL_R, CL_SNORM_INT16},
      {CL_R, CL_UNORM_INT8},
      {CL_R, CL_UNORM_INT16},
      {CL_R, CL_SIGNED_INT8},
      {CL_R, CL_SIGNED_INT16},
      {CL_R, CL_SIGNED_INT32},
      {CL_R, CL_UNSIGNED_INT8},
      {CL_R, CL_UNSIGNED_INT16},
      {CL_R, CL_UNSIGNED_INT32},
      {CL_R, CL_HALF_FLOAT},
      {CL_R, CL_FLOAT},
      {CL_A, CL_SNORM_INT8},
      {CL_A, CL_SNORM_INT16},
      {CL_A, CL_UNORM_INT8},
      {CL_A, CL_UNORM_INT16},
      {CL_A, CL_SIGNED_INT8},
      {CL_A, CL_SIGNED_INT16},
      {CL_A, CL_SIGNED_INT32},
      {CL_A, CL_UNSIGNED_INT8},
      {CL_A, CL_UNSIGNED_INT16},
      {CL_A, CL_UNSIGNED_INT32},
      {CL_A, CL_HALF_FLOAT},
      {CL_A, CL_FLOAT},
      {CL_RG, CL_SNORM_INT8},
      {CL_RG, CL_SNORM_INT16},
      {CL_RG, CL_UNORM_INT8},
      {CL_RG, CL_UNORM_INT16},
      {CL_RG, CL_SIGNED_INT8},
      {CL_RG, CL_SIGNED_INT16},
      {CL_RG, CL_SIGNED_INT32},
      {CL_RG, CL_UNSIGNED_INT8},
      {CL_RG, CL_UNSIGNED_INT16},
      {CL_RG, CL_UNSIGNED_INT32},
      {CL_RG, CL_HALF_FLOAT},
      {CL_RG, CL_FLOAT},
      {CL_RA, CL_SNORM_INT8},
      {CL_RA, CL_SNORM_INT16},
      {CL_RA, CL_UNORM_INT8},
      {CL_RA, CL_UNORM_INT16},
      {CL_RA, CL_SIGNED_INT8},
      {CL_RA, CL_SIGNED_INT16},
      {CL_RA, CL_SIGNED_INT32},
      {CL_RA, CL_UNSIGNED_INT8},
      {CL_RA, CL_UNSIGNED_INT16},
      {CL_RA, CL_UNSIGNED_INT32},
      {CL_RA, CL_HALF_FLOAT},
      {CL_RA, CL_FLOAT},
      {CL_RGB, CL_UNORM_SHORT_565},
      {CL_RGB, CL_UNORM_SHORT_555},
      {CL_RGB, CL_UNORM_INT_101010},
      {CL_RGBA, CL_SNORM_INT8},
      {CL_RGBA, CL_SNORM_INT16},
      {CL_RGBA, CL_UNORM_INT8},
      {CL_RGBA, CL_UNORM_INT16},
      {CL_RGBA, CL_SIGNED_INT8},
      {CL_RGBA, CL_SIGNED_INT16},
      {CL_RGBA, CL_SIGNED_INT32},
      {CL_RGBA, CL_UNSIGNED_INT8},
      {CL_RGBA, CL_UNSIGNED_INT16},
      {CL_RGBA, CL_UNSIGNED_INT32},
      {CL_RGBA, CL_HALF_FLOAT},
      {CL_RGBA, CL_FLOAT},
      {CL_BGRA, CL_SNORM_INT8},
      {CL_BGRA, CL_UNORM_INT8},
      {CL_BGRA, CL_SIGNED_INT8},
      {CL_BGRA, CL_UNSIGNED_INT8},
      {CL_ARGB, CL_SNORM_INT8},
      {CL_ARGB, CL_UNORM_INT8},
      {CL_ARGB, CL_SIGNED_INT8},
      {CL_ARGB, CL_UNSIGNED_INT8},
      {CL_INTENSITY, CL_SNORM_INT8},
      {CL_INTENSITY, CL_SNORM_INT16},
      {CL_INTENSITY, CL_UNORM_INT8},
      {CL_INTENSITY, CL_UNORM_INT16},
      {CL_INTENSITY, CL_HALF_FLOAT},
      {CL_INTENSITY, CL_FLOAT},
      {CL_LUMINANCE, CL_SNORM_INT8},
      {CL_LUMINANCE, CL_SNORM_INT16},
      {CL_LUMINANCE, CL_UNORM_INT8},
      {CL_LUMINANCE, CL_UNORM_INT16},
      {CL_LUMINANCE, CL_HALF_FLOAT},
      {CL_LUMINANCE, CL_FLOAT},
      {CL_Rx, CL_SNORM_INT8},
      {CL_Rx, CL_SNORM_INT16},
      {CL_Rx, CL_UNORM_INT8},
      {CL_Rx, CL_UNORM_INT16},
      {CL_Rx, CL_SIGNED_INT8},
      {CL_Rx, CL_SIGNED_INT16},
      {CL_Rx, CL_SIGNED_INT32},
      {CL_Rx, CL_UNSIGNED_INT8},
      {CL_Rx, CL_UNSIGNED_INT16},
      {CL_Rx, CL_UNSIGNED_INT32},
      {CL_Rx, CL_HALF_FLOAT},
      {CL_Rx, CL_FLOAT},
      {CL_RGx, CL_SNORM_INT8},
      {CL_RGx, CL_SNORM_INT16},
      {CL_RGx, CL_UNORM_INT8},
      {CL_RGx, CL_UNORM_INT16},
      {CL_RGx, CL_SIGNED_INT8},
      {CL_RGx, CL_SIGNED_INT16},
      {CL_RGx, CL_SIGNED_INT32},
      {CL_RGx, CL_UNSIGNED_INT8},
      {CL_RGx, CL_UNSIGNED_INT16},
      {CL_RGx, CL_UNSIGNED_INT32},
      {CL_RGx, CL_HALF_FLOAT},
      {CL_RGx, CL_FLOAT},
      {CL_RGBx, CL_UNORM_SHORT_565},
      {CL_RGBx, CL_UNORM_SHORT_555},
      {CL_RGBx, CL_UNORM_INT_101010},
  };

  const size_t num_formats = sizeof(formats) / sizeof(formats[0]);

  // TODO: Currently we ignore the image type, this will need to be fixed at
  // some point in the future as not all devices may support the same image
  // formats for all image types.
  if (image_formats) {
    std::memcpy(image_formats, formats, sizeof(formats));
  }

  if (num_image_formats) {
    *num_image_formats = num_formats;
  }

  return CL_SUCCESS;
}

void libimg::HostInitializeImage(const cl_image_format &image_format,
                                 const cl_image_desc &image_desc,
                                 libimg::HostImage *image) {
  image->type = image_desc.image_type;

  image->image.meta_data.channel_order = image_format.image_channel_order;
  image->image.meta_data.channel_type = image_format.image_channel_data_type;
  image->image.meta_data.pixel_size = libimg::HostGetPixelSize(image_format);

  image->image.meta_data.width = image_desc.image_width;
  image->image.meta_data.height = 1;
  image->image.meta_data.depth = 1;
  image->image.meta_data.array_size = 1;

  switch (image_desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      image->image.meta_data.height = image_desc.image_height;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      image->image.meta_data.depth = image_desc.image_depth;
      image->image.meta_data.height = image_desc.image_height;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      image->image.meta_data.array_size = image_desc.image_array_size;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      image->image.meta_data.height = image_desc.image_height;
      image->image.meta_data.array_size = image_desc.image_array_size;
      break;
    default:
      IMG_UNREACHABLE("Invalid cl_mem_object_type!");
  }

  if (image_desc.image_row_pitch) {
    image->image.meta_data.row_pitch = image_desc.image_row_pitch;
  } else {
    image->image.meta_data.row_pitch =
        image->image.meta_data.width * image->image.meta_data.pixel_size;
  }

  if (image_desc.image_slice_pitch) {
    image->image.meta_data.slice_pitch = image_desc.image_slice_pitch;
  } else {
    image->image.meta_data.slice_pitch =
        image->image.meta_data.row_pitch * image->image.meta_data.height;
  }
}

void libimg::HostAttachImageStorage(libimg::HostImage *image, void *ptr) {
  image->storage = libimg::kRawDataStorageExternal;
  image->image.raw_data = static_cast<UChar *>(ptr);
}

libimg::HostImage *libimg::HostCreateImage(const cl_mem_flags flags,
                                           const cl_image_format &image_format,
                                           const cl_image_desc &image_desc,
                                           void *ptr, const size_t ptr_size,
                                           void *external_data) {
  IMG_ASSERT(ptr, "ptr must not be null!");
  IMG_ASSERT(ptr_size >= libimg::HostGetImageAllocationSize(flags, image_format,
                                                            image_desc),
             "ptr_size must be at least libimg::HostGetImageAllocationSize!");
  (void)ptr_size;

  libimg::HostImage *image = static_cast<libimg::HostImage *>(ptr);

  image->type = image_desc.image_type;

  libimg::HostInitializeImage(image_format, image_desc, image);

  if (CL_MEM_OBJECT_IMAGE1D_BUFFER == image_desc.image_type ||
      CL_MEM_USE_HOST_PTR == (flags & CL_MEM_USE_HOST_PTR)) {
    HostAttachImageStorage(image, external_data);
  } else {
    image->storage = libimg::kRawDataStorageInternal;
    image->image.raw_data = HostImageRawDataStorageInternalAddress(image);
  }

  if (CL_MEM_COPY_HOST_PTR == (flags & CL_MEM_COPY_HOST_PTR)) {
    std::memcpy(image->image.raw_data, external_data,
                libimg::HostGetImageStorageSize(image_format, image_desc));
  }

  return image;
}

void *libimg::HostGetImageStoragePtr(HostImage *image) {
  IMG_ASSERT(image, "image must not be null!");
  return image->image.raw_data;
}

void *libimg::HostGetImageKernelImagePtr(HostImage *image) {
  IMG_ASSERT(image, "image must not be null!");
  return &image->image;
}

void libimg::HostReadImage(const HostImage *image, const size_t origin[3],
                           const size_t region[3], const size_t dst_row_pitch,
                           const size_t dst_slice_pitch, uint8_t *dst) {
  const ImageMetaData &desc = image->image.meta_data;

  const uint8_t *src = image->image.raw_data + (origin[0] * desc.pixel_size);

  const size_t row_size = region[0] * desc.pixel_size;

  for (size_t z = 0; z < region[2]; ++z) {
    const uint8_t *src_slice = src + ((z + origin[2]) * desc.slice_pitch);
    uint8_t *dst_slice = dst + (z * dst_slice_pitch);
    for (size_t y = 0; y < region[1]; ++y) {
      const uint8_t *src_row = src_slice + ((y + origin[1]) * desc.row_pitch);
      uint8_t *dst_row = dst_slice + (y * dst_row_pitch);
      std::memmove(dst_row, src_row, row_size);
    }
  }
}

void libimg::HostWriteImage(HostImage *image, const size_t origin[3],
                            const size_t region[3], const size_t src_row_pitch,
                            const size_t src_slice_pitch, const uint8_t *src) {
  const ImageMetaData &desc = image->image.meta_data;

  uint8_t *dst = image->image.raw_data + (origin[0] * desc.pixel_size);

  const size_t row_size = region[0] * desc.pixel_size;

  for (size_t z = 0; z < region[2]; ++z) {
    const uint8_t *src_slice = src + (z * src_slice_pitch);
    uint8_t *dst_slice = dst + ((z + origin[2]) * desc.slice_pitch);
    for (size_t y = 0; y < region[1]; ++y) {
      const uint8_t *src_row = src_slice + (y * src_row_pitch);
      uint8_t *dst_row = dst_slice + ((y + origin[1]) * desc.row_pitch);
      std::memmove(dst_row, src_row, row_size);
    }
  }
}

static libimg::UInt4 ShuffleOrder(const libimg::UInt order,
                                  const libimg::UInt4 &in) {
  switch (order)
  case CL_A: {
    return libimg::make<libimg::UInt4>(in[3], 0u, 0u, 0u);
    case CL_RA:
      return libimg::make<libimg::UInt4>(in[0], in[3], 0u, 0u);
    case CL_ARGB:
      return libimg::make<libimg::UInt4>(in[3], in[0], in[1], in[2]);
    case CL_BGRA:
      return libimg::make<libimg::UInt4>(in[2], in[1], in[0], in[3]);
    default:
      break;
  }
    return in;
}

void libimg::HostFillImage(HostImage *image, const void *fill_color,
                           const size_t origin[3], const size_t region[3]) {
  const ImageMetaData &desc = image->image.meta_data;

  UInt4 shuffled_color = ShuffleOrder(
      desc.channel_order, *reinterpret_cast<const UInt4 *>(fill_color));
  uint8_t final_color[16] = {};

  switch (desc.channel_type) {
    case CLK_SIGNED_INT8: {
      Char4 color =
          libimg::convert_char4_sat(cargo::bit_cast<Int4>(shuffled_color));
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_SIGNED_INT16: {
      Short4 color =
          libimg::convert_short4_sat(cargo::bit_cast<Int4>(shuffled_color));
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_SIGNED_INT32: {
      std::memcpy(final_color, &shuffled_color, desc.pixel_size);
    } break;
    case CLK_UNSIGNED_INT8: {
      UChar4 color = libimg::convert_uchar4_sat(shuffled_color);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNSIGNED_INT16: {
      UShort4 color = libimg::convert_ushort4_sat(shuffled_color);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNSIGNED_INT32: {
      std::memcpy(final_color, &shuffled_color, desc.pixel_size);
    } break;
    case CLK_SNORM_INT8: {
      Char4 color = libimg::convert_char4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 127.0f);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_SNORM_INT16: {
      Short4 color = libimg::convert_short4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 32767.0f);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNORM_INT8: {
      UChar4 color = libimg::convert_uchar4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 255.0f);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNORM_INT16: {
      UShort4 color = libimg::convert_ushort4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 65535.0f);
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNORM_SHORT_565: {
      Char4 c = libimg::convert_char4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 31.0f);
      Char4 c1 = libimg::convert_char4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 63.0f);

      UShort color = 0 | c[2] | (c1[1] << 5) | (c[0] << 11);

      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNORM_SHORT_555: {
      Char4 c = libimg::convert_char4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 31.0f);

      UShort color = 0 | c[2] | (c[1] << 5) | (c[0] << 10);

      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_UNORM_INT_101010: {
      Short4 c = libimg::convert_short4_sat(
          cargo::bit_cast<Float4>(shuffled_color) * 1023.0f);

      UInt color = 0 | c[2] | (c[1] << 10) | (c[0] << 20);

      std::memcpy(final_color, &color, desc.pixel_size);

    } break;
    case CLK_HALF_FLOAT: {
      Half4 color = libimg::convert_float4_to_half4_rte(
          cargo::bit_cast<Float4>(shuffled_color));
      std::memcpy(final_color, &color, desc.pixel_size);
    } break;
    case CLK_FLOAT: {
      std::memcpy(final_color, &shuffled_color, desc.pixel_size);
    } break;
    default: {
      IMG_UNREACHABLE("unhandled channel type");
    }
  }

  uint8_t *dst = image->image.raw_data + (origin[0] * desc.pixel_size) +
                 (origin[1] * desc.row_pitch) + (origin[2] * desc.slice_pitch);

  for (size_t z = 0; z < region[2]; ++z) {
    uint8_t *dst_slice = dst + (z * desc.slice_pitch);
    for (size_t y = 0; y < region[1]; ++y) {
      uint8_t *dst_row = dst_slice + (y * desc.row_pitch);
      for (size_t x = 0; x < region[0]; ++x) {
        uint8_t *dst_pixel = dst_row + (x * desc.pixel_size);
        std::memcpy(dst_pixel, final_color, desc.pixel_size);
      }
    }
  }
}

void libimg::HostCopyImage(const HostImage *src_image, HostImage *dst_image,
                           const size_t src_origin[3],
                           const size_t dst_origin[3], const size_t region[3]) {
  const ImageMetaData &src_desc = src_image->image.meta_data;
  const ImageMetaData &dst_desc = dst_image->image.meta_data;

  const uint8_t *const src =
      src_image->image.raw_data + (src_origin[0] * src_desc.pixel_size);
  uint8_t *const dst =
      dst_image->image.raw_data + (dst_origin[0] * dst_desc.pixel_size);

  const size_t z_max = region[2];
  const size_t y_max = region[1];
  const size_t x_size = region[0] * src_desc.pixel_size;

  for (size_t z = 0; z < z_max; z++) {
    const uint8_t *const src_slice =
        src + ((z + src_origin[2]) * src_desc.slice_pitch);
    uint8_t *const dst_slice =
        dst + ((z + dst_origin[2]) * dst_desc.slice_pitch);
    for (size_t y = 0; y < y_max; y++) {
      const uint8_t *const src_row =
          src_slice + ((y + src_origin[1]) * src_desc.row_pitch);
      uint8_t *dst_row = dst_slice + ((y + dst_origin[1]) * dst_desc.row_pitch);
      std::memmove(dst_row, src_row, x_size);
    }
  }
}

void libimg::HostCopyImageToBuffer(const HostImage *src_image, void *dst_buffer,
                                   const size_t src_origin[3],
                                   const size_t region[3],
                                   const size_t dst_offset) {
  const ImageMetaData &desc = src_image->image.meta_data;

  const uint8_t *const src =
      src_image->image.raw_data + (desc.pixel_size * src_origin[0]) +
      (src_origin[1] * desc.row_pitch) + (src_origin[2] * desc.slice_pitch);
  uint8_t *const dst = static_cast<uint8_t *>(dst_buffer) + dst_offset;

  const size_t z_max = region[2];
  const size_t y_max = region[1];
  const size_t x_size = region[0] * desc.pixel_size;

  uint8_t *dst_row = dst;
  for (size_t z = 0; z < z_max; z++) {
    const uint8_t *src_row = src + (desc.slice_pitch * z);
    for (size_t y = 0; y < y_max; y++) {
      std::memmove(dst_row, src_row, x_size);
      dst_row += x_size;
      src_row += desc.row_pitch;
    }
  }
}

void libimg::HostCopyBufferToImage(const void *src_buffer, HostImage *dst_image,
                                   const size_t src_offset,
                                   const size_t dst_origin[3],
                                   const size_t region[3]) {
  const ImageMetaData &desc = dst_image->image.meta_data;

  const uint8_t *src = static_cast<const uint8_t *>(src_buffer) + src_offset;
  uint8_t *dst = dst_image->image.raw_data;

  const size_t z_max = dst_origin[2] + region[2];
  const size_t y_max = dst_origin[1] + region[1];
  const size_t x_size = region[0] * desc.pixel_size;

  const size_t region_slice_pitch = region[1] * region[0] * desc.pixel_size;
  const size_t region_row_pitch = region[0] * desc.pixel_size;

  for (size_t z = dst_origin[2]; z < z_max; z++) {
    const uint8_t *src_row = src;
    uint8_t *dst_row = dst;

    for (size_t y = dst_origin[1]; y < y_max; y++) {
      std::memmove(dst_row, src_row, x_size);

      src_row += region_row_pitch;
      dst_row += desc.row_pitch;
    }

    src += region_slice_pitch;
    dst += desc.slice_pitch;
  }
}
