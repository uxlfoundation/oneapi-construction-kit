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

#include <mux/utils/helpers.h>

#include <cstdint>

#include "common.h"
#include "mux/mux.h"

struct muxResetSemaphoreTest : DeviceTest {
  mux_command_buffer_t command_buffer0 = nullptr;
  mux_command_buffer_t command_buffer1 = nullptr;
  mux_semaphore_t semaphore = nullptr;
  mux_queue_t queue = nullptr;
  mux_fence_t fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (device->info->queue_types[mux_queue_type_compute] == 0) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer0));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer1));
    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroySemaphore(device, semaphore, allocator);
      muxDestroyCommandBuffer(device, command_buffer1, allocator);
      muxDestroyCommandBuffer(device, command_buffer0, allocator);
      muxDestroyFence(device, fence, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxResetSemaphoreTest);

TEST_P(muxResetSemaphoreTest, Default) {
  // This code tests that a reset semaphore can be triggered again. It does so
  // by marching i from 42 -> 13 -> 56 -> 79 using user callbacks, and by
  // enforcing an ordering of the command groups via the reset semaphore we can
  // test that they are all hit in the correct order.
  uint32_t foo = 42;

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer0,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *foo = static_cast<uint32_t *>(user_data);
        if (42 == *foo) {
          // This will happen on the first run of the command group
          *foo = 13;
        } else if (56 == *foo) {
          // This will happen on the second run of the command group
          *foo = 79;
        }
      },
      &foo, 0, nullptr, nullptr));

  // Signal the semaphore the first time
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer0, fence, nullptr, 0,
                             &semaphore, 1, nullptr, nullptr));

  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));

  // Reset the semaphore for use again
  ASSERT_SUCCESS(muxResetSemaphore(semaphore));

  // This waits on the semaphore to trigger
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer0, nullptr, &semaphore, 1,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer1,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *foo = static_cast<uint32_t *>(user_data);
        if (13 == *foo) {
          *foo = 56;
        }
      },
      &foo, 0, nullptr, nullptr));

  // Signal the semaphore the second time
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer1, nullptr, nullptr, 0,
                             &semaphore, 1, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_EQ(79, foo);
}
