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
      [](mux_command_buffer_t, mux_result_t, void *const user_data) {
        *static_cast<std::atomic<bool> *>(user_data) = true;
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
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto data = static_cast<uint32_t *>(user_data);
        *data *= 2;
      },
      &data, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, &semaphore, 1,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(
      queue, command_buffer_2, nullptr, nullptr, 0, &semaphore, 1,
      [](mux_command_buffer_t, mux_result_t, void *const user_data) {
        auto data = static_cast<uint32_t *>(user_data);
        (*data)++;
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

// Test that resetting a waited on semaphore does not deadlock dispatched
// command buffers.
//
// TODO CA-1579: Fix tsan warning about multiple threads accessing semaphores.
TEST_P(muxDispatchTest, DISABLED_NoDeadlockWhenResettingWaitedOnSemaphore) {
  // Create chain of three command buffers. Second one waiting on signal of
  // first one. Last one waiting on signal of first and second one.
  // The second command buffer runs a command that resets the signal from the
  // first one.
  // This should not lead to a deadlock of the third command buffer never
  // being ready to run.

  mux_semaphore_t semaphores[2];
  for (auto &sem : semaphores) {
    ASSERT_ERROR_EQ(mux_success, muxCreateSemaphore(device, allocator, &sem));
  }

  mux_command_buffer_t command_buffers[3];
  for (auto &command_buffer : command_buffers) {
    ASSERT_ERROR_EQ(
        mux_success,
        muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));
  }

  mux_fence_t fence = nullptr;
  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));

  // Set up middle command buffer to reset signal semaphore of first command
  // group it waited on.
  ASSERT_ERROR_EQ(
      mux_success,
      muxCommandUserCallback(
          command_buffers[1],
          [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
            auto semaphore = static_cast<mux_semaphore_t>(user_data);
            muxResetSemaphore(semaphore);
          },
          semaphores[0], 0, nullptr, nullptr));

  // Dispatch in opposite order to ensure that wait semaphores aren't
  // signaled yet.
  ASSERT_ERROR_EQ(mux_success,
                  muxDispatch(queue, command_buffers[2], fence, semaphores, 2,
                              nullptr, 0, nullptr, nullptr));
  ASSERT_ERROR_EQ(mux_success, muxDispatch(queue, command_buffers[1], nullptr,
                                           &semaphores[0], 1, &semaphores[1], 1,
                                           nullptr, nullptr));
  ASSERT_ERROR_EQ(mux_success,
                  muxDispatch(queue, command_buffers[0], nullptr, nullptr, 0,
                              &semaphores[0], 1, nullptr, nullptr));

  // Wait on enqueued work. This should not deadlock, otherwise the
  // OpenCL to Mux mapping **may** also deadlock.
  ASSERT_ERROR_EQ(mux_success, muxTryWait(queue, UINT64_MAX, fence));

  // Cleanup.
  muxDestroyFence(device, fence, allocator);
  for (auto &command_buffer : command_buffers) {
    muxDestroyCommandBuffer(device, command_buffer, allocator);
  }
  for (auto &sem : semaphores) {
    muxDestroySemaphore(device, sem, allocator);
  }
}

// TODO CA-1579: Fix tsan warning about multiple threads accessing semaphores.
TEST_P(muxDispatchTest, DISABLED_MultipleThreadsInteractWithSemaphores) {
  // Only a single semaphore is used, however only with this array the
  // ThreadSanizier warns.
  mux_semaphore_t semaphores[2];
  for (auto &sem : semaphores) {
    ASSERT_ERROR_EQ(mux_success, muxCreateSemaphore(device, allocator, &sem));
  }

  mux_command_buffer_t command_buffers[2];
  for (auto &command_buffer : command_buffers) {
    ASSERT_ERROR_EQ(
        mux_success,
        muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));
  }

  // Set up command buffer to reset signal semaphore of first command buffer it
  // waited on. This seems to trigger ThreadSanitizer warnings.
  ASSERT_ERROR_EQ(
      mux_success,
      muxCommandUserCallback(
          command_buffers[1],
          [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
            auto semaphore = static_cast<mux_semaphore_t>(user_data);
            muxResetSemaphore(semaphore);
          },
          semaphores[0], 0, nullptr, nullptr));

  ASSERT_ERROR_EQ(mux_success,
                  muxDispatch(queue, command_buffers[1], nullptr,
                              &semaphores[0], 1, nullptr, 0, nullptr, nullptr));
  ASSERT_ERROR_EQ(mux_success,
                  muxDispatch(queue, command_buffers[0], nullptr, nullptr, 0,
                              &semaphores[0], 1, nullptr, nullptr));

  ASSERT_ERROR_EQ(mux_success, muxWaitAll(queue));

  // Cleanup.
  for (auto &command_buffer : command_buffers) {
    muxDestroyCommandBuffer(device, command_buffer, allocator);
  }
  for (auto &sem : semaphores) {
    muxDestroySemaphore(device, sem, allocator);
  }
}
