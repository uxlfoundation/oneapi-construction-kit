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

#include <cstdint>

#include "common.h"
#include "mux/mux.h"

struct muxTryWaitTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_queue_t queue = nullptr;
  mux_fence_t fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (device->info->queue_types[mux_queue_type_compute] == 0) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
      muxDestroyFence(device, fence, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxTryWaitTest);

TEST_P(muxTryWaitTest, Default) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  mux_result_t error = mux_success;

  do {
    error = muxTryWait(queue, 0, fence);
  } while (mux_fence_not_ready == error);

  ASSERT_SUCCESS(error);
}

TEST_P(muxTryWaitTest, Timeout) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  mux_result_t error = mux_success;

  // Since the target implementation of muxTryWait may wait longer than the
  // timeout parameter passed to the API we can't really test the API returns
  // within a given duration. Instead we just test we can successfully pass a
  // reasonable value for the timeout parameter.
  const uint64_t timeout = 1000000;
  do {
    error = muxTryWait(queue, timeout, fence);
  } while (mux_fence_not_ready == error);

  ASSERT_SUCCESS(error);
}

TEST_P(muxTryWaitTest, WaitOnUINT64_MAX) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
}
