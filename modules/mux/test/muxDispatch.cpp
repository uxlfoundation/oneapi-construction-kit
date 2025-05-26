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

#include <atomic>
#include <cstdint>

#include "common.h"
#include "mux/mux.h"

struct muxDispatchTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_queue_t queue = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));

    if (0 < device->info->queue_types[mux_queue_type_compute]) {
      ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    }
  }

  void TearDown() override {
    if (device) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDispatchTest);

TEST_P(muxDispatchTest, Default) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));
}

TEST_P(muxDispatchTest, CompleteCallback) {
  std::atomic<bool> hit(false);

  // add a callback to the dispatch that sets hit to true
  ASSERT_SUCCESS(muxDispatch(
      queue, command_buffer, nullptr, nullptr, 0, nullptr, 0,
      [](mux_command_buffer_t, mux_result_t, void *const UserData) {
        *static_cast<std::atomic<bool> *>(UserData) = true;
      },
      &hit));

  while (!hit) {
  }

  ASSERT_SUCCESS(muxWaitAll(queue));
}

TEST_P(muxDispatchTest, UserFunctionBeforeSignal) {
  uint32_t data = 0;

  mux_semaphore_t semaphore;
  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));

  mux_command_buffer_t command_buffer_2;
  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer_2));

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const UserData) {
        auto Data = static_cast<uint32_t *>(UserData);
        *Data *= 2;
      },
      &data, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, &semaphore, 1,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(
      queue, command_buffer_2, nullptr, nullptr, 0, &semaphore, 1,
      [](mux_command_buffer_t, mux_result_t, void *const UserData) {
        auto Data = static_cast<uint32_t *>(UserData);
        (*Data)++;
      },
      &data));

  ASSERT_SUCCESS(muxWaitAll(queue));

  muxDestroyCommandBuffer(device, command_buffer_2, allocator);
  muxDestroySemaphore(device, semaphore, allocator);

  ASSERT_EQ(data, 2);
}

TEST_P(muxDispatchTest, WaitForSignalledSemaphore) {
  mux_semaphore_t semaphore;
  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));

  // dispatch an empty command buffer to signal the semaphore
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             &semaphore, 1, nullptr, nullptr));

  // wait until the semaphore has been signalled
  ASSERT_SUCCESS(muxWaitAll(queue));

  // wait on the semaphore that should now be in the signalled state
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, &semaphore, 1,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  muxDestroySemaphore(device, semaphore, allocator);
}
