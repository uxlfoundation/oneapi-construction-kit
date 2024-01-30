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

namespace {
struct USMMemSetTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    initPointers(bytes, align);

    cl_int err;
    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  static const cl_uint elements = 8;
  static const size_t bytes = sizeof(cl_int) * elements;
  static const size_t align = sizeof(cl_int);

  cl_command_queue queue = nullptr;
};
}  // namespace

TEST_F(USMMemSetTest, InvalidUsage) {
  // Test various error codes
  const cl_int val = 42;
  cl_int err = clEnqueueMemsetINTEL(nullptr, device_ptr, val, sizeof(cl_int), 0,
                                    nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);

  err = clEnqueueMemsetINTEL(queue, nullptr, val, sizeof(cl_int), 0, nullptr,
                             nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  err = clEnqueueMemsetINTEL(queue, device_ptr, val, 0, 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  err = clEnqueueMemsetINTEL(queue, device_ptr, val, 6, 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  const cl_short short_val = static_cast<cl_short>(val);
  err = clEnqueueMemsetINTEL(queue, device_ptr, short_val, sizeof(cl_short), 0,
                             nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
}

TEST_F(USMMemSetTest, DeviceAllocation) {
  // Zero initialize whole device allocation then fill with data using
  // clEnqueueMemsetINTEL and validate allocation contents.
  cl_int value = 0xA;
  cl_int err = clEnqueueMemsetINTEL(queue, device_ptr, value, bytes, 0, nullptr,
                                    nullptr);
  EXPECT_SUCCESS(err);
  value = CL_INT_MAX;
  err = clEnqueueMemsetINTEL(queue, device_ptr, value, sizeof(value), 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  void *offset_device_ptr = getPointerOffset(device_ptr, bytes / 2);
  value = CL_INT_MIN;
  err = clEnqueueMemsetINTEL(queue, offset_device_ptr, value, bytes / 2, 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    // Copy whole device allocation to host for result validation
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr, device_ptr, bytes, 0,
                               nullptr, nullptr);
    EXPECT_SUCCESS(err);
  }

  EXPECT_SUCCESS(clFinish(queue));

  if (host_ptr) {
    cl_int *host_validation_ptr = reinterpret_cast<cl_int *>(host_ptr);
    const cl_int correct_result[8] = {CL_INT_MAX, 0xA,        0xA,
                                      0xA,        CL_INT_MIN, CL_INT_MIN,
                                      CL_INT_MIN, CL_INT_MIN};
    EXPECT_TRUE(0 == std::memcmp(host_validation_ptr, correct_result, bytes));
  }
}

TEST_F(USMMemSetTest, SharedAllocation) {
  if (!shared_ptr) {
    GTEST_SKIP();
  }

  // Zero initialize whole shared allocation then fill with data using
  // clEnqueueMemsetINTEL and validate allocation contents.
  cl_int value = 0xA;
  cl_int err = clEnqueueMemsetINTEL(queue, shared_ptr, value, bytes, 0, nullptr,
                                    nullptr);
  EXPECT_SUCCESS(err);
  value = CL_INT_MAX;
  err = clEnqueueMemsetINTEL(queue, shared_ptr, value, sizeof(value), 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  void *offset_shared_ptr = getPointerOffset(shared_ptr, bytes / 2);
  value = CL_INT_MIN;
  err = clEnqueueMemsetINTEL(queue, offset_shared_ptr, value, bytes / 2, 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    // Copy whole device allocation to host for result validation
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr, shared_ptr, bytes, 0,
                               nullptr, nullptr);
    EXPECT_SUCCESS(err);
  }

  EXPECT_SUCCESS(clFinish(queue));

  if (host_ptr) {
    cl_int *host_validation_ptr = reinterpret_cast<cl_int *>(host_ptr);
    const cl_int correct_result[8] = {CL_INT_MAX, 0xA,        0xA,
                                      0xA,        CL_INT_MIN, CL_INT_MIN,
                                      CL_INT_MIN, CL_INT_MIN};
    EXPECT_TRUE(0 == std::memcmp(host_validation_ptr, correct_result, bytes));
  }
}

TEST_F(USMMemSetTest, HostAllocation) {
  if (!host_ptr) {
    GTEST_SKIP();
  }

  // Zero initialize whole host allocation then fill with data using
  // clEnqueueMemsetINTEL and validate allocation contents.
  cl_int value = 0xA;
  cl_int err =
      clEnqueueMemsetINTEL(queue, host_ptr, value, bytes, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  value = CL_INT_MAX;
  err = clEnqueueMemsetINTEL(queue, host_ptr, value, sizeof(value), 0, nullptr,
                             nullptr);
  EXPECT_SUCCESS(err);

  void *offset_host_ptr = getPointerOffset(host_ptr, bytes / 2);
  value = CL_INT_MIN;
  err = clEnqueueMemsetINTEL(queue, offset_host_ptr, value, bytes / 2, 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clFinish(queue));

  const cl_int correct_result[8] = {CL_INT_MAX, 0xA,        0xA,
                                    0xA,        CL_INT_MIN, CL_INT_MIN,
                                    CL_INT_MIN, CL_INT_MIN};
  EXPECT_TRUE(0 == std::memcmp(host_ptr, correct_result, bytes));
}
