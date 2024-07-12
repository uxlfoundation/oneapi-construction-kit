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
#include <cargo/dynamic_array.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/kernel.h>
#include <cl/macros.h>
#include <cl/mux.h>
#include <cl/opencl-3.0.h>
#include <cl/platform.h>
#include <cl/program.h>
#include <cl/validate.h>
#include <tracer/tracer.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "cargo/array_view.h"

// OpenCL-2.0 APIs
CL_API_ENTRY cl_command_queue CL_API_CALL cl::CreateCommandQueueWithProperties(
    cl_context context, cl_device_id device,
    const cl_queue_properties *properties, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clCreateCommandQueueWithProperties");

  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device || !context->hasDevice(device),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
            return nullptr);

  auto command_queue = _cl_command_queue::create(context, device, properties);
  if (!command_queue) {
    OCL_SET_IF_NOT_NULL(errcode_ret, command_queue.error());
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return command_queue->release();
}

CL_API_ENTRY cl_mem CL_API_CALL
cl::CreatePipe(cl_context context, cl_mem_flags flags, cl_uint pipe_packet_size,
               cl_uint pipe_max_packets, const cl_pipe_properties *properties,
               cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreatePipe");
  // Optional in 3.0.
  (void)context;
  (void)flags;
  (void)pipe_packet_size;
  (void)pipe_max_packets;
  (void)properties;
  *errcode_ret = CL_INVALID_OPERATION;
  return nullptr;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetPipeInfo(cl_mem pipe,
                                                cl_pipe_info param_name,
                                                size_t param_value_size,
                                                void *param_value,
                                                size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetPipeInfo");
  // Optional in 3.0.
  (void)pipe;
  (void)param_name;
  (void)param_value_size;
  (void)param_value;
  (void)param_value_size_ret;
  return CL_INVALID_MEM_OBJECT;
}

CL_API_ENTRY void *CL_API_CALL cl::SVMAlloc(cl_context context,
                                            cl_svm_mem_flags flags, size_t size,
                                            cl_uint alignment) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSVMAlloc");
  // Optional in 3.0.
  (void)context;
  (void)flags;
  (void)size;
  (void)alignment;
  return nullptr;
}

CL_API_ENTRY void CL_API_CALL cl::SVMFree(cl_context context,
                                          void *svm_pointer) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSVMFree");
  // Optional in 3.0.
  (void)context;
  (void)svm_pointer;
  return;
}

CL_API_ENTRY cl_sampler CL_API_CALL cl::CreateSamplerWithProperties(
    cl_context context, const cl_sampler_properties *sampler_properties,
    cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clCreateSamplerWithProperties");
  // TODO: Implement, see CA-2613.
  (void)context;
  (void)sampler_properties;
  *errcode_ret = CL_INVALID_OPERATION;
  return nullptr;
}

