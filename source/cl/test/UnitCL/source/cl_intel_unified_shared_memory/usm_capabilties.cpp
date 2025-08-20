// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Common.h>

#include "cl_intel_unified_shared_memory.h"

// Tests additional clGetDeviceInfo() queries
using USMCapabilities = USMWithParam<cl_device_info>;
TEST_P(USMCapabilities, DeviceMemoryCapabilties) {
  const cl_device_info param_name = GetParam();

  cl_device_unified_shared_memory_capabilities_intel capabilities;
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, param_name, 0, nullptr, &size));
  ASSERT_EQ(size, sizeof(capabilities));

  ASSERT_SUCCESS(clGetDeviceInfo(device, param_name, sizeof(capabilities),
                                 &capabilities, nullptr));

  // Required capability for extension
  if (param_name == CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL) {
    EXPECT_TRUE(0 != (CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL & capabilities));
  }
}
INSTANTIATE_TEST_SUITE_P(
    USMTests, USMCapabilities,
    ::testing::Values(CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                      CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL,
                      CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
                      CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
                      CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL));
