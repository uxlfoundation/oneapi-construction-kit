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

#include <cargo/attributes.h>
#include <libimg/host.h>
#include <libimg/validate.h>

cl_int libimg::ValidateImageFormat(const cl_image_format &image_format) {
  switch (image_format.image_channel_order) {
    case CLK_R:          // Fall through.
    case CLK_A:          // Fall through.
    case CLK_RG:         // Fall through.
    case CLK_RA:         // Fall through.
    case CLK_RGB:        // Fall through.
    case CLK_RGBA:       // Fall through.
    case CLK_BGRA:       // Fall through.
    case CLK_ARGB:       // Fall through.
    case CLK_INTENSITY:  // Fall through.
    case CLK_LUMINANCE:  // Fall through.
    case CLK_Rx:         // Fall through.
    case CLK_RGx:        // Fall through.
    case CLK_RGBx:
      // Channel order is valid.
      break;
    case CLK_DEPTH:          // Fall through, not handled yet.
    case CLK_DEPTH_STENCIL:  // Fall through, not handled yet.
    default:
      return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
  }

  switch (image_format.image_channel_data_type) {
    case CLK_SNORM_INT8:        // Fall through.
    case CLK_SNORM_INT16:       // Fall through.
    case CLK_UNORM_INT8:        // Fall through.
    case CLK_UNORM_INT16:       // Fall through.
    case CLK_UNORM_SHORT_565:   // Fall through.
    case CLK_UNORM_SHORT_555:   // Fall through.
    case CLK_UNORM_INT_101010:  // Fall through.
    case CLK_SIGNED_INT8:       // Fall through.
    case CLK_SIGNED_INT16:      // Fall through.
    case CLK_SIGNED_INT32:      // Fall through.
    case CLK_UNSIGNED_INT8:     // Fall through.
    case CLK_UNSIGNED_INT16:    // Fall through.
    case CLK_UNSIGNED_INT32:    // Fall through.
    case CLK_HALF_FLOAT:        // Fall through.
    case CLK_FLOAT:
      // Channel data type is valid.
      break;
    case CLK_UNORM_INT24:  // Fall through, not handled yet.
    default:
      return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
  }

  if (CLK_INTENSITY == image_format.image_channel_order ||
      CLK_LUMINANCE == image_format.image_channel_order) {
    switch (image_format.image_channel_data_type) {
      case CLK_UNORM_INT8:
      case CLK_UNORM_INT16:
      case CLK_SNORM_INT8:
      case CLK_SNORM_INT16:
      case CLK_HALF_FLOAT:
      case CLK_FLOAT:
        break;
      default:
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
  }

  if (CLK_RGB == image_format.image_channel_order ||
      CLK_RGBx == image_format.image_channel_order) {
    switch (image_format.image_channel_data_type) {
      case CLK_UNORM_SHORT_565:
      case CLK_UNORM_SHORT_555:
      case CLK_UNORM_INT_101010:
        break;
      default:
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
  }

  if (CLK_ARGB == image_format.image_channel_order ||
      CLK_BGRA == image_format.image_channel_order) {
    switch (image_format.image_channel_data_type) {
      case CLK_UNORM_INT8:
      case CLK_SNORM_INT8:
      case CLK_SIGNED_INT8:
      case CLK_UNSIGNED_INT8:
        break;
      default:
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
  }

  if (CLK_UNORM_SHORT_565 == image_format.image_channel_data_type &&
      !(CLK_RGB == image_format.image_channel_order ||
        CLK_RGBx == image_format.image_channel_order)) {
    return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
  }

  if (CLK_UNORM_SHORT_555 == image_format.image_channel_data_type &&
      !(CLK_RGB == image_format.image_channel_order ||
        CLK_RGBx == image_format.image_channel_order)) {
    return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
  }

  if ((CLK_UNORM_INT_101010 == image_format.image_channel_data_type) &&
      !(CLK_RGB == image_format.image_channel_order ||
        CLK_RGBx == image_format.image_channel_order)) {
    return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
  }

  return CL_SUCCESS;
}

cl_int libimg::ValidateImageSize(
    const cl_image_desc &desc, const size_t image2d_max_width,
    const size_t image2d_max_height, const size_t image3d_max_width,
    const size_t image3d_max_height, const size_t image3d_max_depth,
    const size_t image_max_array_size, const size_t image_max_buffer_size) {
  switch (desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      IMG_CHECK(image_max_buffer_size < desc.image_width,
                return CL_INVALID_IMAGE_SIZE);
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      IMG_CHECK(image_max_array_size < desc.image_array_size,
                return CL_INVALID_IMAGE_SIZE);
      [[fallthrough]];
    case CL_MEM_OBJECT_IMAGE1D:
      IMG_CHECK(image2d_max_width < desc.image_width,
                return CL_INVALID_IMAGE_SIZE);
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      IMG_CHECK(image_max_array_size < desc.image_array_size,
                return CL_INVALID_IMAGE_SIZE);
      [[fallthrough]];
    case CL_MEM_OBJECT_IMAGE2D:
      IMG_CHECK(image2d_max_width < desc.image_width ||
                    image2d_max_height < desc.image_height,
                return CL_INVALID_IMAGE_SIZE);
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      IMG_CHECK(image3d_max_width < desc.image_width ||
                    image3d_max_height < desc.image_height ||
                    image3d_max_depth < desc.image_depth,
                return CL_INVALID_IMAGE_SIZE);
      break;
    default:
      return CL_INVALID_IMAGE_DESCRIPTOR;
  }
  return CL_SUCCESS;
}

cl_int libimg::ValidateOriginAndRegion(const cl_image_desc &desc,
                                       const size_t origin[3],
                                       const size_t region[3]) {
  switch (desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      IMG_CHECK(
          desc.image_width < origin[0] || 0 != origin[1] || 0 != origin[2],
          return CL_INVALID_VALUE);
      IMG_CHECK(
          desc.image_width < region[0] || 1 != region[1] || 1 != region[2],
          return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < origin[0] + region[0],
                return CL_INVALID_VALUE);
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      IMG_CHECK(desc.image_width < origin[0] ||
                    desc.image_array_size < origin[1] || 0 != origin[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(
          desc.image_width < region[0] || desc.image_array_size < region[1],
          return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < origin[0] + region[0] ||
                    desc.image_array_size < origin[1] + region[1],
                return CL_INVALID_VALUE);
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      IMG_CHECK(desc.image_width < origin[0] || desc.image_height < origin[1] ||
                    0 != origin[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < region[0] || desc.image_height < region[1] ||
                    1 != region[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < origin[0] + region[0] ||
                    desc.image_height < origin[1] + region[1],
                return CL_INVALID_VALUE);
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      IMG_CHECK(desc.image_width < origin[0] || desc.image_height < origin[1] ||
                    desc.image_array_size < origin[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < region[0] || desc.image_height < region[1] ||
                    desc.image_array_size < region[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < origin[0] + region[0] ||
                    desc.image_height < origin[1] + region[1] ||
                    desc.image_array_size < origin[2] + region[2],
                return CL_INVALID_VALUE);
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      IMG_CHECK(desc.image_width < origin[0] || desc.image_height < origin[1] ||
                    desc.image_depth < origin[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < region[0] || desc.image_height < region[1] ||
                    desc.image_depth < region[2],
                return CL_INVALID_VALUE);
      IMG_CHECK(desc.image_width < origin[0] + region[0] ||
                    desc.image_height < origin[1] + region[1] ||
                    desc.image_depth < origin[2] + region[2],
                return CL_INVALID_VALUE);
      break;
    default:
      return CL_INVALID_MEM_OBJECT;
  }
  return CL_SUCCESS;
}

cl_int libimg::ValidateImageFormatMismatch(
    const cl_image_format &format_left, const cl_image_format &format_right) {
  if ((format_left.image_channel_data_type !=
       format_right.image_channel_data_type) ||
      (format_left.image_channel_order != format_right.image_channel_order)) {
    return CL_IMAGE_FORMAT_MISMATCH;
  }
  return CL_SUCCESS;
}

cl_int libimg::ValidateRowAndSlicePitchForReadWriteImage(
    const cl_image_format &image_format, const cl_image_desc &image_desc,
    const size_t region[3], const size_t host_row_pitch,
    const size_t host_slice_pitch) {
  IMG_ASSERT(region, "region must not be null!");

  size_t min_row_pitch = 0;
  size_t min_slice_pitch = 0;
  libimg::HostSetImagePitches(image_format, image_desc, region, &min_row_pitch,
                              &min_slice_pitch);

  if (0 != host_row_pitch && host_row_pitch < min_row_pitch) {
    return CL_INVALID_VALUE;
  }

  const cl_mem_object_type image_type = image_desc.image_type;

  if (((CL_MEM_OBJECT_IMAGE1D == image_type) ||
       (CL_MEM_OBJECT_IMAGE2D == image_type)) &&
      0 != host_slice_pitch) {
    return CL_INVALID_VALUE;
  }

  if (0 != host_slice_pitch && host_slice_pitch < min_slice_pitch) {
    return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

cl_int libimg::ValidateNoOverlap(const cl_image_desc &desc,
                                 const size_t src_origin[3],
                                 const size_t dst_origin[3],
                                 const size_t region[3]) {
  int dimension = 0;
  switch (desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      dimension = 1;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    case CL_MEM_OBJECT_IMAGE2D:
      dimension = 2;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE3D:
      dimension = 3;
      break;
    default:
      return CL_INVALID_MEM_OBJECT;
  }

  for (int i = 0; i < dimension; ++i) {
    IMG_CHECK(dst_origin[i] >= src_origin[i] &&
                  dst_origin[i] < src_origin[i] + region[i],
              return CL_MEM_COPY_OVERLAP);
    IMG_CHECK(dst_origin[i] + region[i] >= src_origin[i] &&
                  dst_origin[i] + region[i] < src_origin[i] + region[i],
              return CL_MEM_COPY_OVERLAP);
  }

  return CL_SUCCESS;
}