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
/// @brief Definitions for the OpenCL image API.

#ifndef CL_IMAGE_H_INCLUDED
#define CL_IMAGE_H_INCLUDED

// Third party headers
#include <CL/cl.h>
#include <cl/mem.h>
#include <mux/mux.h>

/// @addtogroup cl
/// @{

/// @brief OpenCL image.
///
/// While this struct inherits from _cl_mem it cannot be destroyed via a pointer
/// to _cl_mem. Query the _cl_mem::type to know if a _cl_mem pointer references
/// a buffer or an image and cast to _cl_mem_buffer or _cl_mem_image accordingly
/// before destroying the mem object. This is required to ensure that the ICD
/// dispatch table field is the first one in an OpenCL memory object.
typedef struct _cl_mem_image final : public _cl_mem {
  /// @brief Image constructor.
  ///
  /// @param context Context the image object belongs to.
  /// @param flags Memory allocation flags.
  /// @param image_format Image format descriptor.
  /// @param image_desc Image descriptor.
  /// @param host_ptr User provided host pointer, may be null.
  /// @param optional_parent Parent cl_mem for 1D image buffers.
  /// @param mux_memories Mux memory objects.
  /// @param mux_images Mux image objects.
  _cl_mem_image(cl_context context, cl_mem_flags flags,
                const cl_image_format *image_format,
                const cl_image_desc *image_desc, void *host_ptr,
                cl_mem optional_parent,
                cargo::dynamic_array<mux_memory_t> &&mux_memories,
                cargo::dynamic_array<mux_image_t> &&mux_images);

  /// @brief Deleted move constructor.
  ///
  /// By deleting the move constructor the implicit move assignment operator is
  /// also deleted along with the copy variants of both.
  _cl_mem_image(const _cl_mem_image &&) = delete;

  ~_cl_mem_image();

  /// @brief Description of the image format.
  cl_image_format image_format;
  /// @brief Description of the image dimensions and memory ownership.
  cl_image_desc image_desc;
  /// @brief Mux image object.
  cargo::dynamic_array<mux_image_t> mux_images;
} * cl_mem_image;

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL image memory object.
///
/// @param[in] context Context the image memory object belong to.
/// @param[in] flags Memory allocation flags.
/// @param[in] image_format Description of the image format.
/// @param[in] image_desc Description of the image.
/// @param[in] host_ptr User provided host pointer, may be null.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new image memory object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateImage.html
CL_API_ENTRY cl_mem CL_API_CALL CreateImage(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    const cl_image_desc *image_desc, void *host_ptr, cl_int *errcode_ret);

/// @brief Query the context for supported image formats.
///
/// @param[in] context Context to query for supported image formats.
/// @param[in] flags Memory allocation flags used for the query.
/// @param[in] image_type Type of image to query for supported image formats.
/// @param[in] num_entries Number of entries in the @a image_formats list.
/// @param[out] image_formats List of image formats to be populated.
/// @param[out] num_image_formats Return number of entries if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetSupportedImageFormats.html
CL_API_ENTRY cl_int CL_API_CALL GetSupportedImageFormats(
    cl_context context, cl_mem_flags flags, cl_mem_object_type image_type,
    cl_uint num_entries, cl_image_format *image_formats,
    cl_uint *num_image_formats);

/// @brief Enqueue an image read on the queue.
///
/// @param[in] command_queue Command queue to enqueue the image read on.
/// @param[in] image Image memory object to read from.
/// @param[in] blocking_read Blocking or non-blocking operation.
/// @param[in] origin Origin offset in pixels to begin reading from.
/// @param[in] region Region size in pixels to read.
/// @param[in] row_pitch Size in bytes of an output row.
/// @param[in] slice_pitch Size in bytes of an output slice.
/// @param[in] ptr Host pointer to read image data into.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueReadImage.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueReadImage(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
    const size_t *origin, const size_t *region, size_t row_pitch,
    size_t slice_pitch, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue an image write on the queue.
///
/// @param[in] command_queue Command queue to enqueue the image write on.
/// @param[in] image Image memory object to write into.
/// @param[in] blocking_write Blocking or non-blocking operation.
/// @param[in] origin Origin offset in pixels to begin writing to.
/// @param[in] region Region size in pixels to write.
/// @param[in] input_row_pitch Size in bytes of an input row.
/// @param[in] input_slice_pitch Size in bytes of an input slice.
/// @param[in] ptr Host pointer to write image data into.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueWriteImage.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueWriteImage(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
    const size_t *origin, const size_t *region, size_t input_row_pitch,
    size_t input_slice_pitch, const void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue an image fill on the queue.
///
/// @param[in] command_queue Command queue to enqueue the image fill on.
/// @param[in] image Image memory object to fill.
/// @param[in] fill_color Four element fill color value array.
/// @param[in] origin Origin offset in pixels to begin filling at.
/// @param[in] region Region size in pixels to fill.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueFillImage.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueFillImage(
    cl_command_queue command_queue, cl_mem image, const void *fill_color,
    const size_t *origin, const size_t *region, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue an image copy on the queue.
