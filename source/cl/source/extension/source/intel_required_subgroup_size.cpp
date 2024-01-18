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
#include <cl/device.h>
#include <cl/kernel.h>
#include <extension/intel_required_subgroup_size.h>

extension::intel_required_subgroup_size::intel_required_subgroup_size()
    : extension("cl_intel_required_subgroup_size",
#ifdef OCL_EXTENSION_cl_intel_required_subgroup_size
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}

cl_int extension::intel_required_subgroup_size::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // Only report extension if the extension wasn't disabled in cmake.
  if (usage_category::DISABLED == usage) {
    return CL_INVALID_VALUE;
  }
#if !defined(CL_VERSION_3_0)
  return CL_INVALID_VALUE;
#endif

  if (CL_DEVICE_SUB_GROUP_SIZES_INTEL == param_name) {
    // First check how many sub-group sizes the device reports.
    const uint64_t num_sizes = device->mux_device->info->num_sub_group_sizes;
    const size_t param_size = num_sizes * sizeof(size_t);
    OCL_CHECK(param_value && (param_value_size < param_size),
              return CL_INVALID_VALUE);
    if (param_value) {
      std::copy_n(device->mux_device->info->sub_group_sizes, num_sizes,
                  static_cast<size_t *>(param_value));
    }
    OCL_SET_IF_NOT_NULL(param_value_size_ret, param_size);
    return CL_SUCCESS;
  }

  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}

cl_int extension::intel_required_subgroup_size::GetKernelWorkGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name,
    size_t param_value_size, void *param_value,
    size_t *param_value_size_ret) const {
  // Only report extension if the extension wasn't disabled in cmake.
  if (usage_category::DISABLED == usage) {
    return CL_INVALID_VALUE;
  }
#if !defined(CL_VERSION_3_0)
  return CL_INVALID_VALUE;
#endif

  if (CL_KERNEL_SPILL_MEM_SIZE_INTEL == param_name) {
    OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_ulong));
    OCL_CHECK(param_value && param_value_size < sizeof(cl_ulong),
              return CL_INVALID_VALUE);

    OCL_ASSERT(kernel, "No kernel was provided");
    OCL_SET_IF_NOT_NULL((reinterpret_cast<cl_ulong *>(param_value)),
                        kernel->info->spill_mem_size_bytes);
    return CL_SUCCESS;
  }

  return extension::GetKernelWorkGroupInfo(kernel, device, param_name,
                                           param_value_size, param_value,
                                           param_value_size_ret);
}

#if defined(CL_VERSION_3_0)
cl_int extension::intel_required_subgroup_size::GetKernelSubGroupInfo(
    cl_kernel kernel, cl_device_id device, cl_kernel_sub_group_info param_name,
    size_t input_value_size, const void *input_value, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // Only report extension if the extension wasn't disabled in cmake.
  if (usage_category::DISABLED == usage) {
    return CL_INVALID_VALUE;
  }

  if (CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL == param_name) {
    OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
    OCL_CHECK(param_value && param_value_size < sizeof(size_t),
              return CL_INVALID_VALUE);

    OCL_ASSERT(kernel, "No kernel was provided");
    OCL_SET_IF_NOT_NULL((reinterpret_cast<size_t *>(param_value)),
                        kernel->info->reqd_sub_group_size.value_or(0));
    return CL_SUCCESS;
  }

  return extension::GetKernelSubGroupInfo(
      kernel, device, param_name, input_value_size, input_value,
      param_value_size, param_value, param_value_size_ret);
}
#endif
