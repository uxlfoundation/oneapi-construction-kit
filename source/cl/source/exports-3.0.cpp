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

#include <CL/cl.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/opencl-3.0.h>
#include <cl/platform.h>
#include <cl/program.h>
#include <cl/sampler.h>
#include <tracer/tracer.h>

CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueueWithProperties(
    cl_context context, cl_device_id device,
    const cl_queue_properties *properties, cl_int *errcode_ret) {
  return cl::CreateCommandQueueWithProperties(context, device, properties,
                                              errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreatePipe(cl_context context, cl_mem_flags flags, cl_uint pipe_packet_size,
             cl_uint pipe_max_packets, const cl_pipe_properties *properties,
             cl_int *errcode_ret) {
  return cl::CreatePipe(context, flags, pipe_packet_size, pipe_max_packets,
                        properties, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetPipeInfo(cl_mem pipe,
                                              cl_pipe_info param_name,
                                              size_t param_value_size,
                                              void *param_value,
                                              size_t *param_value_size_ret) {
  return cl::GetPipeInfo(pipe, param_name, param_value_size, param_value,
                         param_value_size_ret);
}

CL_API_ENTRY void *CL_API_CALL clSVMAlloc(cl_context context,
                                          cl_svm_mem_flags flags, size_t size,
                                          cl_uint alignment) {
  return cl::SVMAlloc(context, flags, size, alignment);
}

CL_API_ENTRY void CL_API_CALL clSVMFree(cl_context context, void *svm_pointer) {
  cl::SVMFree(context, svm_pointer);
}

CL_API_ENTRY cl_sampler CL_API_CALL clCreateSamplerWithProperties(
    cl_context context, const cl_sampler_properties *sampler_properties,
    cl_int *errcode_ret) {
  return cl::CreateSamplerWithProperties(context, sampler_properties,
                                         errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clSetKernelArgSVMPointer(
    cl_kernel kernel, cl_uint arg_index, const void *arg_value) {
  return cl::SetKernelArgSVMPointer(kernel, arg_index, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelExecInfo(cl_kernel kernel, cl_kernel_exec_info param_name,
                    size_t param_value_size, const void *param_value) {
  return cl::SetKernelExecInfo(kernel, param_name, param_value_size,
                               param_value);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMFree(
    cl_command_queue command_queue, cl_uint num_svm_pointers,
    void *svm_pointers[],
    void(CL_CALLBACK *pfn_free_func)(cl_command_queue queue,
                                     cl_uint num_svm_pointers,
                                     void *svm_pointers[], void *user_data),
    void *user_data, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMFree(command_queue, num_svm_pointers, svm_pointers,
                            pfn_free_func, user_data, num_events_in_wait_list,
                            event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMMemcpy(
    cl_command_queue command_queue, cl_bool blocking_copy, void *dst_ptr,
    const void *src_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMMemcpy(command_queue, blocking_copy, dst_ptr, src_ptr,
                              size, num_events_in_wait_list, event_wait_list,
                              event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMMemFill(
    cl_command_queue command_queue, void *svm_ptr, const void *pattern,
    size_t pattern_size, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMMemFill(command_queue, svm_ptr, pattern, pattern_size,
                               size, num_events_in_wait_list, event_wait_list,
                               event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMMap(
    cl_command_queue command_queue, cl_bool blocking_map, cl_map_flags flags,
    void *svm_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMMap(command_queue, blocking_map, flags, svm_ptr, size,
                           num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueSVMUnmap(cl_command_queue command_queue, void *svm_ptr,
                  cl_uint num_events_in_wait_list,
                  const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMUnmap(command_queue, svm_ptr, num_events_in_wait_list,
                             event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clSetDefaultDeviceCommandQueue(
    cl_context context, cl_device_id device, cl_command_queue command_queue) {
  return cl::SetDefaultDeviceCommandQueue(context, device, command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL clGetDeviceAndHostTimer(
    cl_device_id device, cl_ulong *device_timestamp, cl_ulong *host_timestamp) {
  return cl::GetDeviceAndHostTimer(device, device_timestamp, host_timestamp);
}

CL_API_ENTRY cl_int CL_API_CALL clGetHostTimer(cl_device_id device,
                                               cl_ulong *host_timestamp) {
  return cl::GetHostTimer(device, host_timestamp);
}

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithIL(cl_context context,
                                                          const void *il,
                                                          size_t length,
                                                          cl_int *errcode_ret) {
  return cl::CreateProgramWithIL(context, il, length, errcode_ret);
}

CL_API_ENTRY cl_kernel CL_API_CALL clCloneKernel(cl_kernel source_kernel,
                                                 cl_int *errcode_ret) {
  return cl::CloneKernel(source_kernel, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clGetKernelSubGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_sub_group_info param_name,
    size_t input_value_size, const void *input_value, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  return cl::GetKernelSubGroupInfo(kernel, device, param_name, input_value_size,
                                   input_value, param_value_size, param_value,
                                   param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMMigrateMem(
    cl_command_queue command_queue, cl_uint num_svm_pointers,
    const void **svm_pointers, const size_t *sizes,
    cl_mem_migration_flags flags, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  return cl::EnqueueSVMMigrateMem(command_queue, num_svm_pointers, svm_pointers,
                                  sizes, flags, num_events_in_wait_list,
                                  event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL clSetProgramReleaseCallback(
    cl_program program,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  return cl::SetProgramReleaseCallback(program, pfn_notify, user_data);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetProgramSpecializationConstant(cl_program program, cl_uint spec_id,
                                   size_t spec_size, const void *spec_value) {
  return cl::SetProgramSpecializationConstant(program, spec_id, spec_size,
                                              spec_value);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateBufferWithProperties(
    cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
    size_t size, void *host_ptr, cl_int *errcode_ret) {
  return cl::CreateBufferWithProperties(context, properties, flags, size,
                                        host_ptr, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateImageWithProperties(
    cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
    const cl_image_format *image_format, const cl_image_desc *image_desc,
    void *host_ptr, cl_int *errcode_ret) {
  return cl::CreateImageWithProperties(context, properties, flags, image_format,
                                       image_desc, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clSetContextDestructorCallback(
    cl_context context, cl::pfn_notify_context_destructor_t pfn_notify,
    void *user_data) {
  return cl::SetContextDestructorCallback(context, pfn_notify, user_data);
}
