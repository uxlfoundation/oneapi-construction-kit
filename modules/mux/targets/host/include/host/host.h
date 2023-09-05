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

#ifndef HOST_HOST_H_INCLUDED
#define HOST_HOST_H_INCLUDED

#include <mux/mux.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/// @addtogroup host
/// @{

/// @brief Host major version number.
#define HOST_MAJOR_VERSION 0
/// @brief Host minor version number.
#define HOST_MINOR_VERSION 79
/// @brief Host patch version number.
#define HOST_PATCH_VERSION 0
/// @brief Host combined version number.
#define HOST_VERSION                      \
  (((uint32_t)HOST_MAJOR_VERSION << 22) | \
   ((uint32_t)HOST_MINOR_VERSION << 12) | ((uint32_t)HOST_PATCH_VERSION))

/// @brief Gets Mux devices' information.
///
/// @param[in] device_types A bitfield of `::mux_device_type_e` values to return
/// in `out_device_infos`.
/// @param[in] device_infos_length The length of out_device_infos. Must be 0, if
/// out_devices is null.
/// @param[out] out_device_infos Array of information for devices Mux knows
/// about, or null if an error occurred. Can be null, if out_device_infos_length
/// is non-null.
/// @param[out] out_device_infos_length The total number of devices for which we
/// are returning information, or 0 if an error occurred. Can be null, if
/// out_device_infos is non-null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostGetDeviceInfos(uint32_t device_types,
                                uint64_t device_infos_length,
                                mux_device_info_t *out_device_infos,
                                uint64_t *out_device_infos_length);

/// @param[in] devices_length The length of out_devices. Must be 0, if
/// out_devices is null.
/// @param[in] device_infos Array of device information determining which
/// devices to create.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_devices Array of devices Mux knows about, or null if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateDevices(uint64_t devices_length,
                               mux_device_info_t *device_infos,
                               mux_allocator_info_t allocator_info,
                               mux_device_t *out_devices);

/// @brief Destroy a device.
///
/// This function allows Mux to call into our partner's code to destroy a device
/// that was created previously by the partner.
///
/// @param[in] device A Mux device to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyDevice(mux_device_t device,
                       mux_allocator_info_t allocator_info);

/// @brief Allocate Mux device memory to be bound to a buffer or image.
///
/// This function uses a Mux device to allocate device memory which can later be
/// bound to buffers and images, providing physical memory backing, with the
/// ::muxBindBufferMemory and ::muxBindImageMemory functions.
///
/// @param[in] device A Mux device.
/// @param[in] size The size in bytes of memory to allocate.
/// @param[in] heap Value of a single set bit in the
/// `::mux_memory_requirements_s::supported_heaps` bitfield of the buffer or
/// image the device memory is being allocated for. Passing the value of `1` to
/// `heap` must result in a successful allocation, `heap` must not be `0`.
/// @param[in] memory_properties Bitfield of memory properties this allocation
/// should support, values from mux_memory_property_e.
/// @param[in] allocation_type The type of allocation.
/// @param[in] alignment Minimum alignment in bytes for the requested
/// allocation.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_memory The created memory, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostAllocateMemory(mux_device_t device, size_t size, uint32_t heap,
                                uint32_t memory_properties,
                                mux_allocation_type_e allocation_type,
                                uint32_t alignment,
                                mux_allocator_info_t allocator_info,
                                mux_memory_t *out_memory);

/// @brief Assigns Mux device visible memory from pre-allocated host side
/// memory.
///
/// This function takes a pointer to pre-allocated host memory and binds it to
/// device visible memory, which is cache coherent with the host allocation.
/// Entry point is optional and must return mux_error_feature_unsupported if
/// device doesn't support mux_allocation_capabilities_cached_host.
///
/// @param[in] device A Mux device.
/// @param[in] size The size in bytes of allocated memory.
/// @param[in] host_pointer Pointer to pre-allocated host addressable memory,
/// must not be null.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_memory The created memory, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
///
/// @see mux_device_info_s::allocation_capabilities
mux_result_t hostCreateMemoryFromHost(mux_device_t device, size_t size,
                                      void *host_pointer,
                                      mux_allocator_info_t allocator_info,
                                      mux_memory_t *out_memory);

/// @brief Free Mux device memory after use has finished.
///
/// This function uses the Mux device used for allocation with either
/// ::muxAllocateMemory or ::muxCreateMemoryFromHost to deallocate the device
/// memory.
///
/// @param[in] device The Mux device the memory was allocated for.
/// @param[in] memory The Mux device memory to be freed.
/// @param[in] allocator_info Allocator information.
void hostFreeMemory(mux_device_t device, mux_memory_t memory,
                    mux_allocator_info_t allocator_info);

