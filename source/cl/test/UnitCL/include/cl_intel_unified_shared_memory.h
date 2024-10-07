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
#ifndef UNITCL_USM_H_INCLUDED
#define UNITCL_USM_H_INCLUDED

#include <CL/cl_ext.h>
#include <cargo/error.h>
#include <cargo/small_vector.h>

#include "Common.h"

// Base class for checking if the USM extension is enabled. If so Setup queries
// for function pointers to new extension entry points that test fixtures
// can use.
struct cl_intel_unified_shared_memory_Test : public virtual ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_intel_unified_shared_memory")) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
        sizeof(shared_capabilities), &shared_capabilities, nullptr));

#define CL_GET_EXTENSION_ADDRESS(FUNC)                            \
  FUNC = reinterpret_cast<FUNC##_fn>(                             \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC)); \
  ASSERT_NE(nullptr, FUNC);

    CL_GET_EXTENSION_ADDRESS(clHostMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clDeviceMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clSharedMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clMemFreeINTEL);
    CL_GET_EXTENSION_ADDRESS(clMemBlockingFreeINTEL);
    CL_GET_EXTENSION_ADDRESS(clGetMemAllocInfoINTEL);
    CL_GET_EXTENSION_ADDRESS(clSetKernelArgMemPointerINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemFillINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemcpyINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMigrateMemINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemAdviseINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemsetINTEL);

#undef GET_EXTENSION_ADDRESS
  }

  void TearDown() override {
    for (auto ptr : allPointers()) {
      const cl_int err = clMemBlockingFreeINTEL(context, ptr);
      EXPECT_SUCCESS(err);
    }
    device_ptr = shared_ptr = host_shared_ptr = host_ptr = nullptr;

    ucl::ContextTest::TearDown();
  }

  static void *getPointerOffset(void *ptr, size_t offset) {
    cl_char *byte_ptr = reinterpret_cast<cl_char *>(ptr) + offset;
    return reinterpret_cast<void *>(byte_ptr);
  }

  /// Allocates USM pointers with the given size of alignment
  ///
  /// Device, host and/or shared USM allocations will be allocated, depending on
  /// what the device supports. The pointers will be available in the
  /// `device_ptr`, `host_ptr` and `shared_ptr` members of this object, and will
  /// be freed during TearDown.
  void initPointers(size_t bytes, size_t align) {
    cl_int err;

    if (host_capabilities) {
      ASSERT_TRUE(host_ptr == nullptr);
      host_ptr = clHostMemAllocINTEL(context, {}, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr != nullptr);
    }

    if (shared_capabilities != 0) {
      ASSERT_TRUE(shared_ptr == nullptr);
      shared_ptr =
          clSharedMemAllocINTEL(context, device, {}, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(shared_ptr != nullptr);

      ASSERT_TRUE(host_shared_ptr == nullptr);
      host_shared_ptr =
          clSharedMemAllocINTEL(context, nullptr, {}, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_shared_ptr != nullptr);
    }

    ASSERT_TRUE(device_ptr == nullptr);
    device_ptr =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);
  }

  /// Return a vector of available USM pointers allocated by `initPointers`
  ///
  /// This will return an array containing up to 3 pointers, one of each device,
  /// host and shared, depending on the capabilities of the device.
  cargo::small_vector<void *, 4> allPointers() {
    cargo::small_vector<void *, 4> to_return{};

    if (device_ptr) {
      (void)to_return.push_back(device_ptr);
    }
    if (shared_ptr) {
      (void)to_return.push_back(shared_ptr);
    }
    if (host_shared_ptr) {
      (void)to_return.push_back(host_shared_ptr);
    }
    if (host_ptr) {
      (void)to_return.push_back(host_ptr);
    }

    return to_return;
  }

  static constexpr size_t MAX_NUM_POINTERS = 3;
  void *host_ptr = nullptr;
  void *shared_ptr = nullptr;
  void *host_shared_ptr = nullptr;
  void *device_ptr = nullptr;

  cl_device_unified_shared_memory_capabilities_intel host_capabilities = 0;
  cl_device_unified_shared_memory_capabilities_intel shared_capabilities = 0;

  clHostMemAllocINTEL_fn clHostMemAllocINTEL = nullptr;
  clDeviceMemAllocINTEL_fn clDeviceMemAllocINTEL = nullptr;
  clSharedMemAllocINTEL_fn clSharedMemAllocINTEL = nullptr;
  clMemFreeINTEL_fn clMemFreeINTEL = nullptr;
  clMemBlockingFreeINTEL_fn clMemBlockingFreeINTEL = nullptr;
  clGetMemAllocInfoINTEL_fn clGetMemAllocInfoINTEL = nullptr;
  clSetKernelArgMemPointerINTEL_fn clSetKernelArgMemPointerINTEL = nullptr;
  clEnqueueMemFillINTEL_fn clEnqueueMemFillINTEL = nullptr;
  clEnqueueMemcpyINTEL_fn clEnqueueMemcpyINTEL = nullptr;
  clEnqueueMigrateMemINTEL_fn clEnqueueMigrateMemINTEL = nullptr;
  clEnqueueMemAdviseINTEL_fn clEnqueueMemAdviseINTEL = nullptr;
  clEnqueueMemsetINTEL_fn clEnqueueMemsetINTEL = nullptr;
};

template <typename T>
class USMWithParam : public cl_intel_unified_shared_memory_Test,
                     public ::testing::WithParamInterface<T> {};

#endif  // UNITCL_USM_H_INCLUDED
