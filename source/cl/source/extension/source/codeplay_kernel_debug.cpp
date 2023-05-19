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
#include <extension/codeplay_kernel_debug.h>

extension::codeplay_kernel_debug::codeplay_kernel_debug()
    : extension("cl_codeplay_kernel_debug",
#ifdef OCL_EXTENSION_cl_codeplay_kernel_debug
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_kernel_debug::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // Intercept device queries so that CL_DEVICE_EXTENSIONS will not contain
  // cl_codeplay_kernel_debug if the device does not support it.
  if (!device->compiler_available || !device->compiler_info->kernel_debug) {
    return CL_INVALID_VALUE;
  }
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
}
