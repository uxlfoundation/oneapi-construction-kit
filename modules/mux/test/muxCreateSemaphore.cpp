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

#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include "common.h"

struct muxCreateSemaphoreTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateSemaphoreTest);

TEST_P(muxCreateSemaphoreTest, Default) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxCreateSemaphoreTest, NullSemaphore) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateSemaphore(device, allocator, 0));
}

TEST_P(muxCreateSemaphoreTest, WaitMultipleSemaphores) {
  std::array<mux_semaphore_t, 2> semaphores;

  for (auto &semaphore : semaphores) {
    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  }

  mux_command_buffer_t signal_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &signal_buffer));

  mux_command_buffer_t wait_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &wait_buffer));

  mux_queue_t queue;

  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  ASSERT_SUCCESS(muxDispatch(queue, signal_buffer, nullptr, nullptr, 0,
                             semaphores.data(), semaphores.size(), nullptr,
                             nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, wait_buffer, nullptr, semaphores.data(),
                             semaphores.size(), nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  muxDestroyCommandBuffer(device, signal_buffer, allocator);

  muxDestroyCommandBuffer(device, wait_buffer, allocator);

  for (auto semaphore : semaphores) {
    muxDestroySemaphore(device, semaphore, allocator);
  }
}

TEST_P(muxCreateSemaphoreTest, 1To1Forward) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));

  mux_queue_t queue;

  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  mux_command_buffer_t buffer_one;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_one));

  mux_command_buffer_t buffer_two;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_two));

  unsigned id = 0;

  // I'm making two pairs, one for each user callback to interact with. Each
  // pair will take the address of ID above, get its value, and increment it.
  // This allows us to test that the ordering of command buffers bound by a
  // wait/signal semaphore is correct.
  typedef std::pair<unsigned *, unsigned> PairType;
  PairType pair_one = std::make_pair(&id, 0);
  PairType pair_two = std::make_pair(&id, 0);

  ASSERT_SUCCESS(muxCommandUserCallback(
      buffer_one,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *pair = static_cast<PairType *>(user_data);
        pair->second = *(pair->first);
        *(pair->first) += 1;
      },
      &pair_one, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxCommandUserCallback(
      buffer_two,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *pair = static_cast<PairType *>(user_data);
        pair->second = *(pair->first);
        *(pair->first) += 1;
      },
      &pair_two, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, buffer_one, nullptr, nullptr, 0, &semaphore,
                             1, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, buffer_two, nullptr, &semaphore, 1, nullptr,
                             0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_EQ(2u, id);

  ASSERT_EQ(0u, pair_one.second);

  ASSERT_EQ(1u, pair_two.second);

  muxDestroyCommandBuffer(device, buffer_two, allocator);
  muxDestroyCommandBuffer(device, buffer_one, allocator);
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxCreateSemaphoreTest, 1To1Backward) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));

  mux_queue_t queue;

  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  mux_command_buffer_t buffer_one;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_one));

  mux_command_buffer_t buffer_two;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_two));

  unsigned id = 0;

  // I'm making two pairs, one for each user callback to interact with. Each
  // pair will take the address of ID above, get its value, and increment it.
  // This allows us to test that the ordering of command buffers bound by a
  // wait/signal semaphore is correct.
  typedef std::pair<unsigned *, unsigned> PairType;
  PairType pair_one = std::make_pair(&id, 0);
  PairType pair_two = std::make_pair(&id, 0);

  ASSERT_SUCCESS(muxCommandUserCallback(
      buffer_one,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *pair = static_cast<PairType *>(user_data);
        pair->second = *(pair->first);
        *(pair->first) += 1;
      },
      &pair_one, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxCommandUserCallback(
      buffer_two,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto *pair = static_cast<PairType *>(user_data);
        pair->second = *(pair->first);
        *(pair->first) += 1;
      },
      &pair_two, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, buffer_one, nullptr, &semaphore, 1, nullptr,
                             0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, buffer_two, nullptr, nullptr, 0, &semaphore,
                             1, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_EQ(2u, id);

  ASSERT_EQ(1u, pair_one.second);

  ASSERT_EQ(0u, pair_two.second);

  muxDestroyCommandBuffer(device, buffer_two, allocator);
  muxDestroyCommandBuffer(device, buffer_one, allocator);
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxCreateSemaphoreTest, 1To1ManyTimes) {
  mux_queue_t queue;

  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  enum { LENGTH = 10000 };

  mux_command_buffer_t buffer[LENGTH];
  mux_semaphore_t semaphore[LENGTH];
  std::pair<unsigned *, unsigned> pairs[LENGTH];
  unsigned id = 0;

  for (unsigned k = 0; k < LENGTH; k++) {
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, buffer + k));

    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, semaphore + k));

    pairs[k] = std::make_pair(&id, 0);

    ASSERT_SUCCESS(muxCommandUserCallback(
        buffer[k],
        [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
          auto *pair =
              static_cast<std::pair<unsigned *, unsigned> *>(user_data);
          pair->second = *(pair->first);
          *(pair->first) += 1;
        },
        pairs + k, 0, nullptr, nullptr));
  }

  for (unsigned k = 0; k < LENGTH; k++) {
    mux_semaphore_t *wait = (0 == k) ? nullptr : semaphore + k;
    const uint64_t wait_length = (0 == k) ? 0 : 1;
    mux_semaphore_t *signal = (LENGTH - 1 == k) ? nullptr : semaphore + k + 1;
    const uint64_t signal_length = (LENGTH - 1 == k) ? 0 : 1;

    ASSERT_SUCCESS(muxDispatch(queue, buffer[k], nullptr, wait, wait_length,
                               signal, signal_length, nullptr, nullptr));
  }

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_EQ(LENGTH, id);

  for (unsigned k = 0; k < LENGTH; k++) {
    ASSERT_EQ(k, pairs[k].second);

    muxDestroySemaphore(device, semaphore[k], allocator);

    muxDestroyCommandBuffer(device, buffer[k], allocator);
  }
}