/// @brief Map Mux device memory to a host address.
///
/// @param[in] device The device where the device memory is allocated.
/// @param[in] memory The memory to be mapped to a host accessible memory
/// address.
/// @param[in] offset Offset in bytes into the device memory to map to host
/// addressable memory.
/// @param[in] size Size in bytes of device memory to map to host addressable
/// memory.
/// @param[out] out_data Pointer to returned mapped host address, must not be
/// null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostMapMemory(mux_device_t device, mux_memory_t memory,
                           uint64_t offset, uint64_t size, void **out_data);

/// @brief Unmap a mapped device memory.
///
/// @param[in] device The device where the device memory is allocated.
/// @param[in] memory The device memory to unmap.
mux_result_t hostUnmapMemory(mux_device_t device, mux_memory_t memory);

/// @brief Explicitly update device memory with data residing in host memory.
///
/// hostFlushMappedMemoryToDevice is intended to be used with mux_memory_ts
/// allocated with the mux_memory_property_host_cached flag set. It updates
/// device memory with the content currently residing in host memory.
///
/// @param[in] device The device where the memory is allocated.
/// @param[in] memory The device memory to be flushed.
/// @param[in] offset The offset in bytes into the device memory to begin the
/// range.
/// @param[in] size The size in bytes of the range to flush.
///
/// @return Returns mux_success on success or mux_error_invalid_value when; @p
/// device is not a valid mux_device_t, @p memory is not a valid mux_memory_t,
/// @p offset combined with @p size is greater than @p memory size.
mux_result_t hostFlushMappedMemoryToDevice(mux_device_t device,
                                           mux_memory_t memory, uint64_t offset,
                                           uint64_t size);

/// @brief Explicitly update host memory with data residing in device memory.
///
/// hostFlushMappedMemoryFromDevice is intended to be used with mux_memory_ts
/// allocated with the mux_memory_property_host_cached flag set. It updates host
/// memory with the content currently residing in device memory.
///
/// @param[in] device The device where the memory is allocated.
/// @param[in] memory The device memory to be flushed.
/// @param[in] offset The offset in bytes into the device memory to begin the
/// range.
/// @param[in] size The size in bytes of the range to flush.
///
/// @return Returns mux_success on success or mux_error_invalid_value when; @p
/// device is not a valid mux_device_t, @p memory is not a valid mux_memory_t,
/// @p offset combined with @p size is greater than @p memory size.
mux_result_t hostFlushMappedMemoryFromDevice(mux_device_t device,
                                             mux_memory_t memory,
                                             uint64_t offset, uint64_t size);

/// @brief Create buffer memory.
///
/// The function uses a Mux device, and is used to call into the Mux device code
/// to create a buffer of a specific type and size.
///
/// @param[in] device A Mux device.
/// @param[in] size The size (in bytes) of buffer requested.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_buffer The created buffer, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateBuffer(mux_device_t device, size_t size,
                              mux_allocator_info_t allocator_info,
                              mux_buffer_t *out_buffer);

/// @brief Destroy a buffer.
///
/// This function allows Mux to call into our partner's code to destroy a buffer
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the buffer was created with.
/// @param[in] buffer The buffer to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                       mux_allocator_info_t allocator_info);

/// @brief Bind Mux device memory to the Mux buffer.
///
/// Bind device memory to a buffer providing physical backing so it can be used
/// when processing a ::mux_command_buffer_t.
///
/// @param[in] device The device on which the memory resides.
/// @param[in] memory The device memory to be bound to the buffer.
/// @param[in] buffer The buffer to which the device memory is to be bound.
/// @param[in] offset The offset into device memory to bind to the buffer.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                  mux_buffer_t buffer, uint64_t offset);

/// @brief Create an image.
///
/// The function uses a Mux device to create an image with specific type,
/// format, and dimensions. Newly created images are by default in the @p
/// ::mux_image_tiling_linear mode.
///
/// @param[in] device A Mux device.
/// @param[in] type The type of image to create.
/// @param[in] format The pixel data format of the image to create.
/// @param[in] width The width of the image in pixels, must be greater than one
/// and less than max width.
/// @param[in] height The height of the image in pixels, must be zero for 1D
/// images, must be greater than one and less than max height for 2D and 3D
/// images.
/// @param[in] depth The depth of the image in pixels, must be zero for 1D and
/// 2D images, must be greater than one and less than max depth for 3D images.
/// @param[in] array_layers The number of layers in an image array, must be 0
/// for non image arrays, must be and less than max array layers for image
/// arrays.
/// @param[in] row_size The size of an image row in bytes.
/// @param[in] slice_size The size on an image slice in bytes.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_image The created image, or null if an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateImage(mux_device_t device, mux_image_type_e type,
                             mux_image_format_e format, uint32_t width,
                             uint32_t height, uint32_t depth,
                             uint32_t array_layers, uint64_t row_size,
                             uint64_t slice_size,
                             mux_allocator_info_t allocator_info,
                             mux_image_t *out_image);

