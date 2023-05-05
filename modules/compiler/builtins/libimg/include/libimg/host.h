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
/// @brief Contains the image libraries host API.

#ifndef CODEPLAY_IMG_HOST_H
#define CODEPLAY_IMG_HOST_H

#include <libimg/shared.h>

#include <CL/cl.h>

namespace libimg {
/// @brief The types and functions encapsulating the host API's.
///
/// @addtogroup libimg
/// @{

/// @brief Enumeration of image data storage types, this is OpenCL specific.
///
/// Enable representation of cl_mem_object_image1d_buffer and OpenCL 2.x 2D
/// images using other images as their raw data providers.
enum RawDataStorage {
  /// @brief Raw data stored internally directly next to meta data.
  kRawDataStorageInternal,
  /// @brief Raw data stored in externally provided memory.
  kRawDataStorageExternal
};

/// @brief Representation of an image, used by the host API.
///
/// If host image meta data storage type is kInternal, then image.raw_data
/// points
/// to the memory area directly following the memory of HostImage.
///
/// If host image meta data storage type is kExternal, then image.raw_data
/// points
/// to user provided memory.
struct HostImage {
  /// @brief The OpenCL image type.
  cl_mem_object_type type;
  /// @brief The location of the image storage ownership.
  RawDataStorage storage;
  /// @brief The kernel image.
  Image image;
};

/// @brief Sampler is a 32-bit bitfield, that ors together filtering mode,
/// addressing mode and normalized coordinates. In image library represented as
/// uint32_t.
typedef uint32_t HostSampler;

/// @brief Host side creation functions.

/// @brief Create a HostImage in previously allocated memory.
///
/// @param flags Memory access and usage flags.
/// @param image_format Description of pixel layout and storage.
/// @param image_desc Description of image dimensions.
/// @param ptr Pointer to previously allocated memory for image storage, must
/// not be null.
/// @param ptr_size Size in bytes of ptr, must be at least
/// libimg::HostGetImageAllocationSize() bytes.
/// @param external_data If CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR flags
/// are set this should be host_ptr, if image type is
/// CL_MEM_OBJECT_IMAGE1D_BUFFER this should be a pointer to the internal buffer
/// data, otherwise it should be null.
///
/// @return Pointer to the created image, points to the same address as ptr.
HostImage* HostCreateImage(const cl_mem_flags flags,
                           const cl_image_format& image_format,
                           const cl_image_desc& image_desc, void* ptr,
                           const size_t ptr_size, void* external_data);

/// @brief Initialize a HostImage in preparation for attaching external storage.
///
/// @param image_format Description of pixel layout and storage.
/// @param image_desc Description of image dimensions.
/// @param image Image to initialize.
void HostInitializeImage(const cl_image_format& image_format,
                         const cl_image_desc& image_desc, HostImage* image);

/// @brief Attach external image storage.
///
/// @param image Image to attach external storage to.
/// @param ptr Pointer to external storage.
void HostAttachImageStorage(HostImage* image, void* ptr);

/// @brief Create a sampler from OpenCL sampler values.
///
/// This function translates the original CL values to their CLK equivalents,
/// then or them together input arguments to form a sampler value.
///
/// @param normalized_coordinates Determines if the image coordinates specified
/// are normalized
/// @param addressing_mode Specifies how out of range image coordinates are
/// handled.
/// @param filter_mode Specifies the type of filter.
///
/// @return HostSampler.
HostSampler HostCreateSampler(const cl_bool normalized_coordinates,
                              const cl_addressing_mode addressing_mode,
                              const cl_filter_mode filter_mode);
/// @}

/// @brief Host side image manipulation functions.
///
/// @weakgroup Manipulation
/// @{

/// @brief Read from an image into the provided memory.
///
/// @param image Image to read from.
/// @param origin Position in pixels to begin reading from.
/// @param region Range in pixels to read.
/// @param dst_row_pitch Size in bytes of dst row, must not be zero use
/// libimg::HostSetImagePitches() for default value.
/// @param dst_slice_pitch Size in bytes of dst slice, must not be zero use
/// libimg::HostSetImagePitches() for default value.
/// @param dst Memory to write read pixels into.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueReadImage.html
void HostReadImage(const HostImage* image, const size_t origin[3],
                   const size_t region[3], const size_t dst_row_pitch,
                   const size_t dst_slice_pitch, uint8_t* dst);

/// @brief Write to an image from the provided memory.
///
/// @param image Image to write into.
/// @param origin Position in pixels to begin writing into.
/// @param region Range in pixels to write into.
/// @param input_row_pitch Size in bytes of src row, must not be zero use
/// libimg::HostSetImagePitches() for default value.
/// @param input_slice_pitch Size in bytes of src slice, must not be zero use
/// libimg::HostSetImagePitches() for default value.
/// @param src Memory to read pixels from.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueWriteImage.html
void HostWriteImage(HostImage* image, const size_t origin[3],
                    const size_t region[3], const size_t input_row_pitch,
                    const size_t input_slice_pitch, const uint8_t* src);

/// @brief Fill an image with a color.
///
/// @param image Image to fill.
/// @param fill_color Color to fill image with.
/// @param origin Position in pixels to begin filling.
/// @param region Range in pixels to fill.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueFillImage.html
void HostFillImage(HostImage* image, const void* fill_color,
                   const size_t origin[3], const size_t region[3]);

/// @brief Copy from source image to destination image.
///
/// @param src_image Source image to read from.
/// @param dst_image Destination image to write into.
/// @param src_origin Source position in pixels to begin reading from.
/// @param dst_origin Destination position in pixels to begin writing to.
/// @param region Range in pixels to copy.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyImage.html
void HostCopyImage(const HostImage* src_image, HostImage* dst_image,
                   const size_t src_origin[3], const size_t dst_origin[3],
                   const size_t region[3]);

/// @brief Copy from source image to destination buffer.
///
/// @param src_image Source image to copy from.
/// @param dst_buffer Destination buffer to copy into.
/// @param src_origin Source position in pixels to begin copying from.
/// @param region Range in pixels to copy.
/// @param dst_offset Destination position in bytes to begin copying into.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyImageToBuffer.html
void HostCopyImageToBuffer(const HostImage* src_image, void* dst_buffer,
                           const size_t src_origin[3], const size_t region[3],
                           const size_t dst_offset);

/// @brief Copy from source buffer to destination image.
///
/// @param src_buffer Source buffer to copy from.
/// @param dst_image Destination image to copy into.
/// @param src_offset Source position in bytes to begin copying from.
/// @param dst_origin Destination position in pixels to begin copying into.
/// @param region Range in pixels to copy.
///
/// @sa
/// http://khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyBufferToImage.html
void HostCopyBufferToImage(const void* src_buffer, HostImage* dst_image,
                           const size_t src_offset, const size_t dst_origin[3],
                           const size_t region[3]);
/// @}

/// @brief Host side query functions.
///
/// @weakgroup Query
/// @{

/// @brief Get the raw data for the given image.
///
/// @param image Valid image object, must not be null.
///
/// @return Pointer to the raw image data.
void* HostGetImageStoragePtr(HostImage* image);

/// @brief Get a pointer to the kernel image data.
///
/// @param image Image to query.
///
/// @return Pointer beginning to kernel image data.
void* HostGetImageKernelImagePtr(HostImage* image);

/// @brief Query the required allocation size for an image.
///
/// @param flags Memory access and usage flags.
/// @param image_format Description of pixel layout and storage.
/// @param image_desc Description of image dimensions.
///
/// @return Size in bytes of required allocation.
uint64_t HostGetImageAllocationSize(const cl_mem_flags flags,
                                    const cl_image_format& image_format,
                                    const cl_image_desc& image_desc);

/// @brief Query the required size to store the image data.
///
/// @param image_format Description of pixel layout and storage.
/// @param image_desc Description of image dimensions.
///
/// @return Size in bytes of required image storage.
size_t HostGetImageStorageSize(const cl_image_format& image_format,
                                 const cl_image_desc& image_desc);

/// @brief Calculate the size in bytes of a single image pixel.
///
/// @param image_format OpenCL image format descriptor.
///
/// @return pixel size in bytes.
size_t HostGetPixelSize(const cl_image_format& image_format);

/// @brief Calculate the offset in bytes of the image origin.
///
/// @param format Image format, describing the pizel size.
/// @param desc Image descriptor, describing the image type and row pitch and
/// slice pitch. The descriptor values must correspond to the info queryable via
/// clGetImageInfo, otherwise behavior is undefined.
/// @param origin Position of the origin in the image.
///
/// @return the offset in bytes of the origin.
size_t HostGetImageOriginOffset(const cl_image_format& format,
                                const cl_image_desc& desc,
                                const size_t origin[3]);

/// @brief Calculate the size in bytes of an image region.
///
/// @param format Image format, describing pixel size.
/// @param type Image type, describing the image dimensions.
/// @param region Dimensions of the region.
///
/// @return The size in bytes of the region.
size_t HostGetImageRegionSize(const cl_image_format& format,
                              const cl_mem_object_type type,
                              const size_t region[3]);

/// @brief Returns number of and list of supported_image formats
///
/// @param[in] flags Memory allocation flags used for the query.
/// @param[in] image_type Type of image to query for supported image formats.
/// @param[in] num_entries Number of entries in the @p image_formats list.
/// @param[out] image_formats List of image formats to be populated.
/// @param[out] num_image_formats Return number of entries if not null.
///
/// @return Return error code.
cl_int HostGetSupportedImageFormats(const cl_mem_flags flags,
                                    const cl_mem_object_type image_type,
                                    const cl_uint num_entries,
                                    cl_image_format* image_formats,
                                    cl_uint* num_image_formats);

/// @brief Set the row and slice pitch for an image.
///
/// If a pitch is 0, then compute the minimum required value for it.
/// Helper function called by clEnqueueReadImage and clEnqueueWriteImage. See
/// specification of row_pitch and slice_pitch for clEnqueueReadImage or see
/// specification for input_row_pitch and input_slice_pitch of
/// clEnqueueWriteImage.
///
/// @param image_format Description of the image format.
/// @param image_desc Description of the image dimensions.
/// @param region Area of the image to be manipulated.
/// @param host_row_pitch Size in bytes of an image row to be adjusted, must not
/// be null.
/// @param host_slice_pitch Size in bytes of an image slice to be adjusted, must
/// not be null.
void HostSetImagePitches(const cl_image_format& image_format,
                         const cl_image_desc& image_desc,
                         const size_t region[3], size_t* host_row_pitch,
                         size_t* host_slice_pitch);
/// @}
}  // namespace libimg

#endif  // CODEPLAY_IMG_HOST_H