///
/// @param[in] command_queue Command queue to enqueue the image copy on.
/// @param[in] src_image Source image memory object to copy from.
/// @param[in] dst_image Destination image memory object to copy to.
/// @param[in] src_origin Source origin offset in pixels to begin copy from.
/// @param[in] dst_origin Destination origin offset in pixels to begin copy to.
/// @param[in] region Region size in pixels to copy.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyImage.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueCopyImage(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event);

/// @brief Enqueue an image to buffer copy on the queue.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_image Source image memory object to copy from.
/// @param[in] dst_buffer Destination buffer memory object to copy to.
/// @param[in] src_origin Source image origin offset in pixel to copy from.
/// @param[in] region Region size in pixels to copy from.
/// @param[in] dst_offset Destination buffer offset in bytes to copy to.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event in not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyImageToBuffer.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueCopyImageToBuffer(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
    const size_t *src_origin, const size_t *region, size_t dst_offset,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event);

/// @brief Enqueue a buffer to image copy on the queue.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_buffer Source buffer memory object to copy from.
/// @param[in] dst_image Destination image memory object to copy to.
/// @param[in] src_offset Source buffer origin offset in bytes to copy from.
/// @param[in] dst_origin Destination image origin offset in pixels to copy to.
/// @param[in] region Region size in pixels to copy.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyBufferToImage.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueCopyBufferToImage(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
    size_t src_offset, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event);

/// @brief Enqueue an image memory map on the queue.
///
/// @param[in] command_queue Command queue to enqueue the map on.
/// @param[in] image Image memory object to map.
/// @param[in] blocking_map Blocking or non-blocking queue operation.
/// @param[in] map_flags Type of map to perform, bit-field.
/// @param[in] origin Original offset in pixels to begin map at.
/// @param[in] region Region size in pixel to map.
/// @param[out] image_row_pitch Return image row pitch in bytes, must not be
/// null.
/// @param[out] image_slice_pitch Return image slice pitch in bytes, must not be
/// null.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List events to wait for.
/// @param[out] event Return event if not null.
/// @param[out] errcode_ret Return error code if no null.
///
/// @return Return void pointer to mapped image memory.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueMapImage.html
CL_API_ENTRY void *CL_API_CALL EnqueueMapImage(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
    cl_map_flags map_flags, const size_t *origin, const size_t *region,
    size_t *image_row_pitch, size_t *image_slice_pitch,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret);

/// @brief Query the image memory object for information.
///
/// @param[in] image Image memory object to query for information.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of @a param_value storage.
/// @param[out] param_value Pointer to value to store query in.
/// @param[out] param_value_size_ret Return required size in bytes to store
/// query in.
///
/// @return Return error code.
///
// @see
// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetImageInfo.html/
CL_API_ENTRY cl_int CL_API_CALL GetImageInfo(cl_mem image,
                                             cl_image_info param_name,
                                             size_t param_value_size,
                                             void *param_value,
                                             size_t *param_value_size_ret);

/// @brief Create a 2D image memory object, deprecated.
///
/// This function was deprecated in OpenCL 1.2 however we must still implement
/// it for backwards compatibility and conformance.
///
/// @param[in] context Context the image memory object belongs to.
/// @param[in] flags Memory allocation flags.
/// @param[in] image_format Description of the image format.
/// @param[in] image_width Width in pixels of the image.
/// @param[in] image_height Height in pixels of the image.
/// @param[in] image_row_pitch Size in bytes of a host pointer row.
/// @param[in] host_ptr User provided host pointer to initialise from, may be
/// null.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new image memory object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage2D.html
CL_API_ENTRY cl_mem CL_API_CALL CreateImage2D(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    size_t image_width, size_t image_height, size_t image_row_pitch,
    void *host_ptr, cl_int *errcode_ret);

/// @brief Create a 3D image memory object, deprecated.
///
/// The function was deprecated in OpenCL 1.2 however we must still implement it
/// for backwards compatibility and conformance.
///
/// @param[in] context Context the image memory object belongs to.
/// @param[in] flags Memory allocation flags.
/// @param[in] image_format Description of the image format.
/// @param[in] image_width Width in pixels of the image.
/// @param[in] image_height Height in pixels of the image.
/// @param[in] image_depth Depth in pixels of the image.
/// @param[in] image_row_pitch Size in bytes of a host pointer row.
/// @param[in] image_slice_pitch Size in bytes of a host pointer slice.
/// @param[in] host_ptr User provided host pointer to initialise from, may be
/// null.
/// @param[out] errcode_ret Return error code in not null.
///
/// @return Return the new image memory object.
///
/// @see
/// *http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage3D.html
CL_API_ENTRY cl_mem CL_API_CALL
CreateImage3D(cl_context context, cl_mem_flags flags,
              const cl_image_format *image_format, size_t image_width,
              size_t image_height, size_t image_depth, size_t image_row_pitch,
              size_t image_slice_pitch, void *host_ptr, cl_int *errcode_ret);

/// @}
}  // namespace cl

#endif  // CL_IMAGE_H_INCLUDED