/// @brief Destroy an image.
///
/// The function allows Mux to call into client code to destroy an image that
/// was previously created.
///
/// @param[in] device The device the image was created with.
/// @param[in] image The image to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyImage(mux_device_t device, mux_image_t image,
                      mux_allocator_info_t allocator_info);

/// @brief Bind Mux device memory to the Mux image.
///
/// Bind device memory to an image providing physical backing so it can be used
/// when processing a ::mux_command_buffer_t.
///
/// @param[in] device The device on which the memory resides.
/// @param[in] memory The device memory to be bound to the image.
/// @param[in] image The image to which the device memory is to be bound.
/// @param[in] offset The offset into device memory to bind the image.
///
/// @return Return mux_success on success, or a mux_error_* if an error
/// occurred.
mux_result_t hostBindImageMemory(mux_device_t device, mux_memory_t memory,
                                 mux_image_t image, uint64_t offset);

/// @brief Query the Mux device for a list of supported image formats.
///
/// @param[in] device The device to query for supported image formats.
/// @param[in] image_type The type of the image.
/// @param[in] allocation_type The required allocation capabilities of the
/// image.
/// @param[in] count The element count of the @p out_formats array, must be
/// greater than zero if @p out_formats is not null and zero otherwise.
/// @param[out] out_formats Return the list of supported formats, may be null.
/// Storage must be an array of @p out_count elements.
/// @param[out] out_count Return the number of supported formats, may be null.
///
/// @return Return mux_success on success, or a mux_error_* if an error
/// occurred.
mux_result_t hostGetSupportedImageFormats(mux_device_t device,
                                          mux_image_type_e image_type,
                                          mux_allocation_type_e allocation_type,
                                          uint32_t count,
                                          mux_image_format_e *out_formats,
                                          uint32_t *out_count);

/// @brief Get the list of supported query counters for a device's queue type.
///
/// @param[in] device A Mux device.
/// @param[in] queue_type The type of queue to get the supported query counters
/// list for.
/// @param[in] count The element count of the `out_counters` and
/// `out_descriptions` arrays, **must** be greater than zero if `out_counters`
/// is not null and zero otherwise.
/// @param[out] out_counters Return the list of supported query counters,
/// **may** be null. Storage **must** be an array of `count` elements.
/// @param[out] out_descriptions Return the list of descriptions of support
/// query counters, **may** be null. Storage **must** be an array of `count`
/// elements.
/// @param[out] out_count Return the total count of supported query counters.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count);

/// @brief Get a queue from the owning device.
///
/// The function uses a Mux device, and is used to call into the Mux code to
/// create a queue.
///
/// @param[in] device A Mux device.
/// @param[in] queue_type The type of queue we want.
/// @param[in] queue_index The index of the queue_type we want to get from the
/// device.
/// @param[out] out_queue The created queue, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostGetQueue(mux_device_t device, mux_queue_type_e queue_type,
                          uint32_t queue_index, mux_queue_t *out_queue);

/// @brief Create a fence.
///
/// The function uses Mux device, and is used to call into the Mux code to
/// create a fence.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_fence The created fence, or uninitialized if an error
/// occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateFence(mux_device_t device,
                             mux_allocator_info_t allocator_info,
                             mux_fence_t *out_fence);

/// @brief Destroy a fence.
///
/// This hook allows Mux to call into our partner's code to destroy a fence that
/// was created previously by the partner.
///
/// @param[in] device The Mux device the fence was created with.
/// @param[in] fence The fence to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyFence(mux_device_t device, mux_fence_t fence,
                      mux_allocator_info_t allocator_info);

/// @brief Reset a fence such that it has no previous signalled state.
///
/// @param[in] fence The fence to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostResetFence(mux_fence_t fence);

/// @brief Create a semaphore.
///
/// The function uses Mux device, and is used to call into the Mux code to
/// create a semaphore.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_semaphore The created semaphore, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateSemaphore(mux_device_t device,
                                 mux_allocator_info_t allocator_info,
                                 mux_semaphore_t *out_semaphore);

