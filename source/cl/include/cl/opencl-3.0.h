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
/// @brief API declarations introduced in OpenCL-3.0

#ifndef CL_OPENCL_3_0_H_INCLUDED
#define CL_OPENCL_3_0_H_INCLUDED
#include <CL/cl.h>
#include <cl/context.h>

namespace cl {
/// @addtogroup cl
/// @{

/// @brief SVM free callback function pointer definition.
///
/// @param[in] queue Command queue passed to callback from API.
/// @param[in] num_svm_pointers Number of elements in @p svm_pointers.
/// @param[in] svm_pointers list of SVM pointers to free.
/// @param[in] user_data Pointer to user supplied data.
using pfn_free_func_t = void(CL_CALLBACK*)(cl_command_queue queue,
                                           cl_uint num_svm_pointers,
                                           void* svm_pointers[],
                                           void* user_data);

/// @brief Program release callback function pointer definition.
///
/// @param[in] program Program object passed to callback.
/// @param[in] user_data Pointer to user supplied data.
using pfn_notify_t = void(CL_CALLBACK*)(cl_program program, void* user_data);

/// @brief Create an OpenCL command queue object.
///
/// @param[in] context Context the command queue belongs to.
/// @param[in] device Device the command queue will target.
/// @param[in] properties List of properties for the queue and their values.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return New command queue object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clCreateBuffer.html
CL_API_ENTRY cl_command_queue CL_API_CALL CreateCommandQueueWithProperties(
    cl_context context, cl_device_id device,
    const cl_queue_properties* properties, cl_int* errcode_ret);

/// @brief Create an OpenCL pipe memory object.
///
/// @param[in] context Context the pipe queue belongs to.
/// @param[in] flags Memory allocation flags.
/// @param[in] pipe_packet_size Size in bytes of a pipe packet.
/// @param[in] pipe_max_packets Maximum number of packets the pipe can hold.
/// @param[in] properties List of properties for the pipe and their values.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return New pipe memory object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clCreatePipe.html
CL_API_ENTRY cl_mem CL_API_CALL CreatePipe(cl_context context,
                                           cl_mem_flags flags,
                                           cl_uint pipe_packet_size,
                                           cl_uint pipe_max_packets,
                                           const cl_pipe_properties* properties,
                                           cl_int* errcode_ret);

/// @brief Query the pipe for information.
///
/// @param[in] pipe Pipe to query for information.
/// @param[in] param_name The information to query.
/// @param[in] param_value_size Size in bytes of @p param_value.
/// @param[out] param_value Pointer to value to return information in.
/// @param[out] param_value_size_ret Return size in bytes of storage required
/// for @p param_value.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clGetPipeInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetPipeInfo(cl_mem pipe,
                                            cl_pipe_info param_name,
                                            size_t param_value_size,
                                            void* param_value,
                                            size_t* param_value_size_ret);

/// @brief Allocate a shared virtual memory buffer.
///
/// @param[in] context Context the buffer belongs to.
/// @param[in] flags Memory allocation flags.
/// @param[in] size Size in bytes of the SVM buffer to be allocated.
/// @param[in] alignment Minimum alignment in bytes required for buffers memory
/// region.
///
/// @return SVM pointer to allocated buffer.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clSVMAlloc.html
CL_API_ENTRY void* CL_API_CALL SVMAlloc(cl_context context,
                                        cl_svm_mem_flags flags, size_t size,
                                        cl_uint alignment);

/// @brief Free a shared virtual memory buffer.
///
/// @param[in] context Context the buffer belongs to.
/// @param[in] svm_pointer Pointer to the SVM buffer to free.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clSVMFree.html
CL_API_ENTRY void CL_API_CALL SVMFree(cl_context context, void* svm_pointer);

/// @brief Create a sampler object.
///
/// @param[in] context Context the sampler belongs to.
/// @param[in] sampler_properties List of properties for the sampler and their
/// values.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return New sampler object.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clCreateSamplerWithProperties.html
CL_API_ENTRY cl_sampler CL_API_CALL CreateSamplerWithProperties(
    cl_context context, const cl_sampler_properties* sampler_properties,
    cl_int* errcode_ret);

/// @brief Set an SVM argument on the kernel.
///
/// @param[in] kernel Kernel to set the argument on.
/// @param[in] arg_index Index of the argument to set.
/// @param[in] arg_value Pointer to SVM buffer to use as argument.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clSetKernelArgSVMPointer.html
CL_API_ENTRY cl_int CL_API_CALL SetKernelArgSVMPointer(cl_kernel kernel,
                                                       cl_uint arg_index,
                                                       const void* arg_value);

/// @brief Pass additional information other than argument values to kernel.
///
/// @param[in] kernel Kernel being queried.
/// @param[in] param_name Information to be passed to the kernel.
/// @param[in] param_value_size size in bytes of memory pointed to by @p
/// param_value.
/// @param[in] param_value pointer to memory where values determined by @p
/// param_name are specified.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clSetKernelExecInfo.html
CL_API_ENTRY cl_int CL_API_CALL
SetKernelExecInfo(cl_kernel kernel, cl_kernel_exec_info param_name,
                  size_t param_value_size, const void* param_value);

