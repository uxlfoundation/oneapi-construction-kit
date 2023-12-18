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

#include <Common.h>

#include "cl_intel_unified_shared_memory.h"

using USMTests = cl_intel_unified_shared_memory_Test;
// Test for invalid API usage of clHostMemAllocINTEL
TEST_F(USMTests, HostMemAlloc_InvalidUsage) {
  cl_device_unified_shared_memory_capabilities_intel capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                                 sizeof(capabilities), &capabilities, nullptr));
  const bool host_alloc_unsupported = (0 == capabilities);

  const size_t bytes = 256;
  const cl_uint align = 4;
  cl_int err;

  // Invalid context
  void *host_ptr = clHostMemAllocINTEL(nullptr, nullptr, bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_EQ(host_ptr, nullptr);

  // Alignment larger than max OpenCL type size supported by context
  host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, 256, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Alignment not a power of two
  host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, 6, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Zero byte allocation
  host_ptr = clHostMemAllocINTEL(context, nullptr, 0, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Allocation greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE
  cl_ulong max_alloc_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                 sizeof(cl_ulong), &max_alloc_size, nullptr));
  host_ptr = clHostMemAllocINTEL(context, nullptr,
                                 max_alloc_size + sizeof(cl_int), align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Duplicate properties
  cl_mem_properties_intel duplicate_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL,
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  host_ptr =
      clHostMemAllocINTEL(context, duplicate_properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Invalid value for CL_MEM_ALLOC_FLAGS_INTEL property
  cl_mem_properties_intel bad_value_properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                    0xFFFF, 0};
  host_ptr =
      clHostMemAllocINTEL(context, bad_value_properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Invalid property name
  cl_mem_properties_intel bad_name_properties[] = {
      0xFFFF, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  host_ptr =
      clHostMemAllocINTEL(context, bad_name_properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  }
  EXPECT_EQ(host_ptr, nullptr);

  // Mutually exclusive properties which should error
  cl_mem_properties_intel conflicting_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL,
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
          CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
      0};
  host_ptr =
      clHostMemAllocINTEL(context, conflicting_properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
  } else {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  }
  EXPECT_EQ(host_ptr, nullptr);
}

// Test for valid API usage of clHostMemAllocINTEL
TEST_F(USMTests, HostMemAlloc_ValidUsage) {
  cl_device_unified_shared_memory_capabilities_intel capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                                 sizeof(capabilities), &capabilities, nullptr));
  const bool host_alloc_unsupported = (0 == capabilities);

  const size_t bytes = 128;
  const cl_uint align = 4;
  cl_int err;

  void *host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(host_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(host_ptr != nullptr);
  }

  if (host_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }

  host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, 0, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(host_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(host_ptr != nullptr);
  }

  if (host_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel no_properties[] = {0};
  host_ptr = clHostMemAllocINTEL(context, no_properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(host_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(host_ptr != nullptr);
  }

  if (host_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL, 0, 0};
  host_ptr = clHostMemAllocINTEL(context, properties, bytes, align, &err);
  if (host_alloc_unsupported) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(host_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(host_ptr != nullptr);
  }

  if (host_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }
}

// Test for invalid API usage of clDeviceMemAllocINTEL
TEST_F(USMTests, DeviceMemAlloc_InvalidUsage) {
  const size_t bytes = 256;
  const cl_uint align = 4;
  cl_int err;

  // Invalid context
  void *device_ptr =
      clDeviceMemAllocINTEL(nullptr, device, nullptr, bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_EQ(device_ptr, nullptr);

  // Invalid device
  device_ptr =
      clDeviceMemAllocINTEL(context, nullptr, nullptr, bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_DEVICE);
  EXPECT_EQ(device_ptr, nullptr);

  // Alignment larger than max OpenCL type size supported by context
  device_ptr =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, 256, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(device_ptr, nullptr);

  // Alignment not a power of two
  device_ptr = clDeviceMemAllocINTEL(context, device, nullptr, bytes, 6, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(device_ptr, nullptr);

  // Zero byte allocation
  device_ptr = clDeviceMemAllocINTEL(context, device, nullptr, 0, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(device_ptr, nullptr);

  // Allocation greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE
  cl_ulong max_alloc_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                 sizeof(cl_ulong), &max_alloc_size, nullptr));
  device_ptr = clDeviceMemAllocINTEL(
      context, device, nullptr, max_alloc_size + sizeof(cl_int), align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(device_ptr, nullptr);

  // Duplicate properties
  cl_mem_properties_intel duplicate_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL,
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  device_ptr = clDeviceMemAllocINTEL(context, device, duplicate_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(device_ptr, nullptr);

  // Invalid value for CL_MEM_ALLOC_FLAGS_INTEL property
  cl_mem_properties_intel bad_value_properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                    0xFFFF, 0};
  device_ptr = clDeviceMemAllocINTEL(context, device, bad_value_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(device_ptr, nullptr);

  // Invalid property name
  cl_mem_properties_intel bad_name_properties[] = {
      0xFFFF, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  device_ptr = clDeviceMemAllocINTEL(context, device, bad_name_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(device_ptr, nullptr);

  // Mutually exclusive properties which should error
  cl_mem_properties_intel conflicting_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL,
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
          CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
      0};
  device_ptr = clDeviceMemAllocINTEL(context, device, conflicting_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(device_ptr, nullptr);
}

// Test for valid API usage of clDeviceMemAllocINTEL
TEST_F(USMTests, DeviceMemAlloc_ValidUsage) {
  const size_t bytes = 128;
  const cl_uint align = 4;
  cl_int err;

  void *device_ptr =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  EXPECT_SUCCESS(err);
  EXPECT_TRUE(device_ptr != nullptr);

  if (device_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
    EXPECT_SUCCESS(err);
  }

  device_ptr = clDeviceMemAllocINTEL(context, device, nullptr, bytes, 0, &err);
  EXPECT_SUCCESS(err);
  EXPECT_TRUE(device_ptr != nullptr);

  if (device_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel no_properties[] = {0};
  device_ptr =
      clDeviceMemAllocINTEL(context, device, no_properties, bytes, align, &err);
  EXPECT_SUCCESS(err);
  EXPECT_TRUE(device_ptr != nullptr);

  if (device_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL, 0, 0};
  device_ptr =
      clDeviceMemAllocINTEL(context, device, properties, bytes, align, &err);
  EXPECT_SUCCESS(err);
  EXPECT_TRUE(device_ptr != nullptr);

  if (device_ptr) {
    cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
    EXPECT_SUCCESS(err);
  }
}

// Test for invalid API usage of clSharedMemAllocINTEL with an associated device
TEST_F(USMTests, SingleSharedMemAlloc_InvalidUsage) {
  const size_t bytes = 256;
  const cl_uint align = 4;
  cl_int err;

  // Require shared USM support - otherwise these functions may return
  // CL_INVALID_OPERATION
  cl_device_unified_shared_memory_capabilities_intel capabilities;
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(capabilities), &capabilities, nullptr));
  if (0 == capabilities) {
    GTEST_SKIP();
  }

  // Invalid context
  void *shared_ptr =
      clSharedMemAllocINTEL(nullptr, device, nullptr, bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_EQ(shared_ptr, nullptr);

  // Invalid device
  cl_device_id invalid_device = reinterpret_cast<cl_device_id>(0x1);
  shared_ptr = clSharedMemAllocINTEL(context, invalid_device, nullptr, bytes,
                                     align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_DEVICE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Alignment larger than max OpenCL type size supported by context
  shared_ptr =
      clSharedMemAllocINTEL(context, device, nullptr, bytes, 256, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Alignment not a power of two
  shared_ptr = clSharedMemAllocINTEL(context, device, nullptr, bytes, 6, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Zero byte allocation
  shared_ptr = clSharedMemAllocINTEL(context, device, nullptr, 0, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Allocation greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE
  cl_ulong max_alloc_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                 sizeof(cl_ulong), &max_alloc_size, nullptr));
  shared_ptr = clSharedMemAllocINTEL(
      context, device, nullptr, max_alloc_size + sizeof(cl_int), align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Duplicate properties
  cl_mem_properties_intel duplicate_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL,
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  shared_ptr = clSharedMemAllocINTEL(context, device, duplicate_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Invalid value for CL_MEM_ALLOC_FLAGS_INTEL property
  cl_mem_properties_intel bad_value_properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                    0xFFFF, 0};
  shared_ptr = clSharedMemAllocINTEL(context, device, bad_value_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Invalid property name
  cl_mem_properties_intel bad_name_properties[] = {
      0xFFFF, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  shared_ptr = clSharedMemAllocINTEL(context, device, bad_name_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Mutually exclusive properties which should error
  cl_mem_properties_intel conflicting_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL,
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
          CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
      0};
  shared_ptr = clSharedMemAllocINTEL(context, device, conflicting_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);
}

// Test for valid API usage of clSharedMemAllocINTEL with an associated device
TEST_F(USMTests, SingleSharedMemAlloc_ValidUsage) {
  cl_device_unified_shared_memory_capabilities_intel single_capabilities,
      cross_capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(single_capabilities), &single_capabilities, nullptr));
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(cross_capabilities), &cross_capabilities, nullptr));
  const bool shared_mem_support = single_capabilities != 0;

  const size_t bytes = 128;
  const cl_uint align = 4;
  cl_int err;

  void *shared_ptr =
      clSharedMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }

  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }
  shared_ptr = clSharedMemAllocINTEL(context, device, nullptr, bytes, 0, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }

  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel no_properties[] = {0};
  shared_ptr =
      clSharedMemAllocINTEL(context, device, no_properties, bytes, align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }
  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL, 0, 0};
  shared_ptr =
      clSharedMemAllocINTEL(context, device, properties, bytes, align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }
}

// Test for invalid API usage of clSharedMemAllocINTEL without an associated
// device
TEST_F(USMTests, CrossSharedMemAlloc_InvalidUsage) {
  const size_t bytes = 256;
  const cl_uint align = 4;
  cl_int err;

  // Require shared USM support - otherwise these functions may return
  // CL_INVALID_OPERATION
  cl_device_unified_shared_memory_capabilities_intel capabilities;
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(capabilities), &capabilities, nullptr));
  if (0 == capabilities) {
    GTEST_SKIP();
  }

  // Invalid context
  void *shared_ptr =
      clSharedMemAllocINTEL(nullptr, nullptr, nullptr, bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_EQ(shared_ptr, nullptr);

  // Alignment larger than max OpenCL type size supported by context
  shared_ptr =
      clSharedMemAllocINTEL(context, nullptr, nullptr, bytes, 256, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Alignment not a power of two
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, nullptr, bytes, 6, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Zero byte allocation
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, nullptr, 0, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Allocation greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE
  cl_ulong max_alloc_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                 sizeof(cl_ulong), &max_alloc_size, nullptr));
  shared_ptr = clSharedMemAllocINTEL(
      context, nullptr, nullptr, max_alloc_size + sizeof(cl_int), align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_BUFFER_SIZE);
  EXPECT_EQ(shared_ptr, nullptr);

  // Duplicate properties
  cl_mem_properties_intel duplicate_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL,
      CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, duplicate_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Invalid value for CL_MEM_ALLOC_FLAGS_INTEL property
  const cl_mem_properties_intel bad_value_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL, 0xFFFF, 0};
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, bad_value_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Invalid property name
  const cl_mem_properties_intel bad_name_properties[] = {
      0xFFFF, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, bad_name_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);

  // Mutually exclusive properties which should error
  cl_mem_properties_intel conflicting_properties[] = {
      CL_MEM_ALLOC_FLAGS_INTEL,
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
          CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
      0};
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, conflicting_properties,
                                     bytes, align, &err);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
  EXPECT_EQ(shared_ptr, nullptr);
}