/// @brief Destroy a semaphore.
///
/// This hook allows Mux to call into our partner's code to destroy a semaphore
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the semaphore was created with.
/// @param[in] semaphore The semaphore to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                          mux_allocator_info_t allocator_info);

/// @brief Reset a semaphore such that it has no previous signalled state.
///
/// @param[in] semaphore The semaphore to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostResetSemaphore(mux_semaphore_t semaphore);

/// @brief Create a command buffer.
///
/// This function allows Mux to call into our partner's code to create a command
/// buffer. Command buffers are a construct whereby we can encapsulate a group
/// of commands for execution on a device.
///
/// @param[in] device A Mux device.
/// @param[in] callback_info User provided callback, which **may** be null, can
/// be used by the implementation to provide detailed messages about command
/// buffer execution to the user.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_command_buffer The newly created command buffer, or
/// uninitialized if an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateCommandBuffer(mux_device_t device,
                                     mux_callback_info_t callback_info,
                                     mux_allocator_info_t allocator_info,
                                     mux_command_buffer_t *out_command_buffer);

/// @brief Finalize a command buffer.
///
/// This function allows Mux to call into our partner's code to finalize a
/// command buffer. Finalized command buffers are in an immutable state and
/// cannot have further commands pushed to them.
///
/// @param[in] command_buffer A Mux command buffer.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostFinalizeCommandBuffer(mux_command_buffer_t command_buffer);

/// @brief Clone a command buffer.
///
/// This function allows Mux to call into our partner's code to clone a command
/// buffer.
///
/// @param[in] device A Mux device.
/// @param[in] allocator_info Allocator information.
/// @param[in] command_buffer A Mux command buffer to clone.
/// @param[out] out_command_buffer The newly created command buffer, or
/// uninitialized if an error occurred.
///
/// @return mux_success, a mux_error_* if an error occurred or
/// mux_error_feature_unsupported if cloning command buffers is not supported by
/// the device.
mux_result_t hostCloneCommandBuffer(mux_device_t device,
                                    mux_allocator_info_t allocator_info,
                                    mux_command_buffer_t command_buffer,
                                    mux_command_buffer_t *out_command_buffer);

/// @brief Destroy a command buffer.
///
/// This hook allows Mux to call into our partner's code to destroy a command
/// buffer that was created previously by the partner.
///
/// @param[in] device The Mux device the command buffer was created with.
/// @param[in] command_buffer The command buffer to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyCommandBuffer(mux_device_t device,
                              mux_command_buffer_t command_buffer,
                              mux_allocator_info_t allocator_info);

/// @brief Load a binary into an executable container.
///
/// This function takes a precompiled binary and turns it into an executable.
///
/// @param[in] device The Mux device to create the executable with.
/// @param[in] binary The source binary data.
/// @param[in] binary_length The length of the source binary (in bytes).
/// @param[in] allocator_info Allocator information.
/// @param[out] out_executable The newly created executable, or uninitialized if
/// an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateExecutable(mux_device_t device, const void *binary,
                                  uint64_t binary_length,
                                  mux_allocator_info_t allocator_info,
                                  mux_executable_t *out_executable);

/// @brief Destroy an executable.
///
/// This hook allows Mux to call into our partner's code to destroy an
/// executable that was created previously by the partner.
///
/// @param[in] device The Mux device the executable was created with.
/// @param[in] executable The executable to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyExecutable(mux_device_t device, mux_executable_t executable,
                           mux_allocator_info_t allocator_info);

/// @brief Create a kernel from an executable.
///
/// This function creates an object that represents a kernel function inside an
/// executable.
///
/// @param[in] device The Mux device to create the kernel with.
/// @param[in] executable A previously created Mux executable.
/// @param[in] name The name of the kernel we want to create.
/// @param[in] name_length The length (in bytes) of @p name.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_kernel The newly created kernel, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateKernel(mux_device_t device, mux_executable_t executable,
                              const char *name, uint64_t name_length,
                              mux_allocator_info_t allocator_info,
                              mux_kernel_t *out_kernel);

/// @brief Create a kernel from a built-in kernel name provided by the device.
///
/// This function creates an object that represents a kernel which is provided
/// by the device itself, rather than contained within a binary contained within
/// an executable.
///
/// @param[in] device The Mux device to create the kernel with.
/// @param[in] name The name of the kernel we want to create.
/// @param[in] name_length The length (in bytes) of @p name.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_kernel The newly created kernel, or uninitialized if an
/// error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateBuiltInKernel(mux_device_t device, const char *name,
                                     uint64_t name_length,
                                     mux_allocator_info_t allocator_info,
                                     mux_kernel_t *out_kernel);

