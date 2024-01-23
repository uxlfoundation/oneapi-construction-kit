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

struct USMMemInfoTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));
    // Allocate with properties so we can test that we can query them
    const cl_mem_properties_intel properties[] = {
        CL_MEM_ALLOC_FLAGS_INTEL, CL_MEM_ALLOC_WRITE_COMBINED_INTEL, 0};

    cl_int err;
    if (host_capabilities != 0) {
      host_ptr = clHostMemAllocINTEL(context, properties, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr != nullptr);
    }

    cl_device_unified_shared_memory_capabilities_intel shared_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
        sizeof(shared_capabilities), &shared_capabilities, nullptr));
    if (shared_capabilities != 0) {
      shared_ptr =
          clSharedMemAllocINTEL(context, device, {}, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(shared_ptr != nullptr);
    }

    device_ptr =
        clDeviceMemAllocINTEL(context, device, properties, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);

    user_ptr = malloc(bytes);
  }

  void TearDown() override {
    if (user_ptr) {
      free(user_ptr);
    }

    if (device_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
      EXPECT_SUCCESS(err);
    }

    if (shared_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, shared_ptr);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
      EXPECT_SUCCESS(err);
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }
  const size_t bytes = 256;
  const cl_uint align = 4;

  void *user_ptr = nullptr;
};

