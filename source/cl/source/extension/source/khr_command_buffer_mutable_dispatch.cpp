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

#include <cl/device.h>
#include <extension/khr_command_buffer.h>
#include <extension/khr_command_buffer_mutable_dispatch.h>
#include <tracer/tracer.h>

extension::khr_command_buffer_mutable_dispatch::
    khr_command_buffer_mutable_dispatch()
    : extension("cl_khr_command_buffer_mutable_dispatch",
#ifdef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

void *extension::khr_command_buffer_mutable_dispatch::
    GetExtensionFunctionAddressForPlatform(cl_platform_id,
                                           const char *func_name) const {
#ifndef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (0 == std::strcmp("clUpdateMutableCommandsKHR", func_name)) {
    return reinterpret_cast<void *>(&clUpdateMutableCommandsKHR);
  } else if (0 == std::strcmp("clGetMutableCommandInfoKHR", func_name)) {
    return reinterpret_cast<void *>(&clGetMutableCommandInfoKHR);
  }
  return nullptr;
#endif
}

cl_int extension::khr_command_buffer_mutable_dispatch::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
#ifndef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
#else
  // Device needs to be able to update kernel descriptors to support this
  // extension
  const auto mux_device_info = device->mux_device->info;
  if (!mux_device_info->descriptors_updatable) {
    return CL_INVALID_DEVICE;
  }

  cl_ndrange_kernel_command_properties_khr result = 0;
  if (CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR == param_name) {
    result = CL_MUTABLE_DISPATCH_ARGUMENTS_KHR;
  } else {
    // Use default implementation that uses the name set in the constructor
    // as the name usage specifies.
    return extension::GetDeviceInfo(device, param_name, param_value_size,
                                    param_value, param_value_size_ret);
  }

  constexpr size_t type_size = sizeof(result);
  if (nullptr != param_value) {
    OCL_CHECK(param_value_size < type_size, return CL_INVALID_VALUE);
    *static_cast<cl_ndrange_kernel_command_properties_khr *>(param_value) =
        result;
  }
  OCL_SET_IF_NOT_NULL(param_value_size_ret, type_size);
  return CL_SUCCESS;
#endif
}

#ifdef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
CL_API_ENTRY cl_int CL_API_CALL
clUpdateMutableCommandsKHR(cl_command_buffer_khr command_buffer,
                           const cl_mutable_base_config_khr *mutable_config) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clUpdateMutableCommandsKHR");
  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(!command_buffer->is_finalized, return CL_INVALID_OPERATION);

  // Command-buffer property needs to be set to support re-enqueue while
  // a previous instance is in flight
  if (!command_buffer->supportsSimultaneousUse() &&
      command_buffer->execution_refcount > 0) {
    return CL_INVALID_OPERATION;
  }

  // CL_COMMAND_BUFFER_MUTABLE_KHR must have been set on command-buffer
  // creation to use this update entry-point.
  if (!command_buffer->isMutable()) {
    return CL_INVALID_OPERATION;
  }

  OCL_CHECK(!mutable_config, return CL_INVALID_VALUE);
  OCL_CHECK(mutable_config->type != CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR,
            return CL_INVALID_VALUE);

  // Values for next would be defined by implementation of mutable mem commands
  // layered extension. Later checks assume next is NULL and so the
  // mutable_dispatch_list field must be set.
  OCL_CHECK(mutable_config->next, return CL_INVALID_VALUE);

  OCL_CHECK(!mutable_config->mutable_dispatch_list ||
                !mutable_config->num_mutable_dispatch,
            return CL_INVALID_VALUE);

  return command_buffer->updateCommandBuffer(*mutable_config);
}

CL_API_ENTRY cl_int CL_API_CALL clGetMutableCommandInfoKHR(
    cl_mutable_command_khr command, cl_mutable_command_info_khr param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetMutableCommandInfoKHR");

  OCL_CHECK(!command, return CL_INVALID_MUTABLE_COMMAND_KHR);

#define MUTABLE_COMMAND_INFO_CASE(TYPE, SIZE_RET, POINTER_TYPE, VALUE)  \
  case TYPE: {                                                          \
    OCL_CHECK(param_value && (param_value_size < SIZE_RET),             \
              return CL_INVALID_VALUE);                                 \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);                \
    OCL_SET_IF_NOT_NULL(static_cast<POINTER_TYPE>(param_value), VALUE); \
  } break

#define MUTABLE_COMMAND_ARRAY_INFO_CASE(TYPE, VALUE)        \
  case TYPE: {                                              \
    const size_t size = sizeof(size_t) * command->work_dim; \
    OCL_CHECK(param_value && (param_value_size < size),     \
              return CL_INVALID_VALUE);                     \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, size);        \
    if (param_value) {                                      \
      std::memcpy(param_value, VALUE.data(), size);         \
    }                                                       \
  } break

  switch (param_name) {
    MUTABLE_COMMAND_INFO_CASE(CL_MUTABLE_COMMAND_COMMAND_QUEUE_KHR,
                              sizeof(cl_command_queue), cl_command_queue *,
                              command->command_buffer->command_queue);

    MUTABLE_COMMAND_INFO_CASE(CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR,
                              sizeof(cl_command_buffer_khr),
                              cl_command_buffer_khr *, command->command_buffer);

    MUTABLE_COMMAND_INFO_CASE(CL_MUTABLE_COMMAND_COMMAND_TYPE_KHR,
                              sizeof(cl_int), cl_int *,
                              CL_COMMAND_NDRANGE_KERNEL);

    MUTABLE_COMMAND_INFO_CASE(CL_MUTABLE_DISPATCH_KERNEL_KHR, sizeof(cl_kernel),
                              cl_kernel *, command->kernel);

    MUTABLE_COMMAND_INFO_CASE(CL_MUTABLE_DISPATCH_DIMENSIONS_KHR,
                              sizeof(cl_uint), cl_uint *, command->work_dim);

    MUTABLE_COMMAND_ARRAY_INFO_CASE(CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR,
                                    command->work_offset);

    MUTABLE_COMMAND_ARRAY_INFO_CASE(CL_MUTABLE_DISPATCH_GLOBAL_WORK_SIZE_KHR,
                                    command->global_size);

    MUTABLE_COMMAND_ARRAY_INFO_CASE(CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR,
                                    command->local_size);

    case CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR: {
      const auto &properties_list = command->properties_list;
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          sizeof(cl_ndrange_kernel_command_properties_khr) *
                              properties_list.size());
      OCL_CHECK(
          param_value && param_value_size <
                             sizeof(cl_ndrange_kernel_command_properties_khr) *
                                 properties_list.size(),
          return CL_INVALID_VALUE);
      if (param_value) {
        std::copy(std::begin(properties_list), std::end(properties_list),
                  static_cast<cl_bitfield *>(param_value));
      }
    } break;
    default:
      return CL_INVALID_VALUE;
  }
#undef MUTABLE_COMMAND_INFO_CASE
#undef MUTABLE_COMMAND_ARRAY_INFO_CASE

  return CL_SUCCESS;
}
#endif
