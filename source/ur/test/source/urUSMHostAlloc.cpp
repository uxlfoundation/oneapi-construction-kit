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

#include "uur/fixtures.h"

using urUSMHostAllocTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urUSMHostAllocTest);

TEST_P(urUSMHostAllocTest, Success) {
  bool host_usm{false};
  const size_t size{sizeof(bool)};
  ASSERT_SUCCESS(urDeviceGetInfo(device, UR_DEVICE_INFO_HOST_UNIFIED_MEMORY,
                                 size, &host_usm, nullptr));
  if (!host_usm) {  // Skip this test if device does not support Host USM
    GTEST_SKIP();
  }

  int *ptr{nullptr};
  ASSERT_SUCCESS(urUSMHostAlloc(context, nullptr, nullptr, sizeof(int), 0,
                                reinterpret_cast<void **>(&ptr)));
  ASSERT_NE(ptr, nullptr);

  // Set 0
  int zero_val = 0;
  ur_event_handle_t event = nullptr;
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, ptr, sizeof(zero_val), &zero_val,
                                  sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));
  ASSERT_EQ(*ptr, 0);

  // Set 1, in all bytes of int
  char one_val = 1;
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, ptr, sizeof(one_val), &one_val,
                                  sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));
  // replicate it on host
  int set_data = 0;
  std::memset(&set_data, 1, sizeof(int));
  ASSERT_EQ(*ptr, set_data);

  ASSERT_SUCCESS(urUSMFree(context, ptr));
}

TEST_P(urUSMHostAllocTest, InvalidNullHandleContext) {
  void *ptr{nullptr};
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urUSMHostAlloc(nullptr, nullptr, nullptr, sizeof(int), 0, &ptr));
}

TEST_P(urUSMHostAllocTest, InvalidNullPtrResult) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urUSMHostAlloc(context, nullptr, nullptr, sizeof(int), 0, nullptr));
}
