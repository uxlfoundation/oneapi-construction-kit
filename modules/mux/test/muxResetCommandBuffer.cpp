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

#include "common.h"
#include "mux/mux.h"

struct muxResetCommandBufferTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_queue_t queue = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (device->info->queue_types[mux_queue_type_compute] == 0) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxResetCommandBufferTest);

TEST_P(muxResetCommandBufferTest, Default) {
  bool shouldNotBeHit = false;
  bool shouldBeHit = false;

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const UserData) {
        *static_cast<bool *>(UserData) = true;
      },
      &shouldNotBeHit, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxResetCommandBuffer(command_buffer));

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const UserData) {
        *static_cast<bool *>(UserData) = true;
      },
      &shouldBeHit, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_FALSE(shouldNotBeHit);
  ASSERT_TRUE(shouldBeHit);
}
