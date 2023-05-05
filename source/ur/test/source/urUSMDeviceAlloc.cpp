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

using urUSMDeviceAllocTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urUSMDeviceAllocTest);

TEST_P(urUSMDeviceAllocTest, Success) {
  void *ptr{nullptr};
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0, &ptr));
  ASSERT_NE(ptr, nullptr);

  ur_event_handle_t event = nullptr;
  int zero_val = 0;
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, ptr, sizeof(zero_val), &zero_val,
                                  sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  // TODO Make sure memset on device happened.

  ASSERT_SUCCESS(urUSMFree(context, ptr));
  EXPECT_SUCCESS(urEventRelease(event));
}

TEST_P(urUSMDeviceAllocTest, InvalidNullContext) {
  void *ptr{nullptr};
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urUSMDeviceAlloc(nullptr, device, nullptr, nullptr,
                                    sizeof(int), 0, &ptr));
}

TEST_P(urUSMDeviceAllocTest, InvalidDevice) {
  void *ptr{nullptr};
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_DEVICE,
                   urUSMDeviceAlloc(context, nullptr, nullptr, nullptr,
                                    sizeof(int), 0, &ptr));
}

TEST_P(urUSMDeviceAllocTest, InvalidNullPtrResult) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                    sizeof(int), 0, nullptr));
}