/// @brief Enqueue command to free SVM buffer.
///
/// @param[in] command_queue Queue to enqueue command to.
/// @param[in] num_svm_pointers Number of svm buffers to free.
/// @param[in] svm_pointers List of pointers to svm buffers to free.
/// @param[in] pfn_free_func Callback to be called to free the SVM buffers.
/// @param[in] user_data Passed to @p pfn_free_func.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued SVMFree command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clEnqueueSVMFree.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueSVMFree(cl_command_queue command_queue, cl_uint num_svm_pointers,
               void* svm_pointers[], cl::pfn_free_func_t pfn_free_func,
               void* user_data, cl_uint num_events_in_wait_list,
               const cl_event* event_wait_list, cl_event* event);

/// @brief Enqueue a command to perform a memcpy.
///
/// @param[in] command_queue Queue to enqueue command to.
/// @param[in] blocking_copy Indicates if copy operation is blocking.
/// @param[in] dst_ptr Pointer to host or SVM memory destination.
/// @param[in] src_ptr Pointer to host or SVM memory source.
/// @param[in] size Size in bytes of data being copied.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued EnqueueSVMMemcpy command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clEnqueueSVMMemcpy.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueSVMMemcpy(
    cl_command_queue command_queue, cl_bool blocking_copy, void* dst_ptr,
    const void* src_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

/// @brief Enqueue a command to perform a memfill.
///
/// @param[in] command_queue Queue to enqueue command to.
/// @param[in] svm_ptr Pointer to memory region that will be filled with @p
/// pattern.
/// @param[in] pattern Used to fill region in buffer starting at @p svm_ptr of
/// size @p size bytes.
/// @param[in] pattern_size size of data pattern pointed to by @p pattern.
/// @param[in] size Size in bytes of region begin filled.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued EnqueueSVMMemFill command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clEnqueueSVMMemFill.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueSVMMemFill(
    cl_command_queue command_queue, void* svm_ptr, const void* pattern,
    size_t pattern_size, size_t size, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

/// @brief Enqueue a command to map an SVM buffer.
///
/// @param[in] command_queue Queue to enqueue command to.
/// @param[in] blocking_map indicates if map option is blocking.
/// @param[in] map_flags Bitfield of memory map flags.
/// @param[in] svm_ptr Pointer to memory region that will be updated by host.
/// @param[in] size Size of memory region starting at @p svm_ptr to be update by
/// host.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued EnqueueSVMMap command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clEnqueueSVMMap.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueSVMMap(cl_command_queue command_queue,
                                              cl_bool blocking_map,
                                              cl_map_flags map_flags,
                                              void* svm_ptr, size_t size,
                                              cl_uint num_events_in_wait_list,
                                              const cl_event* event_wait_list,
                                              cl_event* event);

/// @brief Enqueue a command to unmap an SVM buffer.
///
/// @param[in] command_queue Queue to enqueue command to.
/// @param[in] svm_ptr SVM buffer to unmap.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued EnqueueSVMMap command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/clEnqueueSVMUnmap.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueSVMUnmap(cl_command_queue command_queue,
                                                void* svm_ptr,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event* event_wait_list,
                                                cl_event* event);

/// @brief Replace default command queue on device.
///
/// @param[in] context Context used to create @p command_queue.
/// @param[in] device Device to replace default command queue on.
/// @param[in] command_queue Command queue to replace default command queue.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clSetDefaultDeviceCommandQueue.html
CL_API_ENTRY cl_int CL_API_CALL SetDefaultDeviceCommandQueue(
    cl_context context, cl_device_id device, cl_command_queue command_queue);

/// @brief Query device and host timestamps.
///
/// @param[in] device Device to query timestamp on.
/// @param[out] device_timestamp Updated with value of device timer in
/// nanoseconds.
/// @param[out] host_timestamp Updated with value of host timer in
/// nanoseconds.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clGetDeviceAndHostTimer.html
CL_API_ENTRY cl_int CL_API_CALL GetDeviceAndHostTimer(
    cl_device_id device, cl_ulong* device_timestamp, cl_ulong* host_timestamp);

/// @brief Query host clock.
///
/// @param[in] device Device to query timestamp on.
/// @param[out] host_timestamp Updated with value of host timer in
/// nanoseconds.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clGetHostTimer.html
CL_API_ENTRY cl_int CL_API_CALL GetHostTimer(cl_device_id device,
                                             cl_ulong* host_timestamp);

/// @brief Create program object with code in an IL.
///
/// @param[in] context Context the program will belong to.
/// @param[in] il Pointer to @p length byte block of IL program.
/// @param[in] length Length in bytes of IL program pointed to by il.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Program object consisting of the IL.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clCreateProgramWithIL.html
CL_API_ENTRY cl_program CL_API_CALL CreateProgramWithIL(cl_context context,
                                                        const void* il,
                                                        size_t length,
                                                        cl_int* errcode_ret);

/// @brief Clone a kernel object.
///
/// @param[in] source_kernel Kernel object that will be copied.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Clone of kernel.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clCloneKernel.html
CL_API_ENTRY cl_kernel CL_API_CALL CloneKernel(cl_kernel source_kernel,
                                               cl_int* errcode_ret);