// Test for invalid API usage of clGetMemAllocInfoINTEL()
TEST_F(USMMemInfoTest, InvalidUsage) {
  size_t alloc_size;
  cl_int err =
      clGetMemAllocInfoINTEL(nullptr, device_ptr, CL_MEM_ALLOC_SIZE_INTEL,
                             sizeof(alloc_size), &alloc_size, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);

  const cl_mem_info_intel bad_value_name = ~cl_mem_info_intel(0);
  err = clGetMemAllocInfoINTEL(context, device_ptr, bad_value_name,
                               sizeof(alloc_size), &alloc_size, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  const size_t small_size = sizeof(cl_char);
  err = clGetMemAllocInfoINTEL(context, device_ptr, CL_MEM_ALLOC_SIZE_INTEL,
                               small_size, &alloc_size, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
}

// Test for valid API usage of clGetMemAllocInfoINTEL() with
// CL_MEM_ALLOC_TYPE_INTEL
TEST_F(USMMemInfoTest, AllocType) {
  size_t param_size;
  cl_int err;

  if (host_ptr) {
    err = clGetMemAllocInfoINTEL(context, host_ptr, CL_MEM_ALLOC_TYPE_INTEL, 0,
                                 nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_unified_shared_memory_type_intel), param_size);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));
    cl_unified_shared_memory_type_intel host_alloc_type = 0;
    err = clGetMemAllocInfoINTEL(
        context, offset_host_ptr, CL_MEM_ALLOC_TYPE_INTEL,
        sizeof(host_alloc_type), &host_alloc_type, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(CL_MEM_TYPE_HOST_INTEL, host_alloc_type);
  }

  if (shared_ptr) {
    err = clGetMemAllocInfoINTEL(context, shared_ptr, CL_MEM_ALLOC_TYPE_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_unified_shared_memory_type_intel), param_size);

    void *offset_shared_ptr = getPointerOffset(shared_ptr, sizeof(cl_int));
    cl_unified_shared_memory_type_intel shared_alloc_type = 0;
    err = clGetMemAllocInfoINTEL(
        context, offset_shared_ptr, CL_MEM_ALLOC_TYPE_INTEL,
        sizeof(shared_alloc_type), &shared_alloc_type, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(CL_MEM_TYPE_SHARED_INTEL, shared_alloc_type);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));

  cl_unified_shared_memory_type_intel alloc_type = 0;
  err = clGetMemAllocInfoINTEL(context, offset_device_ptr,
                               CL_MEM_ALLOC_TYPE_INTEL, sizeof(alloc_type),
                               &alloc_type, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(CL_MEM_TYPE_DEVICE_INTEL, alloc_type);

  alloc_type = 0;
  err = clGetMemAllocInfoINTEL(context, user_ptr, CL_MEM_ALLOC_TYPE_INTEL,
                               sizeof(alloc_type), &alloc_type, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(CL_MEM_TYPE_UNKNOWN_INTEL, alloc_type);

  alloc_type = 0;
  err = clGetMemAllocInfoINTEL(context, nullptr, CL_MEM_ALLOC_TYPE_INTEL,
                               sizeof(alloc_type), &alloc_type, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(CL_MEM_TYPE_UNKNOWN_INTEL, alloc_type);
}

// Test for valid API usage of clGetMemAllocInfoINTEL() with
// CL_MEM_ALLOC_BASE_PTR_INTEL
TEST_F(USMMemInfoTest, AllocBasePtr) {
  size_t param_size;
  cl_int err;

  if (host_ptr) {
    err = clGetMemAllocInfoINTEL(context, host_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(void *), param_size);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));
    void *host_alloc_base_addr = nullptr;

    err = clGetMemAllocInfoINTEL(
        context, offset_host_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
        sizeof(host_alloc_base_addr), &host_alloc_base_addr, nullptr);

    EXPECT_SUCCESS(err);
    EXPECT_EQ(host_ptr, host_alloc_base_addr);
  }

  if (shared_ptr) {
    err =
        clGetMemAllocInfoINTEL(context, shared_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
                               0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(void *), param_size);

    void *offset_shared_ptr = getPointerOffset(shared_ptr, sizeof(cl_int));
    void *shared_alloc_base_addr = nullptr;

    err = clGetMemAllocInfoINTEL(
        context, offset_shared_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
        sizeof(shared_alloc_base_addr), &shared_alloc_base_addr, nullptr);

    EXPECT_SUCCESS(err);
    EXPECT_EQ(shared_ptr, shared_alloc_base_addr);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));
  void *alloc_base_addr = nullptr;

  err = clGetMemAllocInfoINTEL(
      context, offset_device_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
      sizeof(alloc_base_addr), &alloc_base_addr, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(device_ptr, alloc_base_addr);

  alloc_base_addr = nullptr;
  err = clGetMemAllocInfoINTEL(context, user_ptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
                               sizeof(alloc_base_addr), &alloc_base_addr,
                               nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(NULL, alloc_base_addr);

  alloc_base_addr = nullptr;
  err = clGetMemAllocInfoINTEL(context, nullptr, CL_MEM_ALLOC_BASE_PTR_INTEL,
                               sizeof(alloc_base_addr), &alloc_base_addr,
                               nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(NULL, alloc_base_addr);
}

// Test for valid API usage of clGetMemAllocInfoINTEL() with
// CL_MEM_ALLOC_SIZE_INTEL
TEST_F(USMMemInfoTest, AllocSize) {
  size_t param_size;
  cl_int err;

  if (host_ptr) {
    err = clGetMemAllocInfoINTEL(context, host_ptr, CL_MEM_ALLOC_SIZE_INTEL, 0,
                                 nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(size_t), param_size);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));
    size_t host_alloc_size = 0;

    err = clGetMemAllocInfoINTEL(
        context, offset_host_ptr, CL_MEM_ALLOC_SIZE_INTEL,
        sizeof(host_alloc_size), &host_alloc_size, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(bytes, host_alloc_size);
  }

  if (shared_ptr) {
    err = clGetMemAllocInfoINTEL(context, shared_ptr, CL_MEM_ALLOC_SIZE_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(size_t), param_size);

    void *offset_shared_ptr = getPointerOffset(shared_ptr, sizeof(cl_int));
    size_t shared_alloc_size = 0;

    err = clGetMemAllocInfoINTEL(
        context, offset_shared_ptr, CL_MEM_ALLOC_SIZE_INTEL,
        sizeof(shared_alloc_size), &shared_alloc_size, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(bytes, shared_alloc_size);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));

  size_t alloc_size = 0;
  err = clGetMemAllocInfoINTEL(context, offset_device_ptr,
                               CL_MEM_ALLOC_SIZE_INTEL, sizeof(alloc_size),
                               &alloc_size, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(bytes, alloc_size);

  alloc_size = ~size_t(0);
  err = clGetMemAllocInfoINTEL(context, user_ptr, CL_MEM_ALLOC_SIZE_INTEL,
                               sizeof(alloc_size), &alloc_size, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(0, alloc_size);

  alloc_size = ~size_t(0);
  err = clGetMemAllocInfoINTEL(context, nullptr, CL_MEM_ALLOC_SIZE_INTEL,
                               sizeof(alloc_size), &alloc_size, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(0, alloc_size);
}

// Test for valid API usage of clGetMemAllocInfoINTEL() with
// CL_MEM_ALLOC_DEVICE_INTEL
TEST_F(USMMemInfoTest, AllocDevice) {
  size_t param_size;
  cl_int err;

  if (host_ptr) {
    err = clGetMemAllocInfoINTEL(context, host_ptr, CL_MEM_ALLOC_DEVICE_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_device_id), param_size);
    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));

    cl_device_id host_alloc_device = device;
    err = clGetMemAllocInfoINTEL(
        context, offset_host_ptr, CL_MEM_ALLOC_DEVICE_INTEL,
        sizeof(host_alloc_device), &host_alloc_device, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(NULL, host_alloc_device);
  }

  if (shared_ptr) {
    err = clGetMemAllocInfoINTEL(context, shared_ptr, CL_MEM_ALLOC_DEVICE_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_device_id), param_size);
    void *offset_shared_ptr = getPointerOffset(shared_ptr, sizeof(cl_int));

    cl_device_id shared_alloc_device = device;
    err = clGetMemAllocInfoINTEL(
        context, offset_shared_ptr, CL_MEM_ALLOC_DEVICE_INTEL,
        sizeof(shared_alloc_device), &shared_alloc_device, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(device, shared_alloc_device);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));
  cl_device_id alloc_device = NULL;
  err = clGetMemAllocInfoINTEL(context, offset_device_ptr,
                               CL_MEM_ALLOC_DEVICE_INTEL, sizeof(alloc_device),
                               &alloc_device, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(device, alloc_device);

  alloc_device = device;
  err = clGetMemAllocInfoINTEL(context, user_ptr, CL_MEM_ALLOC_DEVICE_INTEL,
                               sizeof(alloc_device), &alloc_device, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(NULL, alloc_device);

  alloc_device = device;
  err = clGetMemAllocInfoINTEL(context, nullptr, CL_MEM_ALLOC_DEVICE_INTEL,
                               sizeof(alloc_device), &alloc_device, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(NULL, alloc_device);
}

// Test for valid API usage of clGetMemAllocInfoINTEL() with
// CL_MEM_ALLOC_FLAGS_INTEL
TEST_F(USMMemInfoTest, AllocFlags) {
  size_t param_size;
  cl_int err;

  if (host_ptr) {
    err = clGetMemAllocInfoINTEL(context, host_ptr, CL_MEM_ALLOC_FLAGS_INTEL, 0,
                                 nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_mem_alloc_flags_intel), param_size);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));

    cl_mem_alloc_flags_intel host_alloc_flags = 0;
    err = clGetMemAllocInfoINTEL(
        context, offset_host_ptr, CL_MEM_ALLOC_FLAGS_INTEL,
        sizeof(host_alloc_flags), &host_alloc_flags, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(CL_MEM_ALLOC_WRITE_COMBINED_INTEL, host_alloc_flags);
  }

  if (shared_ptr) {
    err = clGetMemAllocInfoINTEL(context, shared_ptr, CL_MEM_ALLOC_FLAGS_INTEL,
                                 0, nullptr, &param_size);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(sizeof(cl_mem_alloc_flags_intel), param_size);

    void *offset_shared_ptr = getPointerOffset(shared_ptr, sizeof(cl_int));

    cl_mem_alloc_flags_intel shared_alloc_flags = 0;
    err = clGetMemAllocInfoINTEL(
        context, offset_shared_ptr, CL_MEM_ALLOC_FLAGS_INTEL,
        sizeof(shared_alloc_flags), &shared_alloc_flags, nullptr);
    EXPECT_SUCCESS(err);
    EXPECT_EQ(0, shared_alloc_flags);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));

  cl_mem_alloc_flags_intel alloc_flags = 0;
  err = clGetMemAllocInfoINTEL(context, offset_device_ptr,
                               CL_MEM_ALLOC_FLAGS_INTEL, sizeof(alloc_flags),
                               &alloc_flags, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(CL_MEM_ALLOC_WRITE_COMBINED_INTEL, alloc_flags);

  alloc_flags = ~cl_mem_alloc_flags_intel(0);
  err = clGetMemAllocInfoINTEL(context, user_ptr, CL_MEM_ALLOC_FLAGS_INTEL,
                               sizeof(alloc_flags), &alloc_flags, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(0, alloc_flags);

  alloc_flags = ~cl_mem_alloc_flags_intel(0);
  err = clGetMemAllocInfoINTEL(context, nullptr, CL_MEM_ALLOC_FLAGS_INTEL,
                               sizeof(alloc_flags), &alloc_flags, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_EQ(0, alloc_flags);
}
