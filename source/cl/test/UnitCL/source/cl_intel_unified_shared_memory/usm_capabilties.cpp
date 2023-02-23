// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
