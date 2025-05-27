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

#include <stub/stub.h>

mux_result_t stubGetDeviceInfos(uint32_t device_types,
                                uint64_t device_infos_length,
                                mux_device_info_t *out_device_infos,
                                uint64_t *out_device_infos_length) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCreateDevices(uint64_t devices_length,
                               mux_device_info_t *device_infos,
                               mux_allocator_info_t allocator_info,
                               mux_device_t *out_devices) {
  return mux_error_feature_unsupported;
}

void stubDestroyDevice(mux_device_t device,
                       mux_allocator_info_t allocator_info) {}

mux_result_t stubAllocateMemory(mux_device_t device, size_t size, uint32_t heap,
                                uint32_t memory_properties,
                                mux_allocation_type_e allocation_type,
                                uint32_t alignment,
                                mux_allocator_info_t allocator_info,
                                mux_memory_t *out_memory) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCreateMemoryFromHost(mux_device_t device, size_t size,
                                      void *host_pointer,
                                      mux_allocator_info_t allocator_info,
                                      mux_memory_t *out_memory) {
  return mux_error_feature_unsupported;
}

void stubFreeMemory(mux_device_t device, mux_memory_t memory,
                    mux_allocator_info_t allocator_info) {}

mux_result_t stubMapMemory(mux_device_t device, mux_memory_t memory,
                           uint64_t offset, uint64_t size, void **out_data) {
  return mux_error_feature_unsupported;
}

mux_result_t stubUnmapMemory(mux_device_t device, mux_memory_t memory) {
  return mux_error_feature_unsupported;
}

mux_result_t stubFlushMappedMemoryToDevice(mux_device_t device,
                                           mux_memory_t memory, uint64_t offset,
                                           uint64_t size) {
  return mux_error_feature_unsupported;
}

mux_result_t stubFlushMappedMemoryFromDevice(mux_device_t device,
                                             mux_memory_t memory,
                                             uint64_t offset, uint64_t size) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCreateBuffer(mux_device_t device, size_t size,
                              mux_allocator_info_t allocator_info,
                              mux_buffer_t *out_buffer) {
  return mux_error_feature_unsupported;
}

void stubDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                       mux_allocator_info_t allocator_info) {}

mux_result_t stubBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                  mux_buffer_t buffer, uint64_t offset) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCreateImage(mux_device_t device, mux_image_type_e type,
                             mux_image_format_e format, uint32_t width,
                             uint32_t height, uint32_t depth,
                             uint32_t array_layers, uint64_t row_size,
                             uint64_t slice_size,
                             mux_allocator_info_t allocator_info,
                             mux_image_t *out_image) {
  return mux_error_feature_unsupported;
}

void stubDestroyImage(mux_device_t device, mux_image_t image,
                      mux_allocator_info_t allocator_info) {}

mux_result_t stubBindImageMemory(mux_device_t device, mux_memory_t memory,
                                 mux_image_t image, uint64_t offset) {
  return mux_error_feature_unsupported;
}

mux_result_t stubGetSupportedImageFormats(mux_device_t device,
                                          mux_image_type_e image_type,
                                          mux_allocation_type_e allocation_type,
                                          uint32_t count,
                                          mux_image_format_e *out_formats,
                                          uint32_t *out_count) {
  return mux_error_feature_unsupported;
}

mux_result_t stubGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count) {
  return mux_error_feature_unsupported;
}

mux_result_t stubGetQueue(mux_device_t device, mux_queue_type_e queue_type,
                          uint32_t queue_index, mux_queue_t *out_queue) {
  return mux_error_feature_unsupported;
}
mux_result_t stubCreateFence(mux_device_t device,
                             mux_allocator_info_t allocator_info,
                             mux_semaphore_t *out_semaphore) {
  return mux_error_feature_unsupported;
}

void stubDestroyFence(mux_device_t device, mux_semaphore_t semaphore,
                      mux_allocator_info_t allocator_info) {}

