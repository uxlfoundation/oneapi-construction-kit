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

#include "ur/kernel.h"

#include <memory>

#include "ur/context.h"
#include "ur/device.h"
#include "ur/platform.h"
#include "ur/program.h"

ur_kernel_handle_t_::~ur_kernel_handle_t_() {
  for (const auto &device_kernel_pair : device_kernel_map) {
    const auto mux_device = device_kernel_pair.first->mux_device;
    const auto mux_kernel = device_kernel_pair.second;
    muxDestroyKernel(mux_device, mux_kernel,
                     program->context->platform->mux_allocator_info);
  }

  for (const auto &argument : arguments) {
    if (argument.value.data) {
      delete[] argument.value.data;
    }
  }
}

cargo::expected<ur_kernel_handle_t, ur_result_t> ur_kernel_handle_t_::create(
    ur_program_handle_t program, cargo::string_view kernel_name) {
  auto kernel = std::make_unique<ur_kernel_handle_t_>(program, kernel_name);
  if (!kernel) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }

  for (const auto &device_program_pair : program->device_program_map) {
    const auto device = device_program_pair.first;
    const auto mux_executable = device_program_pair.second.mux_executable;

    mux_kernel_t mux_kernel = nullptr;
    if (muxCreateKernel(device->mux_device, mux_executable, kernel_name.data(),
                        kernel_name.size(),
                        program->context->platform->mux_allocator_info,
                        &mux_kernel)) {
      // TODO: It's currently unclear if it is valid to compile here and there
      // is no appropriate error code if you do and it fails.
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }

    kernel->device_kernel_map[device] = mux_kernel;
  }

  // Create vector for arranging values
  const auto kernel_data = kernel->program->getKernelData(kernel_name);
  if (!kernel_data) {
    return cargo::make_unexpected(kernel_data.error());
  }
  if (cargo::success !=
      kernel->arguments.alloc(kernel_data.value().getNumArguments())) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }

  return kernel.release();
}

UR_APIEXPORT ur_result_t UR_APICALL
urKernelCreate(ur_program_handle_t hProgram, const char *pKernelName,
               ur_kernel_handle_t *phKernel) {
  if (!hProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (!pKernelName || !phKernel) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  // TODO: Implement.
  auto kernel = ur_kernel_handle_t_::create(hProgram, pKernelName);
  if (!kernel) {
    return kernel.error();
  }

  *phKernel = *kernel;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urKernelRetain(ur_kernel_handle_t hKernel) {
  if (!hKernel) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hKernel);
}

UR_APIEXPORT ur_result_t UR_APICALL
urKernelRelease(ur_kernel_handle_t hKernel) {
  if (!hKernel) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hKernel);
}

UR_APIEXPORT ur_result_t UR_APICALL urKernelSetArgMemObj(
    ur_kernel_handle_t hKernel, uint32_t argIndex, ur_mem_handle_t hArgValue) {
  if (!hKernel) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (hKernel->program->context->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }

  if (argIndex >= hKernel->arguments.size()) {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  hKernel->arguments[argIndex].mem_handle = hArgValue;

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urKernelSetArgValue(ur_kernel_handle_t hKernel, uint32_t argIndex,
                    size_t argSize, const void *pArgValue) {
  if (!hKernel) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!pArgValue) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  if (hKernel->program->context->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }

  uint64_t num_devices;
  const auto device_info_result = ur::resultFromMux(
      muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &num_devices));
  if (device_info_result != UR_RESULT_SUCCESS) {
    return device_info_result;
  }

  if (argIndex >= hKernel->arguments.size()) {
    return UR_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX;
  }

  const auto kernel_data =
      hKernel->program->getKernelData(hKernel->kernel_name);

  if (!kernel_data) {
    return kernel_data.error();
  }

  const auto argument_kind = kernel_data.value().argument_types[argIndex].kind;

  switch (argument_kind) {
#define CASE_VALUE_TYPE(arg_type, type)                                       \
  case arg_type: {                                                            \
    if (sizeof(type) != argSize) {                                            \
      return UR_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE;                    \
    }                                                                         \
    hKernel->arguments[argIndex].value.size = argSize;                        \
    hKernel->arguments[argIndex].value.data = new char[argSize];              \
    std::memcpy(hKernel->arguments[argIndex].value.data, pArgValue, argSize); \
  } break

#define CASES_VALUE_VECTOR_TYPE(arg_type, type) \
  CASE_VALUE_TYPE(arg_type, type);              \
  CASE_VALUE_TYPE(arg_type##_2, type[2]);       \
  CASE_VALUE_TYPE(arg_type##_3, type[4]);       \
  CASE_VALUE_TYPE(arg_type##_4, type[4]);       \
  CASE_VALUE_TYPE(arg_type##_8, type[8]);       \
  CASE_VALUE_TYPE(arg_type##_16, type[16]);

    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT8, char);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT16, short);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT32, int);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::INT64, long);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::HALF, uint16_t);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::FLOAT, float);
    CASES_VALUE_VECTOR_TYPE(compiler::ArgumentKind::DOUBLE, double);

#undef CASE_VALUE
#undef CASES_VALUE_VECTOR_TYPE

    default:
      return UR_RESULT_ERROR_INVALID_KERNEL;
  }

  return UR_RESULT_SUCCESS;
}
