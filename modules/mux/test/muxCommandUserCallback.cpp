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

struct muxCommandUserCallbackTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (device) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandUserCallbackTest);

TEST_P(muxCommandUserCallbackTest, Default) {
  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, nullptr));
}

TEST_P(muxCommandUserCallbackTest, InvalidUserCallback) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandUserCallback(command_buffer, nullptr, nullptr, 0,
                                         nullptr, nullptr));
}

TEST_P(muxCommandUserCallbackTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 1, &wait, nullptr));
}
