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
#include <CL/cl_ext.h>
#include <CL/cl_ext_codeplay.h>
#include <extension/codeplay_kernel_exec_info.h>

extension::codeplay_kernel_exec_info::codeplay_kernel_exec_info()
    : extension("cl_codeplay_kernel_exec_info",
#ifdef OCL_EXTENSION_cl_codeplay_kernel_exec_info
                usage_category::PLATFORM
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

void *
extension::codeplay_kernel_exec_info::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) const {
  OCL_UNUSED(platform);

#ifndef OCL_EXTENSION_cl_codeplay_kernel_exec_info
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (func_name && 0 == strcmp("clSetKernelExecInfoCODEPLAY", func_name)) {
    return (void *)&clSetKernelExecInfoCODEPLAY;
  }
  return nullptr;
#endif
}

cl_int CL_API_CALL clSetKernelExecInfoCODEPLAY(
    cl_kernel kernel, cl_kernel_exec_info_codeplay param_name,
    size_t param_value_size, const void *param_value) {
  // Validate the inputs to the function and make sure they are not NULL.
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(!param_name, return CL_INVALID_VALUE);
  OCL_CHECK(!param_value_size, return CL_INVALID_VALUE);
  OCL_CHECK(!param_value, return CL_INVALID_VALUE);

#if (defined(CL_VERSION_3_0) || \
     defined(OCL_EXTENSION_cl_codeplay_kernel_exec_info))
  return extension::SetKernelExecInfo(kernel, param_name, param_value_size,
                                      param_value);
#else
  return CL_INVALID_OPERATION;
#endif
}