// Test for valid API usage of clSharedMemAllocINTEL without an associated
// device
TEST_F(USMTests, CrossSharedMemAlloc_ValidUsage) {
  cl_device_unified_shared_memory_capabilities_intel single_capabilities,
      cross_capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(single_capabilities), &single_capabilities, nullptr));
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(cross_capabilities), &cross_capabilities, nullptr));
  const bool shared_mem_support = single_capabilities != 0;

  const size_t bytes = 128;
  const cl_uint align = 4;
  cl_int err;

  void *shared_ptr =
      clSharedMemAllocINTEL(context, nullptr, nullptr, bytes, align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }

  shared_ptr = clSharedMemAllocINTEL(context, nullptr, nullptr, bytes, 0, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel no_properties[] = {0};
  shared_ptr = clSharedMemAllocINTEL(context, nullptr, no_properties, bytes,
                                     align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL, 0, 0};
  shared_ptr =
      clSharedMemAllocINTEL(context, nullptr, properties, bytes, align, &err);
  if (!shared_mem_support) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_OPERATION);
    EXPECT_EQ(shared_ptr, nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(shared_ptr != nullptr);
  }
  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }
}

using USMAllocFlagTest = USMWithParam<cl_mem_alloc_flags_intel>;
TEST_P(USMAllocFlagTest, DeviceAlloc) {
  cl_mem_alloc_flags_intel alloc_flags = GetParam();

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                alloc_flags, 0};
  cl_int err = ~CL_SUCCESS;
  void *device_ptr = clDeviceMemAllocINTEL(
      context, device, properties, sizeof(cl_int), sizeof(cl_int), &err);

  // Flags invalid for device allocation
  const cl_mem_alloc_flags_intel invalid_flags =
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
      CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL;
  if (alloc_flags & invalid_flags) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
    EXPECT_TRUE(device_ptr == nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(device_ptr != nullptr);

    err = clMemBlockingFreeINTEL(context, device_ptr);
    EXPECT_SUCCESS(err);
  }
}

