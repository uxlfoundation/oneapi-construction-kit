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

// Set this so we can get clSetCommandQueueProperty
// as the expectation is we return an invalid operation.
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS 1

#include <CL/cl.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/platform.h>
#include <cl/program.h>
#include <cl/sampler.h>
#include <tracer/tracer.h>

CL_API_ENTRY cl_int CL_API_CALL clGetPlatformIDs(const cl_uint num_entries,
                                                 cl_platform_id *platforms,
                                                 cl_uint *const num_platforms) {
  return cl::GetPlatformIDs(num_entries, platforms, num_platforms);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                  size_t param_value_size, void *param_value,
                  size_t *const param_value_size_ret) {
  return cl::GetPlatformInfo(platform, param_name, param_value_size,
                             param_value, param_value_size_ret);
}

CL_API_ENTRY void *CL_API_CALL
clGetExtensionFunctionAddress(const char *func_name) {
  return cl::GetExtensionFunctionAddress(func_name);
}

CL_API_ENTRY void *CL_API_CALL clGetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) {
  return cl::GetExtensionFunctionAddressForPlatform(platform, func_name);
}

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDs(cl_platform_id platform,
                                               cl_device_type device_type,
                                               cl_uint num_entries,
                                               cl_device_id *devices,
                                               cl_uint *num_devices) {
  return cl::GetDeviceIDs(platform, device_type, num_entries, devices,
                          num_devices);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainDevice(cl_device_id device) {
  return cl::RetainDevice(device);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseDevice(cl_device_id device) {
  return cl::ReleaseDevice(device);
}

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceInfo(cl_device_id device,
                                                cl_device_info param_name,
                                                size_t param_value_size,
                                                void *param_value,
                                                size_t *param_value_size_ret) {
  return cl::GetDeviceInfo(device, param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clCreateSubDevices(
    cl_device_id in_device, const cl_device_partition_property *properties,
    cl_uint num_devices, cl_device_id *out_devices, cl_uint *num_devices_ret) {
  return cl::CreateSubDevices(in_device, properties, num_devices, out_devices,
                              num_devices_ret);
}

CL_API_ENTRY cl_context CL_API_CALL clCreateContext(
    const cl_context_properties *properties, cl_uint num_devices,
    const cl_device_id *devices,
    void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *user_data, cl_int *errcode_ret) {
  return cl::CreateContext(properties, num_devices, devices, pfn_notify,
                           user_data, errcode_ret);
}

CL_API_ENTRY cl_context CL_API_CALL clCreateContextFromType(
    const cl_context_properties *properties, cl_device_type device_type,
    void(CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *user_data, cl_int *errcode_ret) {
  return cl::CreateContextFromType(properties, device_type, pfn_notify,
                                   user_data, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainContext(cl_context context) {
  return cl::RetainContext(context);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseContext(cl_context context) {
  return cl::ReleaseContext(context);
}

CL_API_ENTRY cl_int CL_API_CALL clGetContextInfo(cl_context context,
                                                 cl_context_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret) {
  return cl::GetContextInfo(context, param_name, param_value_size, param_value,
                            param_value_size_ret);
}

CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueue(
    cl_context context, cl_device_id device,
    cl_command_queue_properties properties, cl_int *errcode_ret) {
  return cl::CreateCommandQueue(context, device, properties, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) {
  return cl::RetainCommandQueue(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) {
  return cl::ReleaseCommandQueue(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL clGetCommandQueueInfo(
    cl_command_queue command_queue, cl_command_queue_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  return cl::GetCommandQueueInfo(command_queue, param_name, param_value_size,
                                 param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrierWithWaitList(cl_command_queue queue, cl_uint num_events,
                             const cl_event *event_list, cl_event *event) {
  return cl::EnqueueBarrierWithWaitList(queue, num_events, event_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarkerWithWaitList(cl_command_queue queue, cl_uint num_events,
                            const cl_event *event_list, cl_event *event) {
  return cl::EnqueueMarkerWithWaitList(queue, num_events, event_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueWaitForEvents(
    cl_command_queue queue, cl_uint num_events, const cl_event *event_list) {
  return cl::EnqueueWaitForEvents(queue, num_events, event_list);
}

CL_API_ENTRY cl_int CL_API_CALL clFlush(cl_command_queue command_queue) {
  return cl::Flush(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL clFinish(cl_command_queue command_queue) {
  return cl::Finish(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueBarrier(cl_command_queue queue) {
  return cl::EnqueueBarrier(queue);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueMarker(cl_command_queue queue,
                                                cl_event *event) {
  return cl::EnqueueMarker(queue, event);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateBuffer(cl_context context,
                                               cl_mem_flags flags, size_t size,
                                               void *host_ptr,
                                               cl_int *errcode_ret) {
  return cl::CreateBuffer(context, flags, size, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateSubBuffer(
    cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type,
    const void *buffer_create_info, cl_int *errcode_ret) {
  return cl::CreateSubBuffer(buffer, flags, buffer_create_type,
                             buffer_create_info, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueReadBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    size_t offset, size_t size, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueReadBuffer(command_queue, buffer, blocking_read, offset,
                               size, ptr, num_events_in_wait_list,
                               event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer,
                     cl_bool blocking_write, size_t offset, size_t size,
                     const void *ptr, cl_uint num_events_in_wait_list,
                     const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueWriteBuffer(command_queue, buffer, blocking_write, offset,
                                size, ptr, num_events_in_wait_list,
                                event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueReadBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  return cl::EnqueueReadBufferRect(
      command_queue, buffer, blocking_read, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      ptr, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueWriteBufferRect(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    const size_t *buffer_origin, const size_t *host_origin,
    const size_t *region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, const void *ptr,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  return cl::EnqueueWriteBufferRect(
      command_queue, buffer, blocking_write, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      ptr, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer,
                    const void *pattern, size_t pattern_size, size_t offset,
                    size_t size, cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueFillBuffer(command_queue, buffer, pattern, pattern_size,
                               offset, size, num_events_in_wait_list,
                               event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
                    cl_mem dst_buffer, size_t src_offset, size_t dst_offset,
                    size_t size, cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueCopyBuffer(command_queue, src_buffer, dst_buffer,
                               src_offset, dst_offset, size,
                               num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueCopyBufferRect(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
    size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueCopyBufferRect(
      command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region,
      src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
      num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY void *CL_API_CALL clEnqueueMapBuffer(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
    cl_map_flags map_flags, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret) {
  return cl::EnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags,
                              offset, size, num_events_in_wait_list,
                              event_wait_list, event, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateImage(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    const cl_image_desc *image_desc, void *host_ptr, cl_int *errcode_ret) {
  return cl::CreateImage(context, flags, image_format, image_desc, host_ptr,
                         errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetSupportedImageFormats(
    cl_context context, cl_mem_flags flags, cl_mem_object_type image_type,
    cl_uint num_entries, cl_image_format *image_formats,
    cl_uint *num_image_formats) {
  return cl::GetSupportedImageFormats(context, flags, image_type, num_entries,
                                      image_formats, num_image_formats);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueReadImage(
    cl_command_queue command_queue, cl_mem image_, cl_bool blocking_read,
    const size_t *origin, const size_t *region, size_t row_pitch,
    size_t slice_pitch, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueReadImage(command_queue, image_, blocking_read, origin,
                              region, row_pitch, slice_pitch, ptr,
                              num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueWriteImage(
    cl_command_queue command_queue, cl_mem image_, cl_bool blocking_write,
    const size_t *origin, const size_t *region, size_t input_row_pitch,
    size_t input_slice_pitch, const void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueWriteImage(command_queue, image_, blocking_write, origin,
                               region, input_row_pitch, input_slice_pitch, ptr,
                               num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueFillImage(
    cl_command_queue command_queue, cl_mem image_, const void *fill_color,
    const size_t *origin, const size_t *region, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueFillImage(command_queue, image_, fill_color, origin, region,
                              num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueCopyImage(
    cl_command_queue command_queue, cl_mem src_image_, cl_mem dst_image_,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  return cl::EnqueueCopyImage(command_queue, src_image_, dst_image_, src_origin,
                              dst_origin, region, num_events_in_wait_list,
                              event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueCopyImageToBuffer(
    cl_command_queue command_queue, cl_mem src_image_, cl_mem dst_buffer_,
    const size_t *src_origin, const size_t *region, size_t dst_offset,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  return cl::EnqueueCopyImageToBuffer(
      command_queue, src_image_, dst_buffer_, src_origin, region, dst_offset,
      num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueCopyBufferToImage(
    cl_command_queue command_queue, cl_mem src_buffer_, cl_mem dst_image_,
    size_t src_offset, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  return cl::EnqueueCopyBufferToImage(
      command_queue, src_buffer_, dst_image_, src_offset, dst_origin, region,
      num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY void *CL_API_CALL clEnqueueMapImage(
    cl_command_queue command_queue, cl_mem memobj, cl_bool blocking_map,
    cl_map_flags map_flags, const size_t *origin, const size_t *region,
    size_t *image_row_pitch, size_t *image_slice_pitch,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret) {
  return cl::EnqueueMapImage(command_queue, memobj, blocking_map, map_flags,
                             origin, region, image_row_pitch, image_slice_pitch,
                             num_events_in_wait_list, event_wait_list, event,
                             errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
                        void *mapped_ptr, cl_uint num_events_in_wait_list,
                        const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueUnmapMemObject(command_queue, memobj, mapped_ptr,
                                   num_events_in_wait_list, event_wait_list,
                                   event);
}

CL_API_ENTRY cl_int CL_API_CALL clGetMemObjectInfo(
    cl_mem memobj, cl_mem_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  return cl::GetMemObjectInfo(memobj, param_name, param_value_size, param_value,
                              param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetImageInfo(cl_mem image,
                                               cl_image_info param_name,
                                               size_t param_value_size,
                                               void *param_value,
                                               size_t *param_value_size_ret) {
  return cl::GetImageInfo(image, param_name, param_value_size, param_value,
                          param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainMemObject(cl_mem memobj) {
  return cl::RetainMemObject(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseMemObject(cl_mem memobj) {
  return cl::ReleaseMemObject(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL clSetMemObjectDestructorCallback(
    cl_mem memobj,
    void(CL_CALLBACK *pfn_notify)(cl_mem memobj, void *user_data),
    void *user_data) {
  return cl::SetMemObjectDestructorCallback(memobj, pfn_notify, user_data);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueMigrateMemObjects(
    cl_command_queue queue, cl_uint num_mem_objects, const cl_mem *mem_objects,
    cl_mem_migration_flags flags, cl_uint num_events, const cl_event *events,
    cl_event *event) {
  return cl::EnqueueMigrateMemObjects(queue, num_mem_objects, mem_objects,
                                      flags, num_events, events, event);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateImage2D(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    size_t image_width, size_t image_height, size_t image_row_pitch,
    void *host_ptr, cl_int *errcode_ret) {
  return cl::CreateImage2D(context, flags, image_format, image_width,
                           image_height, image_row_pitch, host_ptr,
                           errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D(cl_context context, cl_mem_flags flags,
                const cl_image_format *image_format, size_t image_width,
                size_t image_height, size_t image_depth, size_t image_row_pitch,
                size_t image_slice_pitch, void *host_ptr, cl_int *errcode_ret) {
  return cl::CreateImage3D(context, flags, image_format, image_width,
                           image_height, image_depth, image_row_pitch,
                           image_slice_pitch, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(cl_context context, cl_bool normalized_coords,
                cl_addressing_mode addressing_mode, cl_filter_mode filter_mode,
                cl_int *errcode_ret) {
  return cl::CreateSampler(context, normalized_coords, addressing_mode,
                           filter_mode, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainSampler(cl_sampler sampler) {
  return cl::RetainSampler(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseSampler(cl_sampler sampler) {
  return cl::ReleaseSampler(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL clGetSamplerInfo(cl_sampler sampler,
                                                 cl_sampler_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret) {
  return cl::GetSamplerInfo(sampler, param_name, param_value_size, param_value,
                            param_value_size_ret);
}

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithSource(
    cl_context context, cl_uint count, const char **strings,
    const size_t *lengths, cl_int *errcode_ret) {
  return cl::CreateProgramWithSource(context, count, strings, lengths,
                                     errcode_ret);
}

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithBinary(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const size_t *lengths, const unsigned char **binaries,
    cl_int *binary_status, cl_int *errcode_ret) {
  return cl::CreateProgramWithBinary(context, num_devices, device_list, lengths,
                                     binaries, binary_status, errcode_ret);
}

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithBuiltInKernels(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *kernel_names, cl_int *errcode_ret) {
  return cl::CreateProgramWithBuiltInKernels(context, num_devices, device_list,
                                             kernel_names, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainProgram(cl_program program) {
  return cl::RetainProgram(program);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseProgram(cl_program program) {
  return cl::ReleaseProgram(program);
}

CL_API_ENTRY cl_int CL_API_CALL clCompileProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_headers,
    const cl_program *input_headers, const char **header_include_names,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  return cl::CompileProgram(program, num_devices, device_list, options,
                            num_input_headers, input_headers,
                            header_include_names, pfn_notify, user_data);
}

CL_API_ENTRY cl_program CL_API_CALL clLinkProgram(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_programs,
    const cl_program *input_programs,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data, cl_int *errcode_ret) {
  return cl::LinkProgram(context, num_devices, device_list, options,
                         num_input_programs, input_programs, pfn_notify,
                         user_data, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clBuildProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  return cl::BuildProgram(program, num_devices, device_list, options,
                          pfn_notify, user_data);
}

CL_API_ENTRY cl_int CL_API_CALL clGetProgramInfo(cl_program program,
                                                 cl_program_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret) {
  return cl::GetProgramInfo(program, param_name, param_value_size, param_value,
                            param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetProgramBuildInfo(
    cl_program program, cl_device_id device, cl_program_build_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  return cl::GetProgramBuildInfo(program, device, param_name, param_value_size,
                                 param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clUnloadCompiler() {
  return cl::UnloadCompiler();
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler(cl_platform_id platform) {
  return cl::UnloadPlatformCompiler(platform);
}

CL_API_ENTRY cl_kernel CL_API_CALL clCreateKernel(cl_program program,
                                                  const char *kernel_name,
                                                  cl_int *errcode_ret) {
  return cl::CreateKernel(program, kernel_name, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainKernel(cl_kernel kernel) {
  return cl::RetainKernel(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseKernel(cl_kernel kernel) {
  return cl::ReleaseKernel(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL clSetKernelArg(cl_kernel kernel,
                                               cl_uint arg_index,
                                               size_t arg_size,
                                               const void *arg_value) {
  return cl::SetKernelArg(kernel, arg_index, arg_size, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                         cl_kernel *kernels, cl_uint *num_kernels_ret) {
  return cl::CreateKernelsInProgram(program, num_kernels, kernels,
                                    num_kernels_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetKernelInfo(cl_kernel kernel,
                                                cl_kernel_info param_name,
                                                size_t param_value_size,
                                                void *param_value,
                                                size_t *param_value_size_ret) {
  return cl::GetKernelInfo(kernel, param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetKernelArgInfo(
    cl_kernel kernel, cl_uint arg_indx, cl_kernel_arg_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  return cl::GetKernelArgInfo(kernel, arg_indx, param_name, param_value_size,
                              param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetKernelWorkGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  return cl::GetKernelWorkGroupInfo(kernel, device, param_name,
                                    param_value_size, param_value,
                                    param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueNDRangeKernel(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t *global_work_offset, const size_t *global_work_size,
    const size_t *local_work_size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueNDRangeKernel(
      command_queue, kernel, work_dim, global_work_offset, global_work_size,
      local_work_size, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueTask(cl_command_queue command_queue,
                                              cl_kernel kernel,
                                              cl_uint num_events_in_wait_list,
                                              const cl_event *event_wait_list,
                                              cl_event *event) {
  return cl::EnqueueTask(command_queue, kernel, num_events_in_wait_list,
                         event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueNativeKernel(
    cl_command_queue command_queue, void(CL_CALLBACK *user_func)(void *),
    void *args, size_t cb_args, cl_uint num_mem_objects, const cl_mem *mem_list,
    const void **args_mem_loc, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueNativeKernel(
      command_queue, user_func, args, cb_args, num_mem_objects, mem_list,
      args_mem_loc, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_event CL_API_CALL clCreateUserEvent(cl_context context,
                                                    cl_int *errcode_ret) {
  return cl::CreateUserEvent(context, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clRetainEvent(cl_event event) {
  return cl::RetainEvent(event);
}

CL_API_ENTRY cl_int CL_API_CALL clReleaseEvent(cl_event event) {
  return cl::ReleaseEvent(event);
}

CL_API_ENTRY cl_int CL_API_CALL clWaitForEvents(cl_uint num_events,
                                                const cl_event *event_list) {
  return cl::WaitForEvents(num_events, event_list);
}

CL_API_ENTRY cl_int CL_API_CALL clGetEventInfo(cl_event event,
                                               cl_event_info param_name,
                                               size_t param_value_size,
                                               void *param_value,
                                               size_t *param_value_size_ret) {
  return cl::GetEventInfo(event, param_name, param_value_size, param_value,
                          param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clSetEventCallback(
    cl_event event, cl_int command_exec_callback_type,
    void(CL_CALLBACK *pfn_event_notify)(cl_event event,
                                        cl_int event_command_exec_status,
                                        void *user_data),
    void *user_data) {
  return cl::SetEventCallback(event, command_exec_callback_type,
                              pfn_event_notify, user_data);
}

CL_API_ENTRY cl_int CL_API_CALL clSetUserEventStatus(cl_event event,
                                                     cl_int execution_status) {
  return cl::SetUserEventStatus(event, execution_status);
}

CL_API_ENTRY cl_int CL_API_CALL clGetEventProfilingInfo(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  return cl::GetEventProfilingInfo(event, param_name, param_value_size,
                                   param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clSetCommandQueueProperty(
    cl_command_queue command_queue, cl_command_queue_properties properties,
    cl_bool enable, cl_command_queue_properties *old_properties) {
  return cl::SetCommandQueueProperty(command_queue, properties, enable,
                                     old_properties);
}