CL_API_ENTRY cl_int CL_API_CALL cl::SetKernelArgSVMPointer(
    cl_kernel kernel, cl_uint arg_index, const void *arg_value) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetKernelArgSVMPointer");
  // Optional in 3.0.
  (void)kernel;
  (void)arg_index;
  (void)arg_value;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::SetKernelExecInfo(cl_kernel kernel, cl_kernel_exec_info param_name,
                      size_t param_value_size, const void *param_value) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetKernelExecInfo");

  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  const cl_int err = extension::SetKernelExecInfo(
      kernel, param_name, param_value_size, param_value);

  // CL_INVALID_KERNEL is returned if the extension failed to set the argument,
  // if the kernel really was invalid a check above already caught it.
  if (CL_INVALID_KERNEL != err) {
    return err;
  }

  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueSVMFree(
    cl_command_queue command_queue, cl_uint num_svm_pointers,
    void *svm_pointers[],
    void(CL_CALLBACK *pfn_free_func)(cl_command_queue queue,
                                     cl_uint num_svm_pointers,
                                     void *svm_pointers[], void *user_data),
    void *user_data, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMFree");
  // Optional in 3.0.
  (void)command_queue;
  (void)num_svm_pointers;
  (void)svm_pointers;
  (void)pfn_free_func;
  (void)user_data;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueSVMMemcpy(
    cl_command_queue command_queue, cl_bool blocking_copy, void *dst_ptr,
    const void *src_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMMemcpy");
  // Optional in 3.0.
  (void)command_queue;
  (void)blocking_copy;
  (void)dst_ptr;
  (void)src_ptr;
  (void)size;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueSVMMemFill(
    cl_command_queue command_queue, void *svm_ptr, const void *pattern,
    size_t pattern_size, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMMemFill");
  // Optional in 3.0.
  (void)command_queue;
  (void)svm_ptr;
  (void)pattern;
  (void)pattern_size;
  (void)size;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueSVMMap(
    cl_command_queue command_queue, cl_bool blocking_map, cl_map_flags flags,
    void *svm_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMMap");
  // Optional in 3.0.
  (void)command_queue;
  (void)blocking_map;
  (void)flags;
  (void)svm_ptr;
  (void)size;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueSVMUnmap(cl_command_queue command_queue, void *svm_ptr,
                    cl_uint num_events_in_wait_list,
                    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMUnmap");
  // Optional in 3.0.
  (void)command_queue;
  (void)svm_ptr;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

// OpenCL-2.1 APIs
CL_API_ENTRY cl_int CL_API_CALL cl::SetDefaultDeviceCommandQueue(
    cl_context context, cl_device_id device, cl_command_queue command_queue) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clSetDefaultDeviceCommandQueue");
  // Optional in 3.0.
  (void)context;
  (void)device;
  (void)command_queue;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetDeviceAndHostTimer(
    cl_device_id device, cl_ulong *device_timestamp, cl_ulong *host_timestamp) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetDeviceAndHostTimer");
  // Optional in 3.0.
  (void)device;
  (void)device_timestamp;
  (void)host_timestamp;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetHostTimer(cl_device_id device,
                                                 cl_ulong *host_timestamp) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetHostTimer");
  // Optional in 3.0.
  (void)device;
  (void)host_timestamp;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_program CL_API_CALL cl::CreateProgramWithIL(
    cl_context context, const void *il, size_t length, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateProgramWithIL");
  OCL_CHECK(nullptr == context,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(nullptr == context->getCompilerContext(),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
            return nullptr);
  OCL_CHECK(nullptr == il || 0 == length,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!context->getCompilerContext()->isValidSPIRV(
                {static_cast<const uint32_t *>(il), length / sizeof(uint32_t)}),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  auto program = _cl_program::create(context, il, length);
  if (!program) {
    OCL_SET_IF_NOT_NULL(errcode_ret, program.error());
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return program->release();
}

CL_API_ENTRY cl_kernel CL_API_CALL cl::CloneKernel(cl_kernel source_kernel,
                                                   cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCloneKernel");
  if (!source_kernel) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_KERNEL);
    return nullptr;
  }
  auto kernel = source_kernel->clone();
  if (!kernel) {
    OCL_SET_IF_NOT_NULL(errcode_ret, kernel.error());
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return *kernel;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetKernelSubGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_sub_group_info param_name,
    size_t input_value_size, const void *input_value, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetKernelSubGroupInfo");

  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  // If the list of devices associated with kernel is a single device, device
  // can be a NULL value.
  if (!device) {
    OCL_CHECK(kernel->device_kernel_map.size() > 1, return CL_INVALID_DEVICE);
    device = kernel->device_kernel_map.begin()->first;
  } else {
    OCL_CHECK(std::end(kernel->device_kernel_map) ==
                  kernel->device_kernel_map.find(device),
              return CL_INVALID_DEVICE);
  }
  OCL_CHECK(0 == device->max_num_sub_groups, return CL_INVALID_OPERATION);

  switch (param_name) {
    case CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE: {
      OCL_CHECK(param_value && (param_value_size < sizeof(size_t)),
                return CL_INVALID_VALUE);
      OCL_CHECK(input_value_size == 0 || input_value_size % sizeof(size_t),
                return CL_INVALID_VALUE);
      OCL_CHECK(!input_value, return CL_INVALID_VALUE);
      if (param_value) {
        // The OpenCL spec for this query says:
        //
        // The input_value must be an array of size_t values corresponding to
        // the local work size parameter of the intended dispatch. The number of
        // dimensions in the ND-range will be inferred from the value specified
        // for input_value_size.
        //
        // We start by setting all dimensions to 1, then copy over the input
        // local size. That way we always have a valid local size.
        size_t local_size[]{1, 1, 1};
        std::memcpy(local_size, input_value, input_value_size);
        const auto expected_sub_group_size =
            kernel->device_kernel_map[device]->getSubGroupSizeForLocalSize(
                local_size[0], local_size[1], local_size[2]);
        if (!expected_sub_group_size) {
          return expected_sub_group_size.error();
        }
        *static_cast<size_t *>(param_value) = *expected_sub_group_size;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
    } break;
    case CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE: {
      OCL_CHECK(param_value && (param_value_size < sizeof(size_t)),
                return CL_INVALID_VALUE);
      OCL_CHECK(input_value_size == 0 || input_value_size % sizeof(size_t),
                return CL_INVALID_VALUE);
      OCL_CHECK(!input_value, return CL_INVALID_VALUE);
      if (param_value) {
        // The OpenCL spec for this query says:
        //
        // The input_value must be an array of size_t values corresponding to
        // the local work size parameter of the intended dispatch. The number of
        // dimensions in the ND-range will be inferred from the value specified
        // for input_value_size.
        //
        // We start by setting all dimensions to 1, then copy over the input
        // local size. That way we always have a valid local size.
        size_t local_size[]{1, 1, 1};
        std::memcpy(local_size, input_value, input_value_size);
        const auto expected_sub_group_count =
            kernel->device_kernel_map[device]->getSubGroupCountForLocalSize(
                local_size[0], local_size[1], local_size[2]);
        if (!expected_sub_group_count) {
          return expected_sub_group_count.error();
        }
        *static_cast<size_t *>(param_value) = *expected_sub_group_count;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
    } break;
    case CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT: {
      OCL_CHECK(param_value && (param_value_size % sizeof(size_t) ||
                                param_value_size == 0),
                return CL_INVALID_VALUE);
      OCL_CHECK(input_value_size != sizeof(size_t), return CL_INVALID_VALUE);
      OCL_CHECK(!input_value, return CL_INVALID_VALUE);
      if (param_value) {
        auto *out_local_size = static_cast<size_t *>(param_value);
        const auto sub_group_count = *static_cast<const size_t *>(input_value);
        auto expected_local_sizes =
            kernel->device_kernel_map[device]->getLocalSizeForSubGroupCount(
                sub_group_count);
        if (!expected_local_sizes) {
          return expected_local_sizes.error();
        }
        std::memcpy(out_local_size, expected_local_sizes->data(),
                    param_value_size);
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, param_value_size
                                                    ? param_value_size
                                                    : 3 * sizeof(size_t));
    } break;
    case CL_KERNEL_MAX_NUM_SUB_GROUPS: {
      OCL_CHECK(param_value && (param_value_size < sizeof(size_t)),
                return CL_INVALID_VALUE);
      if (param_value) {
        const auto expected_max_num_sub_groups =
            kernel->device_kernel_map[device]->getMaxNumSubGroups();
        if (!expected_max_num_sub_groups) {
          return expected_max_num_sub_groups.error();
        }
        *static_cast<size_t *>(param_value) = *expected_max_num_sub_groups;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
    } break;
    case CL_KERNEL_COMPILE_NUM_SUB_GROUPS: {
      OCL_CHECK(param_value && (param_value_size < sizeof(size_t)),
                return CL_INVALID_VALUE);
      if (param_value) {
        // TODO: Actually query this from Mux and don't just return 0 (see
        // CA-3948).
        *static_cast<size_t *>(param_value) = 0;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
    } break;
    default: {
      return extension::GetKernelSubGroupInfo(
          kernel, device, param_name, input_value_size, input_value,
          param_value_size, param_value, param_value_size_ret);
    }
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueSVMMigrateMem(
    cl_command_queue command_queue, cl_uint num_svm_pointers,
    const void **svm_pointers, const size_t *sizes,
    cl_mem_migration_flags flags, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueSVMMigrateMem");
  // Optional in 3.0.
  (void)command_queue;
  (void)num_svm_pointers;
  (void)svm_pointers;
  (void)sizes;
  (void)flags;
  (void)num_events_in_wait_list;
  (void)event_wait_list;
  (void)event;
  return CL_INVALID_OPERATION;
}

// OpenCL-2.0 APIs
CL_API_ENTRY cl_int CL_API_CALL cl::SetProgramReleaseCallback(
    cl_program program,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clSetProgramReleaseCallback");
  // Optional in 3.0.
  (void)program;
  (void)pfn_notify;
  (void)user_data;
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::SetProgramSpecializationConstant(cl_program program, cl_uint spec_id,
                                     size_t spec_size, const void *spec_value) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clSetProgramSpecializationConstant");

  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  // SPIR-V is optional in 3.0 so if we have no compiler we just disable
  // it. Note that if supporting the with SPIR-V but without compiler
  // configuration we would need to return CL_COMPILER_NOT_AVAILABLE.
  OCL_CHECK(nullptr == program->context->getCompilerContext(),
            return CL_INVALID_OPERATION);

  OCL_CHECK(program->type != cl::program_type::SPIRV,
            return CL_INVALID_PROGRAM);

  if (auto error =
          program->spirv.setSpecConstant(spec_id, spec_size, spec_value)) {
    return error;
  }
  return CL_SUCCESS;
}

// OpenCL-3.0 APIs
CL_API_ENTRY cl_mem CL_API_CALL cl::CreateBufferWithProperties(
    cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
    size_t size, void *host_ptr, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clCreateBufferWithProperties");

  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  const cl_int error = cl::validate::MemFlags(flags, host_ptr);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  OCL_CHECK(0 == size, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_BUFFER_SIZE);
            return nullptr);

  auto buffer = _cl_mem_buffer::create(context, flags, size, host_ptr);
  if (!buffer) {
    OCL_SET_IF_NOT_NULL(errcode_ret, buffer.error());
    return nullptr;
  }

  // The properties parameter is allowed to be a `nullptr`, if so, that just
  // means we have no properties to apply with and can proceed as normal.
  if (properties) {
    auto current = properties;
    for (; current[0]; current += 2) {
      // We currently don't have any valid properties to check for. When we do,
      // we can check them here.
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROPERTY);
      return nullptr;
    }
    // Store the buffer properties and their values.
    auto end = current + 1;
    if (buffer->get()->properties.alloc(std::distance(properties, end))) {
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
      return nullptr;
    }
    std::copy(properties, end, buffer->get()->properties.data());
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return buffer->release();
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateImageWithProperties(
    cl_context context, const cl_mem_properties *properties, cl_mem_flags flags,
    const cl_image_format *image_format, const cl_image_desc *image_desc,
    void *host_ptr, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateImageWithProperties");
  // TODO: Implement, see CA-2610.
  (void)context;
  (void)properties;
  (void)flags;
  (void)image_format;
  (void)image_desc;
  (void)host_ptr;
  *errcode_ret = CL_INVALID_OPERATION;
  return nullptr;
}

CL_API_ENTRY cl_int CL_API_CALL cl::SetContextDestructorCallback(
    cl_context context, cl::pfn_notify_context_destructor_t pfn_notify,
    void *user_data) {
  if (!context) {
    return CL_INVALID_CONTEXT;
  }
  if (!pfn_notify) {
    return CL_INVALID_VALUE;
  }
  const std::lock_guard<std::mutex> lock(context->mutex);
  return context->pushDestructorCallback(pfn_notify, user_data);
}
