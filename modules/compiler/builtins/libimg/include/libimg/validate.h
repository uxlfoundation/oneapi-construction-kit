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
/// @brief Contains the image libraries validation API.

#ifndef CODEPLAY_IMG_VALIDATE_H
#define CODEPLAY_IMG_VALIDATE_H

#include <CL/cl.h>

#ifndef NDEBUG
#include <cstdio>
#include <cstdlib>

/// @brief Host side image validation API.
///
/// @addtogroup libimg
/// @{

/// @brief Ensure the condition is true, otherwise abort. Disabled by NDEBUG.
///
/// @param CONDITION Expression to evaluate and ensure is true.
/// @param MESSAGE A human readable message describing the failure.
#define IMG_ASSERT(CONDITION, MESSAGE)                                 \
  if (!(CONDITION)) {                                                  \
    fprintf(stderr, "%s: %d: libimg assert: %s\n", __FILE__, __LINE__, \
            MESSAGE);                                                  \
    abort();                                                           \
  }

/// @brief Abort due to a situation that should never happen. Disabled by NDEBUG
///
/// @param MESSAGE A human readable message describing the failure.
#define IMG_UNREACHABLE(MESSAGE)                                            \
  {                                                                         \
    fprintf(stderr, "%s: %d: libimg unreachable: %s\n", __FILE__, __LINE__, \
            MESSAGE);                                                       \
    abort();                                                                \
  }
#else
#define IMG_ASSERT(CONDITION, MESSAGE)
#define IMG_UNREACHABLE(MESSAGE)
#endif

/// @brief Check that the condition is false, otherwise perform the action.
///
/// @param CONDITION Expression to evaluate and check it is true.
/// @param ACTION Commands to be executed in event of a true condition.
#define IMG_CHECK(CONDITION, ACTION) \
  if (CONDITION) {                   \
    ACTION;                          \
  }

/// @}

namespace libimg {
/// @addtogroup libimg
/// @{

/// @brief Check that the image format is valid.
///
/// @param image_format Image format, describing pixel stroage.
///
/// @return CL_SUCCESS on success, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR otherwise.
cl_int ValidateImageFormat(const cl_image_format& image_format);

/// @brief Check that the image descriptor is within the given device limits.
///
/// @param desc The image descriptor.
/// @param image2d_max_width Device max supported 2d image width.
/// @param image2d_max_height Device max supported 2d image height.
/// @param image3d_max_width Device max supported 3d image width.
/// @param image3d_max_height Device max supported 3d image height.
/// @param image3d_max_depth Device max supported 3d image depth.
/// @param image_max_array_size Device max supported array size.
/// @param image_max_buffer_size Device max supported buffer size.
///
/// @return Returns CL_INVALID_IMAGE_SIZE on error, CL_SUCCESS otherwise.
cl_int ValidateImageSize(
    const cl_image_desc& desc, const size_t image2d_max_width,
    const size_t image2d_max_height, const size_t image3d_max_width,
    const size_t image3d_max_height, const size_t image3d_max_depth,
    const size_t image_max_array_size, const size_t image_max_buffer_size);

/// @brief Check that the origin and region are valid for the given image.
///
/// @param desc The descriptor for the image from.
/// @param origin The origin of the image to copy from.
/// @param region The region of the image to copy from.
///
/// @return Returns CL_INVALID_VALUE on error, CL_SUCCESS otherwise.
cl_int ValidateOriginAndRegion(const cl_image_desc& desc,
                               const size_t origin[3], const size_t region[3]);

/// @brief Check that the left and right image formats are compatible.
///
/// @param format_left Left hand image format.
/// @param format_right Right hand image format.
///
/// @return Returns CL_IMAGE_FORMAT_MISMATCH on error, CL_SUCCESS otherwise.
cl_int ValidateImageFormatMismatch(const cl_image_format& format_left,
                                   const cl_image_format& format_right);

/// @brief Validates the row pitch and slice pitch of the user provided host
/// memory to read from/write to.  Helper function called by clEnqueueReadImage
/// and clEnqueueWriteImage.  See specification of row_pitch and slice_pitch for
/// clEnqueueReadImage or see specification for input_row_pitch and
/// input_slice_pitch of clEnqueueWriteImage.
///
/// @param image_format Description of the image format.
/// @param image_desc Description of the image data.
/// @param region Validated region providing the width and, if required, the
/// height to validate the values for the pitches.
/// @param host_row_pitch Row pitch of the user provided host memory to read
/// from/write to.
/// @param host_slice_pitch Slice pitch of the user provided host memory to read
/// from/write to.
///
/// @return CL_SUCCESS if the pitches are valid. CL_INVALID_VALUE if the image
/// is of type CL_MEM_OBJECT_IMAGE1D or CL_MEM_OBJECT_IMAGE2D and the pitches
/// are not 0.  CL_INVALID_VALUE of the pitches are larger than 0 but lesser
/// than the minimum required values.
cl_int ValidateRowAndSlicePitchForReadWriteImage(
    const cl_image_format& image_format, const cl_image_desc& image_desc,
    const size_t region[3], const size_t host_row_pitch,
    const size_t host_slice_pitch);

/// @brief Validates a src and dst origin with region and image_description to
/// ensure that they are not overlapping. This is only used if the src and dst
/// images are the same. Called by clEnqueueCopyImage. This also previously
/// assumes other validations have been performed and that the src_origin and
/// dst_origin are legitimate
///
/// @param desc Description of the image format. As the image is the same for
/// src and dst we only need the one
/// @param src_origin The origin for the src image.
/// @param dst_origin The origin for the dst image.
/// @param region Row pitch of the user provided host memory to read
/// from/write to.
///
/// @return CL_SUCCESS if the pitches are valid. CL_MEM_COPY_OVERLAP if the
/// image overlaps for any of the image types on any dimension.
cl_int ValidateNoOverlap(const cl_image_desc& desc, const size_t src_origin[3],
                         const size_t dst_origin[3], const size_t region[3]);

/// @}
}  // namespace libimg

#endif  // CODEPLAY_IMG_VALIDATE_H
