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
/// @brief Definition of common validations functions.

#ifndef CL_VALIDATE_H_INCLUDED
#define CL_VALIDATE_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>

#include <algorithm>
#include <cstdint>

namespace cl {
/// @addtogroup cl
/// @{

namespace validate {
/// @brief Validate an event wait list.
///
/// @param num_events Number of events in the wait list.
/// @param events Event wait list.
/// @param context The context to which the event in the wait list should
/// belong.
/// @param event Return event of the command, can be a nullptr
/// @param blocking Indicate if the event wait list is for a blocking command,
/// defaults to false.
///
/// @return
/// @retval `CL_SUCCESS` if the event wait list is valid.
/// @retval `CL_INVALID_EVENT_WAIT_LIST` is returned in the following cases:
/// * The number of events in the wait list is 0 and the wait list is not a
///   nullptr.
/// * The number of events in the wait list is more than 0 and the wait list is
///   a nullptr.
/// * One of the event in the wait list is a nullptr.
/// * One of the event in the wait list is the same as the event returned to
///   the user.
/// * If an event has a negative status (failed), whilst this is not specified
///   it will prevent deadlocks.
/// @retval `CL_INVALID_CONTEXT` is returned if one of the event in the wait
/// list is not in the provided context.
/// @retval `CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST` is returned if the
/// call is blocking and one of the event in the wait list is in a failed
/// status, that is to say its status is a negative integer.
inline cl_int EventWaitList(cl_uint num_events, const cl_event *events,
                            cl_context context, cl_event *event,
                            cl_bool blocking = CL_FALSE) {
  // If the event list is empty
  if (0 == num_events) {
    return (nullptr == events) ? CL_SUCCESS : CL_INVALID_EVENT_WAIT_LIST;
  }

  // If num_events is not 0 but the event list is empty
  if (nullptr == events) {
    return CL_INVALID_EVENT_WAIT_LIST;
  }

  for (cl_uint i = 0; i < num_events; ++i) {
    // If en event in the event list is a nullptr
    if (nullptr == events[i]) {
      return CL_INVALID_EVENT_WAIT_LIST;
    }

    // If the event is not in the command queue's context
    if (context != events[i]->context) {
      return CL_INVALID_CONTEXT;
    }

    // If the return event is in the event list
    if (event == &events[i]) {
      return CL_INVALID_EVENT_WAIT_LIST;
    }

    if (events[i]->command_status < 0) {
      // The event_wait_list contains an event with a negative status (failed).
      if (blocking) {
        // The call is blocking it must not contain a failed event.
        return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
      } else {
        // The call is non-blocking, failed events could cause a deadlock.
        return CL_INVALID_EVENT_WAIT_LIST;
      }
    }
  }

  return CL_SUCCESS;
}

/// @brief Validate that at least one device of the given context supports
/// images.
///
/// @param context OpenCL context whose devices are checked for image support.
///
/// @return cl_int CL_SUCCESS if at least one device of context supports images.
/// CL_INVALID_OPERATION if no device of context supports images.
inline cl_int ImageSupportForAnyDevice(const _cl_context *context) {
  return std::any_of(context->devices.begin(), context->devices.end(),
                     [](cl_device_id d) { return d->image_support == CL_TRUE; })
             ? CL_SUCCESS
             : CL_INVALID_OPERATION;
}

/// @brief Check if a given value is set in the given bit set.
///
/// @tparam T set type.
///
/// @param bitset Bit set.
/// @param value Value to check.
///
/// @return True if the value is in the bit set, false otherwise.
template <typename T>
inline bool IsInBitSet(const T bitset, const int32_t value) {
  T temp = static_cast<T>(value);

  return temp == (temp & bitset);
}

/// @brief Check if the given binary type is valid and supported.
///
/// @param[in] type Binary type to check.
///
/// @return True if the binary type is valid and supported, false otherwise.
inline bool BinaryType(const cl_program_binary_type type) {
  switch (type) {
    default:
      return false;
    case CL_PROGRAM_BINARY_TYPE_INTERMEDIATE:  // Fall through.
    case CL_PROGRAM_BINARY_TYPE_LIBRARY:       // Fall through.
    case CL_PROGRAM_BINARY_TYPE_EXECUTABLE:    // Fall through.
    case CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT:
      return true;
  }
}

/// @brief Validate `cl_mem_flags` for creating buffers and images.
///
/// Below is a table of the valid combinations of `cl_mem_flags`, the numbered
/// columns match the flag names in the left hand column.
///
/// | flag                | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
/// |---------------------|---|---|---|---|---|---|---|---|---|
/// | read write (1)      | / |   |   | x | x | x | x | x | x |
/// | write only (2)      |   | / |   | x | x | x | x | x | x |
/// | read only (3)       |   |   | / | x | x | x | x | x | x |
/// | use host ptr (4)    | x | x | x | / |   |   | x | x | x |
/// | alloc host ptr (5)  | x | x | x |   | / | x | x | x | x |
/// | copy host ptr (6)   | x | x | x |   | x | / | x | x | x |
/// | host write only (7) | x | x | x | x | x | x | / |   |   |
/// | host read only (8)  | x | x | x | x | x | x |   | / |   |
/// | host no access (9)  | x | x | x | x | x | x |   |   | / |
///
/// @param flags Bit field to validate.
/// @param host_ptr Host pointer referenced by some flags.
///
/// @return Returns `CL_SUCCESS` if the flags are valid, `CL_INVALID_VALUE` or
/// `CL_INVALID_HOST_PTR` otherwise.
inline cl_int MemFlags(cl_mem_flags flags, void *host_ptr) {
  // Mask the flags bit field related to device memory access.
  switch (flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY)) {
    case 0:  // Defaults to `CL_MEM_READ_WRITE`.
    case CL_MEM_READ_WRITE:
    case CL_MEM_READ_ONLY:
    case CL_MEM_WRITE_ONLY:
      break;
    default:  // All memory access flags are mutually exclusive.
      return CL_INVALID_VALUE;
  }