/// @brief Query the maximum number of sub-groups that this kernel supports for
/// each work-group.
///
/// @param[in] kernel The Mux kernel to query the max number of subgroups for.
/// @param[out] out_max_num_sub_groups The maximum number of sub-groups this
/// kernel supports.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostQueryMaxNumSubGroups(mux_kernel_t kernel,
                                      size_t *out_max_num_sub_groups);

/// @brief Query the local size that would give the requested number of
/// sub-groups.
///
/// This function queries a kernel for the local size that would give the
/// requested number of sub-groups. It must return 0 if no local size would give
/// the requested number of sub-groups.
///
/// @param[in] kernel The Mux kernel to query the local size for.
/// @param[in] sub_group_count The requested number of sub-groups.
/// @param[out] out_local_size_x The local size in the x dimension which would
/// give the requested number of sub-groups.
/// @param[out] out_local_size_y The local size in the y dimension which would
/// give the requested number of sub-groups.
/// @param[out] out_local_size_z The local size in the z dimension which would
/// give the requested number of sub-groups.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                                size_t sub_group_count,
                                                size_t *out_local_size_x,
                                                size_t *out_local_size_y,
                                                size_t *out_local_size_z);

/// @brief Query the sub-group size for the given local size.
///
/// This function queries a kernel for maximum sub-group size that would exist
/// in a kernel enqueued with the local work-group size passed as parameters. A
/// kernel enqueue **may** include one sub-group with a smaller size when the
/// sub-group size doesn't evenly divide the local size.
///
/// @param[in] kernel The Mux kernel to query the sub-group size for.
/// @param[in] local_size_x The local size in the x dimension for which we wish
/// to query sub-group size.
/// @param[in] local_size_y The local size in the y dimension for which we wish
/// to query sub-group size.
/// @param[in] local_size_z The local size in the z dimension for which we wish
/// to query sub-group size.
/// @param[out] out_sub_group_size The maximum sub-group sub-group size for the
/// queried local size.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                               size_t local_size_x,
                                               size_t local_size_y,
                                               size_t local_size_z,
                                               size_t *out_sub_group_size);

/// @brief Query the whole function vectorization status and dimension work
/// widths for the given local size.
///
/// This function queries a kernel for the whole function vectorization status
/// and dimension work widths that would be applicable for a kernel enqueued
/// with the local work-group size passed as parameters. The number of
/// work-items executed per kernel invocation **shall** be equal to the
/// dimension work widths returned from this function.
///
/// @param[in] kernel The Mux kernel to query the WFV info for.
/// @param[in] local_size_x The local size in the x dimension for which we wish
/// to query WFV info.
/// @param[in] local_size_y The local size in the y dimension for which we wish
/// to query WFV info.
/// @param[in] local_size_z The local size in the z dimension for which we wish
/// to query WFV info.
/// @param[out] out_wfv_status The status of whole function vectorization for
/// the queried local size.
/// @param[out] out_work_width_x The work width in the x dimension for the
/// queried local size.
/// @param[out] out_work_width_y The work width in the y dimension for the
/// queried local size.
/// @param[out] out_work_width_z The work width in the z dimension for the
/// queried local size.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostQueryWFVInfoForLocalSize(
    mux_kernel_t kernel, size_t local_size_x, size_t local_size_y,
    size_t local_size_z, mux_wfv_status_e *out_wfv_status,
    size_t *out_work_width_x, size_t *out_work_width_y,
    size_t *out_work_width_z);

/// @brief Destroy a kernel.
///
/// This hook allows Mux to call into our partner's code to destroy a kernel
/// that was created previously by the partner.
///
/// @param[in] device The Mux device the kernel was created with.
/// @param[in] kernel The kernel to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                       mux_allocator_info_t allocator_info);

/// @brief Create a query pool.
///
/// This function creates a query pool into which data can be stored about the
/// runtime behavior of commands on a queue.
///
/// @param[in] queue The Mux queue to create the query pool with.
/// @param[in] query_type The type of query pool to create (one of the values in
/// the enum mux_query_type_e).
/// @param[in] query_count The number of queries to allocate storage for.
/// @param[in] query_counter_configs Array of query counter configuration data
/// with length `query_count`, **must** be provided when `query_type` is
/// `mux_query_type_counter`, otherwise **must** be NULL.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_query_pool The newly created query pool, or uninitialized if
/// an error occurred.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool);

