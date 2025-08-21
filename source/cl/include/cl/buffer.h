// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
/// @brief Definitions of the OpenCL buffer API.

#ifndef CL_BUFFER_H_INCLUDED
#define CL_BUFFER_H_INCLUDED

// Third party headers
#include <CL/cl.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <cl/mem.h>
#include <mux/mux.h>

#include <vector>

/// @addtogroup cl
/// @{

/// @brief Subclass of _cl_mem representing OpenCL buffer objects.
///
/// While this struct inherits from _cl_mem it cannot be destroyed via a pointer
/// to _cl_mem. Query the _cl_mem::type to know if a _cl_mem pointer references
/// a buffer or an image and cast to _cl_mem_buffer or _cl_mem_image accordingly
/// before destroying the mem object. This is required to ensure that the ICD
/// dispatch table field is the first one in an OpenCL memory object.
typedef struct _cl_mem_buffer final : public _cl_mem {
  /// @brief Buffer constructor with explicit reference count type.
  ///
  /// @param[in] context Context the buffer belongs to.
  /// @param[in] flags Memory allocation flags.
  /// @param[in] size Size in bytes of the buffer.
  /// @param[in] host_ptr Pointer to optionally provided host memory.
  /// @param[in] mux_memories List of mux memory objects.
  /// @param[in] mux_buffers List of mux buffer objects.
  _cl_mem_buffer(cl_context context, const cl_mem_flags flags,
                 const size_t size, void *host_ptr,
                 cargo::dynamic_array<mux_memory_t> &&mux_memories,
                 cargo::dynamic_array<mux_buffer_t> &&mux_buffers);

  /// @brief Sub buffer constructor.
  ///
  /// @param[in] flags Memory allocation flags.
  /// @param[in] offset Offset in bytes into the sub buffer object.
  /// @param[in] size Size in bytes of the buffer.
  /// @param[in] parent Parent buffer object.
  /// @param[in] mux_memories List of mux memory objects.
  /// @param[in] mux_buffers List of mux buffer objects.
  _cl_mem_buffer(const cl_mem_flags flags, const size_t offset,
                 const size_t size, cl_mem parent,
                 cargo::dynamic_array<mux_memory_t> &&mux_memories,
                 cargo::dynamic_array<mux_buffer_t> &&mux_buffers);

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  _cl_mem_buffer(_cl_mem_buffer &&) = delete;

  /// @brief Destructor.
  ~_cl_mem_buffer();

  /// @brief Create an OpenCL buffer memory object.
  ///
  /// @param[in] context Context the buffer belongs to.
  /// @param[in] flags Memory allocation flags.
  /// @param[in] size Size in bytes of the requested device allocation.
  /// @param[in] host_ptr Pointer to optionally provide user memory.
  ///
  /// @return Returns the buffer object on success.
  /// @retval `CL_OUT_OF_HOST_MEMORY` if an allocation failure occurred.
  /// @retval `CL_INVALID_BUFFER_SIZE` if the buffer size exceeds the maximum
  /// memory allocation size of any device.
  /// @retval `CL_MEM_OBJECT_ALLOCATION_FAILURE` if there is an issue when
  /// binding, mapping or flushing mapped memory.
  static cargo::expected<std::unique_ptr<_cl_mem_buffer>, cl_int>
  create(cl_context context, cl_mem_flags flags, size_t size, void *host_ptr);

  /// @brief Synchronize data when a buffer has multiple device in its context.
  ///
  /// When the context which created this buffer contains multiple devices the
  /// memory backing the buffer on each device must be kept in sync. This is
  /// done by mapping the memory from both devices to host and performing a
  /// `std::memcpy` of the whole memory region. When there is only a single
  /// device in the context no synchronization is done.
  ///
  /// @todo Currently pessimistic synchronization is performed, that is, the
  /// whole buffer is synchronized. For the sub-buffer case this should not be
  /// required, only the sub-buffer region of the core memory needs to be
  /// synchronized.
  ///
  /// @param[in] command_queue The queue containing the device which is required
  /// to be synchronized.
  ///
  /// @return Returns `CL_SUCCESS` or `CL_OUT_OF_RESOURCES` in the event of a
  /// failure.
  cl_int synchronize(cl_command_queue command_queue);

  // TODO: redmine(7057) Currently _cl_mem::optional_parent is where the parent
  // buffer is stored, given that sub buffers are not relevant to images in
  // OpenCL 1.2 there is no need to share this parent cl_mem. Should the parent
  // be stored here instead?

  /// @brief Offset in bytes into the sub buffer object.
  size_t offset;

  /// @brief Mux buffer objects, one per device in the parent `cl_context`.
  cargo::dynamic_array<mux_buffer_t> mux_buffers;

} *cl_mem_buffer;

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Create an OpenCL buffer memory object.
///
/// @param[in] context Context the buffer belongs to.
/// @param[in] flags Memory allocation flags.
/// @param[in] size Size in bytes of the requested device allocation.
/// @param[in] host_ptr Pointer to optionally provide user memory.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new buffer memory object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateBuffer.html
CL_API_ENTRY cl_mem CL_API_CALL CreateBuffer(cl_context context,
                                             cl_mem_flags flags, size_t size,
                                             void *host_ptr,
                                             cl_int *errcode_ret);

/// @brief Create an OpenCL sub buffer memory object from existing buffer.
///
/// @param[in] buffer Buffer object to create the sub buffer from.
/// @param[in] flags Memory allocation flags.
/// @param[in] buffer_create_type Type of sub buffer to create.
/// @param[in] buffer_create_info Information about how to create the sub
/// buffer.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new sub buffer memory object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateSubBuffer.html
CL_API_ENTRY cl_mem CL_API_CALL CreateSubBuffer(
    cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type,
    const void *buffer_create_info, cl_int *errcode_ret);

