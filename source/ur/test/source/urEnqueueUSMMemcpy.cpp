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

using urEnqueueUSMMemcpyTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueUSMMemcpyTest);

TEST_P(urEnqueueUSMMemcpyTest, Success) {
  bool host_usm = false;
  const size_t size = sizeof(bool);
  ASSERT_SUCCESS(urDeviceGetInfo(device, UR_DEVICE_INFO_HOST_UNIFIED_MEMORY,
                                 size, &host_usm, nullptr));

  ur_event_handle_t event = nullptr;
  if (host_usm) {  // Only test host USM if device supports it
    int *host_dst = nullptr, *host_src = nullptr;
    ASSERT_SUCCESS(urUSMHostAlloc(context, nullptr, nullptr, sizeof(int), 0,
                                  reinterpret_cast<void **>(&host_dst)));

    ASSERT_SUCCESS(urUSMHostAlloc(context, nullptr, nullptr, sizeof(int), 0,
                                  reinterpret_cast<void **>(&host_src)));
    *host_src = 42;
    *host_dst = 0;
    ASSERT_SUCCESS(urEnqueueUSMMemcpy(queue, false, host_dst, host_src,
                                      sizeof(int), 0, nullptr, &event));
    EXPECT_SUCCESS(urQueueFlush(queue));
    ASSERT_SUCCESS(urEventWait(1, &event));
    EXPECT_SUCCESS(urEventRelease(event));
    ASSERT_EQ(*host_dst, *host_src);
    ASSERT_SUCCESS(urUSMFree(context, host_dst));
    ASSERT_SUCCESS(urUSMFree(context, host_src));
  }
  int *device_dst = nullptr, *device_src = nullptr;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&device_dst)));
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&device_src)));

  // Fill the allocations with different values first
  int zero_val = 0, one_val = 1;
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, device_dst, sizeof(zero_val),
                                  &zero_val, sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, device_src, sizeof(one_val), &one_val,
                                  sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));

  ASSERT_SUCCESS(urEnqueueUSMMemcpy(queue, false, device_dst, device_src,
                                    sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  EXPECT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));

  ASSERT_EQ(*device_dst, *device_src);

  ASSERT_SUCCESS(urUSMFree(context, device_dst));
  ASSERT_SUCCESS(urUSMFree(context, device_src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullQueueHandle) {
  int *dst = nullptr, *src = nullptr;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&dst)));

  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&src)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueUSMMemcpy(nullptr, false, dst, src, sizeof(int), 0,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(urUSMFree(context, dst));
  ASSERT_SUCCESS(urUSMFree(context, src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullPtr) {
  // We need a valid pointer to check the params separately.
  int *valid_ptr = nullptr;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&valid_ptr)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueUSMMemcpy(queue, false, valid_ptr, nullptr,
                                      sizeof(int), 0, nullptr, nullptr));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueUSMMemcpy(queue, false, nullptr, valid_ptr,
                                      sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urUSMFree(context, valid_ptr));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullPtrEventWaitList) {
  int *dst = nullptr, *src = nullptr;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&dst)));

  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&src)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueUSMMemcpy(queue, false, dst, src, sizeof(int), 1,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(urUSMFree(context, dst));
  ASSERT_SUCCESS(urUSMFree(context, src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidMemObject) {
  // We need a valid pointer to check the params separately.
  int *valid_ptr = nullptr;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0,
                                  reinterpret_cast<void **>(&valid_ptr)));

  // Random pointer which is not a usm allocation
  const intptr_t address = 0xDEADBEEF;
  int *bad_ptr = reinterpret_cast<int *>(address);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_MEM_OBJECT,
                   urEnqueueUSMMemcpy(queue, false, bad_ptr, valid_ptr,
                                      sizeof(int), 0, nullptr, nullptr));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_MEM_OBJECT,
                   urEnqueueUSMMemcpy(queue, false, valid_ptr, bad_ptr,
                                      sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urUSMFree(context, valid_ptr));
}