  // Mask the flags bit field related to host memory.
  switch (flags & (CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                   CL_MEM_COPY_HOST_PTR)) {
    case 0:  // Defaults to no `host_ptr`.
    case CL_MEM_ALLOC_HOST_PTR:
      if (host_ptr) {
        // Providing `host_ptr` is invalid when allocating host memory or for
        // the default case of no `host_ptr`.
        return CL_INVALID_HOST_PTR;
      }
      break;
    case CL_MEM_USE_HOST_PTR:
    case CL_MEM_COPY_HOST_PTR:
    case CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR:
      if (!host_ptr) {
        // Not providing `host_ptr` is invalid when specifying use host, copy
        // host, or both flags together.
        return CL_INVALID_HOST_PTR;
      }
      break;
    default:  // All other host memory flag combinations are invalid.
      return CL_INVALID_VALUE;
  }

  // Mask the flags bit field related to host memory access.
  switch (flags & (CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY |
                   CL_MEM_HOST_NO_ACCESS)) {
    case 0:  // Default to no host access.
    case CL_MEM_HOST_WRITE_ONLY:
    case CL_MEM_HOST_READ_ONLY:
    case CL_MEM_HOST_NO_ACCESS:
      break;
    default:  // All host memory access flags are mutually exclusive.
      return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

/// @brief Validate the user inputs passed to a copy buffer command
///
/// Used by both `clEnqueueCopyBuffer` and `clCommandCopyBufferKHR`.
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
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int CopyBufferArguments(cl_command_queue command_queue, cl_mem src_buffer,
                           cl_mem dst_buffer, size_t src_offset,
                           size_t dst_offset, size_t size);

/// @brief Validate the user inputs passed to a fill buffer command
///
/// Used by both `clEnqueueFillBuffer` and `clCommandFillBufferKHR`.
///
/// @param[in] command_queue Command queue to enqueue fill on.
/// @param[in] buffer Buffer to fill.
/// @param[in] pattern Pattern to fill buffer with.
/// @param[in] pattern_size Size in bytes of the pattern to fill with.
/// @param[in] offset Offset in bytes to begin the fill.
/// @param[in] size Size in bytes of the region to fill.
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int FillBufferArguments(cl_command_queue command_queue, cl_mem buffer,
                           const void *pattern, size_t pattern_size,
                           size_t offset, size_t size);

/// @brief Validate the user inputs passed to a copy buffer rect command
///
/// Used by both `clEnqueueCopyBufferRect` and `clCommandCopyBufferRectKHR`.
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
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int CopyBufferRectArguments(cl_command_queue command_queue,
                               cl_mem src_buffer, cl_mem dst_buffer,
                               const size_t *src_origin,
                               const size_t *dst_origin, const size_t *region,
                               size_t src_row_pitch, size_t src_slice_pitch,
                               size_t dst_row_pitch, size_t dst_slice_pitch);

/// @brief Validate the user inputs passed to a fill image command
///
/// Used by both `clEnqueueFillImage` and `clCommandFillImageKHR`.
///
/// @param[in] command_queue Command queue to enqueue the image fill on.
/// @param[in] image_ Image memory object to fill.
/// @param[in] fill_color Four element fill color value array.
/// @param[in] origin Origin offset in pixels to begin filling at.
/// @param[in] region Region size in pixels to fill.
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int FillImageArguments(cl_command_queue command_queue, cl_mem image_,
                          const void *fill_color, const size_t *origin,
                          const size_t *region);

/// @brief Validate the user inputs passed to a copy image command
///
/// Used by both `clEnqueueCopyImage` and `clCommandCopyImageKHR`.
///
/// @param[in] command_queue Command queue to enqueue the image copy on.
/// @param[in] src_image_ Source image memory object to copy from.
/// @param[in] dst_image_ Destination image memory object to copy to.
/// @param[in] src_origin Source origin offset in pixels to begin copy from.
/// @param[in] dst_origin Destination origin offset in pixels to begin copy to.
/// @param[in] region Region size in pixels to copy.
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int CopyImageArguments(cl_command_queue command_queue, cl_mem src_image_,
                          cl_mem dst_image_, const size_t *src_origin,
                          const size_t *dst_origin, const size_t *region);

/// @brief Validate the user inputs passed to a copy image to buffer command
///
/// Used by both `clEnqueueCopyImageToBuffer` and
/// `clCommandCopyImageToBufferKHR`.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_image_ Source image memory object to copy from.
/// @param[in] dst_buffer_ Destination buffer memory object to copy to.
/// @param[in] src_origin Source image origin offset in pixel to copy from.
/// @param[in] region Region size in pixels to copy from.
/// @param[in] dst_offset Destination buffer offset in bytes to copy to.
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int CopyImageToBufferArguments(cl_command_queue command_queue,
                                  cl_mem src_image_, cl_mem dst_buffer_,
                                  const size_t *src_origin,
                                  const size_t *region, size_t dst_offset);

/// @brief Validate the user inputs passed to a copy buffer to image command
///
/// Used by both `clEnqueueCopyBufferToImage` and
/// `clCommandCopyBufferToImageKHR`.
///
/// @param[in] command_queue Command queue to enqueue the copy on.
/// @param[in] src_buffer_ Source buffer memory object to copy from.
/// @param[in] dst_image_ Destination image memory object to copy to.
/// @param[in] src_offset Source buffer origin offset in bytes to copy from.
/// @param[in] dst_origin Destination image origin offset in pixels to copy to.
/// @param[in] region Region size in pixels to copy.
///
/// @return CL_SUCCESS if arguments are valid or an appropriate OpenCL error
/// code as defined by the specification.
cl_int CopyBufferToImageArguments(cl_command_queue command_queue,
                                  cl_mem src_buffer_, cl_mem dst_image_,
                                  size_t src_offset, const size_t *dst_origin,
                                  const size_t *region);
}  // namespace validate

/// @}
}  // namespace cl

#endif  // CL_VALIDATE_H_INCLUDED