/// @brief Enqueue a rectangular region buffer write to the queue.
///
/// @param[in] command_queue Command queue to enqueue the write on.
/// @param[in] buffer Buffer object to write into.
/// @param[in] blocking_write Blocking or non-blocking queue operation.
/// @param[in] buffer_origin Offset in bytes into the buffer to begin writing
/// from.
/// @param[in] host_origin Offset in bytes into the host pointer to begin
/// reading from.
/// @param[in] region Rectangular region to write.
/// @param[in] buffer_row_pitch Size in bytes of a buffer row, may be 0.
/// @param[in] buffer_slice_pitch Size in bytes of a buffer slice, may be 0.
/// @param[in] host_row_pitch Size in bytes of a host pointer row, may be 0.
/// @param[in] host_slice_pitch Size in bytes of a host pointer slice, may be 0.
/// @param[in] ptr Host pointer to write from.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueWriteBufferRect.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueWriteBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, const void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event);

/// @brief Enqueue a rectangular region buffer read to the queue.
///
/// @param[in] command_queue Command queue to enqueue the read on.
/// @param[in] buffer Buffer object to read from.
/// @param[in] blocking_read Blocking or non-blocking queue operation.
/// @param[in] buffer_origin Offset into the buffer to begin reading from.
/// @param[in] host_origin Offset into the host pointer to begin writing from.
/// @param[in] region Rectangular region to read.
/// @param[in] buffer_row_pitch Size in bytes of a buffer row, may be 0.
/// @param[in] buffer_slice_pitch Size in bytes of a buffer slice, may be 0.
/// @param[in] host_row_pitch Size in bytes of a host pointer row, may be 0.
/// @param[in] host_slice_pitch Size in bytes of a host pointer slice, may be 0.
/// @param[in] ptr Host pointer to read into.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[out] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueReadBufferRect.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueReadBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event);

/// @brief Enqueue a rectangular region buffer copy to the queue.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_buffer Source buffer to copy from.
/// @param[in] dst_buffer Destination buffer to copy to.
/// @param[in] src_origin Offset in bytes into the source buffer to begin
/// copying from.
/// @param[in] dst_origin Offset in bytes into the destination buffer to begin
/// copying into.
/// @param[in] region Rectangular region to copy.
/// @param[in] src_row_pitch Size in bytes of a source buffer row, may be 0.
/// @param[in] src_slice_pitch Size in bytes of a source buffer slice, may be 0.
/// @param[in] dst_row_pitch Size in bytes of a destination buffer row, may be
/// 0.
/// @param[in] dst_slice_pitch Size in bytes of a destination buffer slice, may
/// be 0.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyBufferRect.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueCopyBufferRect(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
    size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a buffer map to the queue.
///
/// @param[in] command_queue Command queue to enqueue the map on.
/// @param[in] buffer Buffer to map.
/// @param[in] blocking_map Blocking or non-blocking queue operation.
/// @param[in] map_flags Type of map to perform, bit field.
/// @param[in] offset Offset in bytes into buffer to begin map.
/// @param[in] size Size in bytes of region to be mapped.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[in] event Return event in not null.
/// @param[out] errcode_ret Return error code in not null.
///
/// @return Return void pointer to mapped memory.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueMapBuffer.html
CL_API_ENTRY void *CL_API_CALL EnqueueMapBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
    cl_map_flags map_flags, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret);

/// @brief Enqueue a buffer write to the queue.
///
/// @param[in] command_queue Command queue to enqueue the write on.
/// @param[in] buffer Buffer object to write to.
/// @param[in] blocking_write Blocking or non-blocking queue operation.
/// @param[in] offset Offset in bytes into buffer to begin write.
/// @param[in] size Size in bytes of the region to write.
/// @param[in] ptr Host pointer to write buffer from.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event in not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueWriteBuffer.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer,
                   cl_bool blocking_write, size_t offset, size_t size,
                   const void *ptr, cl_uint num_events_in_wait_list,
                   const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a buffer read to the queue.
///
/// @param[in] command_queue Command queue to enqueue the read on.
/// @param[in] buffer Buffer object to read from.
/// @param[in] blocking_read Blocking or non-blocking queue operation.
/// @param[in] offset Offset in bytes into the buffer to read.
/// @param[in] size Size in bytes of the region to read.
/// @param[in] ptr Host pointer to read buffer into.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueReadBuffer.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueReadBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    size_t offset, size_t size, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a buffer copy to the queue.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_buffer Source buffer to copy from.
/// @param[in] dst_buffer Destination buffer to copy to.
/// @param[in] src_offset Source buffer offset in bytes to begin copy.
/// @param[in] dst_offset Destination buffer offset in bytes to begin copy.
/// @param[in] size Size in bytes of region to copy.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueCopyBuffer.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
                  cl_mem dst_buffer, size_t src_offset, size_t dst_offset,
                  size_t size, cl_uint num_events_in_wait_list,
                  const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a buffer fill on the queue.
///
/// @param[in] command_queue Command queue to enqueue fill on.
/// @param[in] buffer Buffer to fill.
/// @param[in] pattern Pattern to fill buffer with.
/// @param[in] pattern_size Size in bytes of the pattern to fill with.
/// @param[in] offset Offset in bytes to begin the fill.
/// @param[in] size Size in bytes of the region to fill.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueFillBuffer.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer,
                  const void *pattern, size_t pattern_size, size_t offset,
                  size_t size, cl_uint num_events_in_wait_list,
                  const cl_event *event_wait_list, cl_event *event);

/// @}
} // namespace cl

#endif // CL_BUFFER_H_INCLUDED