mux_result_t stubCreateSemaphore(mux_device_t device,
                                 mux_allocator_info_t allocator_info,
                                 mux_semaphore_t *out_semaphore) {
  return mux_error_feature_unsupported;
}

void stubDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                          mux_allocator_info_t allocator_info) {}

mux_result_t stubCreateCommandBuffer(mux_device_t device,
                                     mux_callback_info_t callback_info,
                                     mux_allocator_info_t allocator_info,
                                     mux_command_buffer_t *out_command_buffer) {
  return mux_error_feature_unsupported;
}

mux_result_t stubFinalizeCommandBuffer(mux_command_buffer_t command_buffer) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCloneCommandBuffer(mux_device_t device,
                                    mux_allocator_info_t allocator_info,
                                    mux_command_buffer_t command_buffer,
                                    mux_command_buffer_t *out_command_buffer) {
  return mux_error_feature_unsupported;
}

void stubDestroyCommandBuffer(mux_device_t device,
                              mux_command_buffer_t command_buffer,
                              mux_allocator_info_t allocator_info) {}

mux_result_t stubCreateExecutable(mux_device_t device, const void *binary,
                                  uint64_t binary_length,
                                  mux_allocator_info_t allocator_info,
                                  mux_executable_t *out_executable) {
  return mux_error_feature_unsupported;
}

void stubDestroyExecutable(mux_device_t device, mux_executable_t executable,
                           mux_allocator_info_t allocator_info) {}

mux_result_t stubCreateKernel(mux_device_t device, mux_executable_t executable,
                              const char *name, uint64_t name_length,
                              mux_allocator_info_t allocator_info,
                              mux_kernel_t *out_kernel) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCreateBuiltInKernel(mux_device_t device, const char *name,
                                     uint64_t name_length,
                                     mux_allocator_info_t allocator_info,
                                     mux_kernel_t *out_kernel) {
  return mux_error_feature_unsupported;
}

mux_result_t stubQueryLocalSizeForSubGroupCount(mux_kernel_t, size_t, size_t *,
                                                size_t *, size_t *) {
  return mux_error_feature_unsupported;
}

mux_result_t stubQuerySubGroupSizeForLocalSize(mux_kernel_t, size_t, size_t,
                                               size_t, size_t *) {
  return mux_error_feature_unsupported;
}

mux_result_t stubQueryWFVInfoForLocalSize(mux_kernel_t, size_t, size_t, size_t,
                                          mux_wfv_status_e *, size_t *,
                                          size_t *, size_t *) {
  return mux_error_feature_unsupported;
}

void stubDestroyKernel(mux_device_t device, mux_kernel_t kernel,
                       mux_allocator_info_t allocator_info) {}

mux_result_t stubCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool) {
  return mux_error_feature_unsupported;
}

void stubDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                          mux_allocator_info_t allocator_info) {}

mux_result_t stubGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count) {
  return mux_error_feature_unsupported;
}