/// @brief Destroy a query pool.
///
/// This hook allows Mux to call into our partner's code to destroy a query pool
/// that was created previously by the partner.
///
/// @param[in] queue The Mux queue the query pool was created with.
/// @param[in] query_pool The Mux query pool to destroy.
/// @param[in] allocator_info Allocator information.
void hostDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                          mux_allocator_info_t allocator_info);

/// @brief Get the number of passes required for valid query counter results.
///
/// @param[in] queue A Mux queue.
/// @param[in] query_count The number of elements in the array pointed to by
/// `query_info`.
/// @param[in] query_counter_configs Array of query counter configuration data
/// with length `query_count`, **must** be provided when `query_type` is
/// `mux_query_type_counter`, otherwise **must** be NULL.
/// @param[out] out_pass_count Return the number of passes required to produce
/// valid results for the given list of query counters.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count);

/// @brief Get query results previously written to the query pool.
///
/// This function copies the requested query pool results previously written
/// during runtime on the Mux queue into the user provided storage.
///
/// @param[in] queue The Mux queue the query pool was created with.
/// @param[in] query_pool The Mux query pool to get the results from.
/// @param[in] query_index The query index of the first query to get the result
/// of.
/// @param[in] query_count The number of queries to get the results of.
/// @param[in] size The size in bytes of the memory pointed to by `data`.
/// @param[out] data A pointer to the memory to write the query results into.
/// @param[in] stride The stride in bytes between query results to be written
/// into `data`.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostGetQueryPoolResults(mux_queue_t queue,
                                     mux_query_pool_t query_pool,
                                     uint32_t query_index, uint32_t query_count,
                                     size_t size, void *data, size_t stride);

/// @brief Push a read buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the read command to.
/// @param[in] buffer The buffer to read from.
/// @param[in] offset The offset (in bytes) into the buffer object to read.
/// @param[out] host_pointer The host pointer to write to.
/// @param[in] size The size (in bytes) of the buffer object to read.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandReadBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   void *host_pointer, uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a read buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the command to.
/// @param[in] buffer The buffer to read from.
/// @param[out] host_pointer The host pointer to write to.
/// @param[in] regions The regions to read from buffer to host_pointer.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandReadBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a write buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the write command to.
/// @param[in] buffer The buffer to write to.
/// @param[in] offset The offset (in bytes) into the buffer object to write.
/// @param[in] host_pointer The host pointer to read from.
/// @param[in] size The size (in bytes) of the buffer object to write.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandWriteBuffer(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer, uint64_t offset,
    const void *host_pointer, uint64_t size,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a write buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the command to.
/// @param[in] buffer The buffer to write to.
/// @param[in] host_pointer The host pointer to read from.
/// @param[in] regions The regions to read from host_pointer to buffer.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandWriteBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    const void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a copy buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the copy command to.
/// @param[in] src_buffer The source buffer to copy from.
/// @param[in] src_offset The offset (in bytes) into the source buffer to copy.
/// @param[in] dst_buffer The destination buffer to copy to.
/// @param[in] dst_offset The offset (in bytes) into the destination buffer to
/// copy.
/// @param[in] size The size (in bytes) to copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t src_buffer, uint64_t src_offset,
                                   mux_buffer_t dst_buffer, uint64_t dst_offset,
                                   uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a copy buffer regions command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the copy command to.
/// @param[in] src_buffer The source buffer to copy from.
/// @param[in] dst_buffer The destination buffer to copy to.
/// @param[in] regions The regions to copy from src to dst.
/// @param[in] regions_length The length of regions.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandCopyBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_buffer_t dst_buffer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a write buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer push the fill command to.
/// @param[in] buffer The buffer to fill.
/// @param[in] offset The offset (in bytes) into the buffer object to fill.
/// @param[in] size The size (in bytes) of the buffer object to fill.
/// @param[in] pattern_pointer The pointer to the pattern to read and fill the
/// buffer with, this data must be copied by the device.
/// @param[in] pattern_size The size (in bytes) of `pattern_pointer`, the
/// maximum size is 128 bytes which is the size of the largest supported vector
/// type.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandFillBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   uint64_t size, const void *pattern_pointer,
                                   uint64_t pattern_size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a read image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image read command
/// to.
/// @param[in] image The image to read from.
/// @param[in] offset The x, y, z, offset in pixels into the image to read from.
/// @param[in] extent The width, height, depth in pixels of the image to read
/// from.
/// @param[in] row_size The row size in bytes of the host addressable pointer
/// data.
/// @param[in] slice_size The slice size in bytes of the host addressable
/// pointer data.
/// @param[out] pointer The host addressable pointer to image data to write
/// into.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandReadImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent, uint64_t row_size,
                                  uint64_t slice_size, void *pointer,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a write image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image write command
