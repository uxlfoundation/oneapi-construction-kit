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

#include <CL/cl_ext.h>
#include <cl/config.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/program.h>
#include <extension/khr_il_program.h>

#include <cstring>

extension::khr_il_program::khr_il_program()
    : extension("cl_khr_il_program",
#ifdef OCL_EXTENSION_cl_khr_il_program
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::khr_il_program::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
#if !defined(CL_VERSION_3_0)
  // This device property is unreachable in OpenCL 3.0 mode because the
  // CL_DEVICE_IL_VERSION enum has the same value, and is always processed
  // first.  So we never reach this case for 3.0, so it is not included.
  if (param_name == CL_DEVICE_IL_VERSION_KHR) {
    // Set to a space separated list of IL version strings of the form
    // <IL_Prefix>_<Major_version>.<Minor_version> "SPIR-V" is a required IL
    // prefix when the cl_khr_il_program extension is reported.
    static const cargo::string_view il_version = "SPIR-V_1.0";
    if (nullptr != param_value) {
      OCL_CHECK(param_value_size < il_version.size() + 1,
                return CL_INVALID_VALUE);
      std::strncpy(static_cast<char *>(param_value), il_version.data(),
                   il_version.size());
    }
    OCL_SET_IF_NOT_NULL(param_value_size_ret, il_version.size() + 1);
    return CL_SUCCESS;
  }
#endif

  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}

void *extension::khr_il_program::GetExtensionFunctionAddressForPlatform(
    cl_platform_id, const char *func_name) const {
#ifndef OCL_EXTENSION_cl_khr_il_program
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (0 == std::strcmp("clCreateProgramWithILKHR", func_name)) {
    return reinterpret_cast<void *>(&clCreateProgramWithILKHR);
  }
  return nullptr;
#endif
}

cl_int extension::khr_il_program::GetProgramInfo(
    cl_program program, cl_program_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  switch (param_name) {
    case CL_PROGRAM_IL_KHR: {
      if (program->type == cl::program_type::SPIRV) {
        // Returns the program IL for programs created with
        // clCreateProgramWithILKHR.
        const size_t size = program->spirv.code.size() * sizeof(uint32_t);
        if (nullptr != param_value) {
          OCL_CHECK(param_value_size < size, return CL_INVALID_VALUE);
          std::copy(program->spirv.code.begin(), program->spirv.code.end(),
                    static_cast<uint32_t *>(param_value));
        }
        OCL_SET_IF_NOT_NULL(param_value_size_ret, size);
      } else {
        // If program is created with clCreateProgramWithSource,
        // clCreateProgramWithBinary, or clCreateProgramWithBuiltinKernels, the
        // memory pointed to by param_value will be unchanged and
        // param_value_size_ret will be set to zero.
        OCL_SET_IF_NOT_NULL(param_value_size_ret, 0);
      }
    } break;
    default:
      return extension::GetProgramInfo(program, param_name, param_value_size,
                                       param_value, param_value_size_ret);
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithILKHR(
    cl_context context, const void *il, size_t length, cl_int *errcode_ret) {
  OCL_CHECK(nullptr == context,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(nullptr == context->getCompilerContext(),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
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