/// @brief Query subgroup information about a kernel.
///
/// @param[in] kernel Kernel object being queried.
/// @param[in] device Identifies device in device list associated with @p
/// kernel.
/// @param[in] param_name Information to query.
/// @param[in] input_value_size Size in bytes of memory pointed to by
/// input_value.
/// @param[in] input_value Pointer to memory where parameterization of query is
/// passed from.
/// @param[in] param_value_size Size in bytes of memory pointed to by @p
/// param_value.
/// @param[out] param_value Pointer to memory where appropriate result being
/// queried is returned.
/// @param[out] param_value_size_ret Returns actual size in bytes of data
/// queried by param_name.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clGetKernelSubGroupInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetKernelSubGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_sub_group_info param_name,
    size_t input_value_size, const void* input_value, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret);

/// @brief Enqueue command to indicate which device a set of ranges of SVM
/// allocations should be associated with.
///
/// @param[in] command_queue Queue to add command to. The set of allocation
/// ranges will be migrated to the device associated with @p command_queue.
/// @param[in] num_svm_pointers Number of pointers in @p svm_pointers array and
/// number of elements in the @p sizes array.
/// @param[in] svm_pointers Pointer to an array of pointers to SVM buffers.
/// @param[in] sizes Array of sizes. Together svm_pointers[i] and sizes[i]
/// define the starting address and number of bytes in a range to be migrated.
/// @param[in] flags Bitfield used to specify migration options.
/// @param[in] num_events_in_wait_list Number of events in @p event_wait_list.
/// @param[in] event_wait_list List of events that are to be completed before
/// the enqueued EnqueueSVMMigrateMem command.
/// @param[out] event Event object identifying this particular command.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.1/docs/man/xhtml/clEnqueueSVMMigrateMem.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueSVMMigrateMem(
    cl_command_queue command_queue, cl_uint num_svm_pointers,
    const void** svm_pointers, const size_t* sizes,
    cl_mem_migration_flags flags, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

/// @brief Register user callback function with a program object.
///
/// @param[in] program Program to register callback to.
/// @param[in] pfn_notify Callback function registered by the application.
/// @param[in] user_data Pointer to user supplied data.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.2/docs/man/html/clSetProgramReleaseCallback.html
CL_API_ENTRY cl_int CL_API_CALL SetProgramReleaseCallback(
    cl_program program, pfn_notify_t pfn_notify, void* user_data);

/// @brief Set the value of a specialization constant.
///
/// @param[in] program Program created from an intermediate language.
/// @param[in] spec_id Idendify the specialization constant who's value will be
/// @param[in] spec_size Size in bytes of data pointed to by spec_value.
/// @param[in] spec_value Pointer to memory containing the value of the
/// specialization constant. set.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/2.2/docs/man/html/clSetProgramSpecializationConstant.html
CL_API_ENTRY cl_int CL_API_CALL
SetProgramSpecializationConstant(cl_program program, cl_uint spec_id,
                                 size_t spec_size, const void* spec_value);

/// @brief Create an OpenCL buffer memory object.
///
/// @param[in] context Context the buffer will belong to.
/// @param[in] properties List of properties for the buffer and their
/// corresponding values.
/// @param[in] flags Memory allocation flags.
/// @param[in] size Size in bytes of the requested device allocation.
/// @param[in] host_ptr Pointer to optionally provide user memory.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new buffer memory object.
///
/// @see
/// https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCreateBufferWithProperties
CL_API_ENTRY cl_mem CL_API_CALL CreateBufferWithProperties(
    cl_context context, const cl_mem_properties* properties, cl_mem_flags flags,
    size_t size, void* host_ptr, cl_int* errcode_ret);

/// @brief Create an OpenCL image memory object.
///
/// @param[in] context Context the image memory object belong to.
/// @param[in] properties List of properties for the image and their
/// corresponding values.
/// @param[in] flags Memory allocation flags.
/// @param[in] image_format Description of the image format.
/// @param[in] image_desc Description of the image.
/// @param[in] host_ptr User provided host pointer, may be null.
/// @param[out] errcode_ret Return error code if not null.
///
/// @return Return the new image memory object.
///
/// @see
/// https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCreateImageWithProperties
CL_API_ENTRY cl_mem CL_API_CALL CreateImageWithProperties(
    cl_context context, const cl_mem_properties* properties, cl_mem_flags flags,
    const cl_image_format* image_format, const cl_image_desc* image_desc,
    void* host_ptr, cl_int* errcode_ret);

/// @brief Set context destructor callback.
///
/// @param[in] context Context to set the destructor callback on.
/// @param[in] pfn_notify Destructor callback to set.
/// @param[in] user_data User data for the destructor callback.
///
/// @return Return an error code.
///
/// @see TODO when the spec is ratified and public.
CL_API_ENTRY cl_int CL_API_CALL SetContextDestructorCallback(
    cl_context context, cl::pfn_notify_context_destructor_t pfn_notify,
    void* user_data);

/// @}
}  // namespace cl

#endif