/// to.
/// @param[in] image The image to write to.
/// @param[in] offset The x, y, z, offset in pixel into the image to write to.
/// @param[in] extent The width, height, depth in pixels of the image to write
/// to.
/// @param[in] row_size The row size in bytes of the host addressable pointer
/// data.
/// @param[in] slice_size The slice size in bytes of the host addressable
/// pointer data.
/// @param[in] pointer The host addressable pointer to image data to read from.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandWriteImage(mux_command_buffer_t command_buffer,
                                   mux_image_t image, mux_offset_3d_t offset,
                                   mux_extent_3d_t extent, uint64_t row_size,
                                   uint64_t slice_size, const void *pointer,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a fill image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image fill command
/// to.
/// @param[in] image The image to fill.
/// @param[in] color The color to fill the image with, this data must be copied
/// by the device.
/// @param[in] color_size The size (in bytes) of the color.
/// @param[in] offset The x, y, z, offset in pixels into the image to fill.
/// @param[in] extent The width, height, depth in pixels of the image to fill.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandFillImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, const void *color,
                                  uint32_t color_size, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a copy image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image copy command
/// to.
/// @param[in] src_image The source image where data will be copied from.
/// @param[in] dst_image The destination image where data will be copied to.
/// @param[in] src_offset The x, y, z offset in pixels into the source image.
/// @param[in] dst_offset The x, y, z offset in pixels into the destination
/// image.
/// @param[in] extent The width, height, depth in pixels range of the images to
/// be copied.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandCopyImage(mux_command_buffer_t command_buffer,
                                  mux_image_t src_image, mux_image_t dst_image,
                                  mux_offset_3d_t src_offset,
                                  mux_offset_3d_t dst_offset,
                                  mux_extent_3d_t extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point);

/// @brief Push a copy image to buffer command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the image to buffer
/// copy to.
/// @param[in] src_image The source image to copy data from.
/// @param[in] dst_buffer The destination buffer to copy data to.
/// @param[in] src_offset The x, y, z, offset in pixels into the source image to
/// copy.
/// @param[in] dst_offset The offset in bytes into the destination buffer to
/// copy.
/// @param[in] extent The width, height, depth in pixels of the image to copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a copy buffer to image command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the buffer to image
/// copy command to.
/// @param[in] src_buffer The source buffer to copy data from.
/// @param[in] dst_image The destination image to copy data to.
/// @param[in] src_offset The offset in bytes into the source buffer to copy.
/// @param[in] dst_offset The x, y, z, offset in pixels into the destination
/// image to copy.
/// @param[in] extent The width, height, depth in pixels range of the data to
/// copy.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push an N-Dimensional run command to the command buffer.
///
/// @param[in] command_buffer The command buffer to push the N-Dimensional run
/// command to.
/// @param[in] kernel The kernel to execute.
/// @param[in] options The execution options to use during the run command.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandNDRange(mux_command_buffer_t command_buffer,
                                mux_kernel_t kernel,
                                mux_ndrange_options_t options,
                                uint32_t num_sync_points_in_wait_list,
                                const mux_sync_point_t *sync_point_wait_list,
                                mux_sync_point_t *sync_point);

/// @brief Update arguments to an N-Dimensional run command within the command
/// buffer.
///
/// @param[in] command_buffer The command buffer containing an N-Dimensional run
/// command to update.
/// @param[in] command_id The unique ID representing the index of the ND range
/// command within its containing command buffer.
/// @param[in] num_args Number of arguments to the kernel to be updated.
/// @param[in] arg_indices Indices of the arguments to be updated in the order
/// they appear as parameters to the kernel.
/// @param[in] descriptors Array of num_args argument descriptors which are the
/// new values for the arguments to the kernel.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostUpdateDescriptors(mux_command_buffer_t command_buffer,
                                   mux_command_id_t command_id,
                                   uint64_t num_args, uint64_t *arg_indices,
                                   mux_descriptor_info_t *descriptors);

/// @brief Push a user callback to the command buffer.
///
/// User callback commands allow a command to be placed within a command buffer
/// that will call back into user code. Care should be taken that the work done
/// within the user callback is minimal, as this will stall any forward progress
/// on the command buffer.
///
/// @param[in] command_buffer The command buffer to push the user callback
/// command to.
/// @param[in] user_function The user callback to invoke, must not be null.
/// @param[in] user_data The data to pass to the user callback on invocation,
/// may be null.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostCommandUserCallback(
    mux_command_buffer_t command_buffer,
    mux_command_user_callback_t user_function, void *user_data,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Push a begin query command to the command buffer.