mux_result_t stubGetQueryPoolResults(mux_queue_t queue,
                                     mux_query_pool_t query_pool,
                                     uint32_t query_index, uint32_t query_count,
                                     size_t size, void *data, size_t stride) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandReadBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   void *host_pointer, uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandReadBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandWriteBuffer(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer, uint64_t offset,
    const void *host_pointer, uint64_t size,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandWriteBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t buffer,
    const void *host_pointer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandCopyBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t src_buffer, uint64_t src_offset,
                                   mux_buffer_t dst_buffer, uint64_t dst_offset,
                                   uint64_t size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandCopyBufferRegions(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_buffer_t dst_buffer, mux_buffer_region_info_t *regions,
    uint64_t regions_length, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandFillBuffer(mux_command_buffer_t command_buffer,
                                   mux_buffer_t buffer, uint64_t offset,
                                   uint64_t size, const void *pattern_pointer,
                                   uint64_t pattern_size,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandReadImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent, uint64_t row_size,
                                  uint64_t slice_size, void *pointer,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandWriteImage(mux_command_buffer_t command_buffer,
                                   mux_image_t image, mux_offset_3d_t offset,
                                   mux_extent_3d_t extent, uint64_t row_size,
                                   uint64_t slice_size, const void *pointer,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandFillImage(mux_command_buffer_t command_buffer,
                                  mux_image_t image, const void *color,
                                  uint32_t color_size, mux_offset_3d_t offset,
                                  mux_extent_3d_t extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandCopyImage(mux_command_buffer_t command_buffer,
                                  mux_image_t src_image, mux_image_t dst_image,
                                  mux_offset_3d_t src_offset,
                                  mux_offset_3d_t dst_offset,
                                  mux_extent_3d_t extent,
                                  uint32_t num_sync_points_in_wait_list,
                                  const mux_sync_point_t *sync_point_wait_list,
                                  mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandCopyImageToBuffer(
    mux_command_buffer_t command_buffer, mux_image_t src_image,
    mux_buffer_t dst_buffer, mux_offset_3d_t src_offset, uint64_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandCopyBufferToImage(
    mux_command_buffer_t command_buffer, mux_buffer_t src_buffer,
    mux_image_t dst_image, uint32_t src_offset, mux_offset_3d_t dst_offset,
    mux_extent_3d_t extent, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandNDRange(mux_command_buffer_t command_buffer,
                                mux_kernel_t kernel,
                                mux_ndrange_options_t options,
                                uint32_t num_sync_points_in_wait_list,
                                const mux_sync_point_t *sync_point_wait_list,
                                mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubUpdateDescriptors(mux_command_buffer_t command_buffer,
                                   mux_command_id_t command_id,
                                   uint64_t num_args, uint64_t *arg_indices,
                                   mux_descriptor_info_t *descriptors) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandUserCallback(
    mux_command_buffer_t command_buffer,
    void (*user_function)(mux_queue_t queue,
                          mux_command_buffer_t command_buffer,
                          void *const user_data),
    void *user_data, uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandBeginQuery(mux_command_buffer_t command_buffer,
                                   mux_query_pool_t query_pool,
                                   uint32_t query_index, uint32_t query_count,
                                   uint32_t num_sync_points_in_wait_list,
                                   const mux_sync_point_t *sync_point_wait_list,
                                   mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandEndQuery(mux_command_buffer_t command_buffer,
                                 mux_query_pool_t query_pool,
                                 uint32_t query_index, uint32_t query_count,
                                 uint32_t num_sync_points_in_wait_list,
                                 const mux_sync_point_t *sync_point_wait_list,
                                 mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubCommandResetQueryPool(
    mux_command_buffer_t command_buffer, mux_query_pool_t query_pool,
    uint32_t query_index, uint32_t query_count,
    uint32_t num_sync_points_in_wait_list,
    const mux_sync_point_t *sync_point_wait_list,
    mux_sync_point_t *sync_point) {
  return mux_error_feature_unsupported;
}

mux_result_t stubResetCommandBuffer(mux_command_buffer_t command_buffer) {
  return mux_error_feature_unsupported;
}

mux_result_t stubDispatch(
    mux_queue_t queue, mux_command_buffer_t command_buffer,
    mux_semaphore_t *wait_semaphores, uint32_t wait_semaphores_length,
    mux_semaphore_t *signal_semaphores, uint32_t signal_semaphores_length,
    void (*user_function)(mux_command_buffer_t command_buffer,
                          mux_result_t error, void *const user_data),
    void *user_data) {
  return mux_error_feature_unsupported;
}

mux_result_t stubTryWait(mux_queue_t queue,
                         mux_command_buffer_t command_buffer) {
  return mux_error_feature_unsupported;
}

mux_result_t stubWaitAll(mux_queue_t queue) {
  return mux_error_feature_unsupported;
}
