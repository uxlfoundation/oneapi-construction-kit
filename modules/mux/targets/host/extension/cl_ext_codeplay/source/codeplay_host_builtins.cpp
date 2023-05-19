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

#include <cl/config.h>
#include <cl/device.h>
#include <extension/codeplay_host_builtins.h>

extension::codeplay_host_builtins::codeplay_host_builtins()
    : extension("cl_codeplay_host_builtins",
#ifdef OCL_EXTENSION_cl_codeplay_host_builtins
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_host_builtins::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // This extension is only valid on a ComputeAorta host CPU device.
  if ((0 != std::strncmp(CA_HOST_CL_DEVICE_NAME_PREFIX,
                         device->mux_device->info->device_name,
                         std::strlen(CA_HOST_CL_DEVICE_NAME_PREFIX))) ||
      (mux_device_type_cpu != device->mux_device->info->device_type)) {
    return CL_INVALID_DEVICE;
  }

  // The base class's GetDeviceInfo handles CL_DEVICE_EXTENSIONS and
  // CL_DEVICE_EXTENSIONS_WITH_VERSION queries.
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}