// Test whether we can submit dispatches out of order provided we set up the
// correct semaphore dependencies between them.
TEST_P(muxCreateSemaphoreTest, OutOfOrderDispatch) {
  //  Create a semaphore that the first enqueued command buffer will wait on and
  //  the second will signal.
  mux_semaphore_t semaphore;
  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));

  // Create a queue into which we we will enqueue our command buffers.
  mux_queue_t queue;
  EXPECT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  // Create the command buffers.
  mux_command_buffer_t buffer_a, buffer_b;
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_a));
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &buffer_b));

  // Create the fence.
  mux_fence_t fence = nullptr;
  EXPECT_SUCCESS(muxCreateFence(device, allocator, &fence));

  // Add a callback that stores the address of the executing command buffer in a
  // container.
  auto command_buffer_call_back = [](mux_queue_t,
                                     mux_command_buffer_t command_buffer,
                                     void *const user_data) {
    auto command_buffers =
        static_cast<std::vector<mux_command_buffer_t> *>(user_data);
    command_buffers->push_back(command_buffer);
  };

  // Storage to be passed to the callbacks and record the actual order in which
  // the mux target executes the command buffers.
  std::vector<mux_command_buffer_t> command_buffers;
  command_buffers.reserve(2);

  // Add the callbacks, one in each command buffer. We will be able to tell
  // which order they executed in based on the order the command buffer
  // addresses appear in the command_buffers vector.
  EXPECT_SUCCESS(muxCommandUserCallback(buffer_b, command_buffer_call_back,
                                        &command_buffers, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxCommandUserCallback(buffer_a, command_buffer_call_back,
                                        &command_buffers, 0, nullptr, nullptr));

  // Dispatch the command buffers out of order, but using the semaphores to
  // enforce an ordering.
  EXPECT_SUCCESS(muxDispatch(queue, buffer_a, fence, &semaphore, 1, nullptr, 0,
                             nullptr, nullptr));
  EXPECT_ERROR_EQ(muxTryWait(queue, 0, fence), mux_fence_not_ready);
  EXPECT_SUCCESS(muxDispatch(queue, buffer_b, nullptr, nullptr, 0, &semaphore,
                             1, nullptr, nullptr));
  // We can just keep waiting on the fence here in a loop; eventually buffer_b
  // will finish then signal buffer_a therefore once buffer_a completes and
  // signals the fence we know buffer_b is also complete.
  mux_result_t mux_error = mux_success;
  while ((mux_error = muxTryWait(queue, 0, fence)) != mux_success) {
    EXPECT_ERROR_EQ(mux_error, mux_fence_not_ready);
  }

  // Check the ordering was correct.
  const std::vector<mux_command_buffer_t> expected_result{buffer_b, buffer_a};
  EXPECT_EQ(command_buffers, expected_result);

  // Clean up the resources.
  muxDestroyFence(device, fence, allocator);
  muxDestroyCommandBuffer(device, buffer_a, allocator);
  muxDestroyCommandBuffer(device, buffer_b, allocator);
  muxDestroySemaphore(device, semaphore, allocator);
}