///
/// Begin query commands enable a query pool for use storing query results.
///
/// @param[in] command_buffer The command buffer to push the begin query command
/// to.
/// @param[in] query_pool The query pool to store the query result in.
/// @param[in] query_index The initial query slot index that will contain the
/// result.
/// @param[in] query_count The number of query slots to enable.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t hostCommandBeginQuery(mux_command_buffer_t command_buffer,
                                   mux_query_pool_t query_pool,
                                   uint32_t query_index, uint32_t query_count,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point);

/// @brief Push a begin query command to the command buffer.
///
/// End query commands disable a query pool from use for storing query results.
///
/// @param[in] command_buffer The command buffer to push the end query command
/// to.
/// @param[in] query_pool The query pool the result is stored in.
/// @param[in] query_index The initial query slot index that contains the
/// result.
/// @param[in] query_count The number of query slots to disable.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t hostCommandEndQuery(mux_command_buffer_t command_buffer,
                                 mux_query_pool_t query_pool,
                                 uint32_t query_index, uint32_t query_count,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point);

/// @brief Push a reset query pool command to the command buffer.
///
/// Reset query pool commands enable reuse of the query pool as if it was newly
/// created.
///
/// @param[in] command_buffer The command buffer to push the query pool reset
/// command to.
/// @param[in] query_pool The query pool to reset.
/// @param[in] query_index The first query index to reset.
/// @param[in] query_count The number of query slots to reset.
/// @param[in] num_sync_points_in_wait_list Number of items in
/// sync_point_wait_list.
/// @param[in] sync_point_wait_list List of sync-points that need to complete
/// before this command can be executed.
/// @param[out] sync_point Returns a sync-point identifying this command, which
/// may be passed as NULL, that other commands in the command-buffer can wait
/// on.
///
/// @return mux_success, or a mux_error_* error code in an error occurred.
mux_result_t hostCommandResetQueryPool(
    mux_command_buffer_t command_buffer, mux_query_pool_t query_pool,
    uint32_t query_index, uint32_t query_count,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list, mux_sync_point_t *sync_point);

/// @brief Reset a command buffer.
///
/// @param[in] command_buffer The command buffer to reset.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostResetCommandBuffer(mux_command_buffer_t command_buffer);

/// @brief Push a command buffer to a queue.
///
/// Push a command buffer to a queue.
///
/// @param[in] queue A Mux queue.
/// @param[in] command_buffer A command buffer to push to a queue.
/// @param[in] fence A fence to signal when the dispatch completes.
/// @param[in] wait_semaphores An array of semaphores that this dispatch must
/// wait on before executing.
/// @param[in] wait_semaphores_length The length of wait_semaphores.
/// @param[in] signal_semaphores An array of semaphores that this dispatch will
/// signal when complete.
/// @param[in] signal_semaphores_length The length of signal_semaphores.
/// @param[in] user_function The command buffer complete callback, may be null.
/// @param[in] user_data The data to pass to the command buffer complete
/// callback on completion, may be null.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostDispatch(
    mux_queue_t queue, mux_command_buffer_t command_buffer, mux_fence_t fence,
    mux_semaphore_t *wait_semaphores, uint32_t wait_semaphores_length,
    mux_semaphore_t *signal_semaphores, uint32_t signal_semaphores_length,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data);

/// @brief Try to wait for a fence to be signaled.
///
/// If the fence is not yet signaled, return mux_fence_not_ready.
///
/// @param[in] queue A Mux queue.
/// @param[in] timeout The timeout period in units of nanoseconds. If the fence
/// waited on isn't signaled before timeout nanoseconds after the API is called,
/// tryWait will return with the fence having not been signaled. timeout ==
/// UINT64_MAX indicates an infinite timeout period and the entry point will
/// block until the fence is signaled.
/// @param[in] fence A fence to wait on.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostTryWait(mux_queue_t queue, uint64_t timeout,
                         mux_fence_t fence);

/// @brief Wait for all previously pushed command buffers to complete.
///
/// Wait for all previously pushed command buffers to complete.
///
/// @param[in] queue A Mux queue.
///
/// @return mux_success, or a mux_error_* if an error occurred.
mux_result_t hostWaitAll(mux_queue_t queue);

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // HOST_HOST_H_INCLUDED