TEST_P(USMAllocFlagTest, HostAlloc) {
  cl_device_unified_shared_memory_capabilities_intel capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                                 sizeof(capabilities), &capabilities, nullptr));
  if (0 == capabilities) {
    GTEST_SKIP();
  }

  cl_mem_alloc_flags_intel alloc_flags = GetParam();

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                alloc_flags, 0};
  cl_int err = ~CL_SUCCESS;
  void *host_ptr = clHostMemAllocINTEL(context, properties, sizeof(cl_int),
                                       sizeof(cl_int), &err);

  // Flags invalid for host allocation
  const cl_mem_alloc_flags_intel invalid_flags =
      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
      CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL;
  if (alloc_flags & invalid_flags) {
    EXPECT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
    EXPECT_TRUE(host_ptr == nullptr);
  } else {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(host_ptr != nullptr);

    err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }
}

TEST_P(USMAllocFlagTest, SharedAlloc) {
  cl_device_unified_shared_memory_capabilities_intel capabilities;

  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
      sizeof(capabilities), &capabilities, nullptr));
  if (0 == capabilities) {
    GTEST_SKIP();
  }

  cl_mem_alloc_flags_intel alloc_flags = GetParam();

  const cl_mem_properties_intel properties[] = {CL_MEM_ALLOC_FLAGS_INTEL,
                                                alloc_flags, 0};
  cl_int err = ~CL_SUCCESS;
  void *shared_ptr = clSharedMemAllocINTEL(
      context, device, properties, sizeof(cl_int), sizeof(cl_int), &err);

  EXPECT_SUCCESS(err);
  EXPECT_TRUE(shared_ptr != nullptr);

  if (shared_ptr) {
    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }
}

INSTANTIATE_TEST_SUITE_P(
    USMTests, USMAllocFlagTest,
    ::testing::Values(CL_MEM_ALLOC_WRITE_COMBINED_INTEL,
                      CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL,
                      CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
                      CL_MEM_ALLOC_WRITE_COMBINED_INTEL |
                          CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL,
                      CL_MEM_ALLOC_WRITE_COMBINED_INTEL |
                          CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL));
