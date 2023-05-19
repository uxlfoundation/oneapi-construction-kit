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

#include <gtest/gtest.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include "common.h"

struct muxFinalizeCommandBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxFinalizeCommandBufferTest);

// Tests that muxFinalizeCommandBuffer returns the correct error when passed a
// NULL command group.
TEST_P(muxFinalizeCommandBufferTest, FinalizeNullComandGroup) {
  mux_command_buffer_t command_buffer = nullptr;
  EXPECT_EQ(mux_error_null_out_parameter,
            muxFinalizeCommandBuffer(command_buffer));
}

// Tests that muxFinalizeCommandBuffer can handle empty command groups.
TEST_P(muxFinalizeCommandBufferTest, FinalizeEmptyCommandBuffer) {
  mux_command_buffer_t command_buffer;
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

// Tests that muxFinalizeCommandBuffer can handle non-empty command groups.
TEST_P(muxFinalizeCommandBufferTest, FinalizeCommandBuffer) {
  mux_command_buffer_t command_buffer;
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));
  EXPECT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

// Tests that muxFinalizeCommandBuffer can handle finalizing command groups
// twice.
TEST_P(muxFinalizeCommandBufferTest, FinalizeCommandBufferTwice) {
  mux_command_buffer_t command_buffer;
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));
  EXPECT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

// Tests that finalized command groups can be reset and then have new commands
// pushed to them and be refinalized.
TEST_P(muxFinalizeCommandBufferTest, FinalizeResetFinalize) {
  // Two flags we will use to check the command buffer was reset and refinalized
  // correctly.
  bool flag_a = false;
  bool flag_b = false;

  // Callback to turn on one of the flags.
  auto callback = [](mux_queue_t, mux_command_buffer_t, void *const data) {
    auto flag = static_cast<bool *>(data);
    *flag = true;
  };

  // Create a command group.
  mux_command_buffer_t command_buffer;
  EXPECT_SUCCESS(
      muxCreateCommandBuffer(device, nullptr, allocator, &command_buffer));

  // Enqueue a callback to update the first flag.
  EXPECT_SUCCESS(muxCommandUserCallback(command_buffer, callback, &flag_a, 0,
                                        nullptr, nullptr));

  // Finalize the command group.
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));

  // Create a queue into which we we will enqueue our command buffers.
  mux_queue_t queue;
  EXPECT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

  // Dispatch the command buffer and wait for it to finish.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check our callback got executed correctly.
  EXPECT_TRUE(flag_a);
  EXPECT_FALSE(flag_b);

  // Reset the flags;
  flag_a = false;
  flag_b = false;

  // Reset the command group.
  EXPECT_SUCCESS(muxResetCommandBuffer(command_buffer));

  // Enqueue a callback to update the second flag.
  EXPECT_SUCCESS(muxCommandUserCallback(command_buffer, callback, &flag_b, 0,
                                        nullptr, nullptr));

  // Re-finalize the command group.
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));

  // Dispatch the command buffer a second time and wait for it to finish.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check our callback got executed correctly.
  EXPECT_FALSE(flag_a);
  EXPECT_TRUE(flag_b);

  // Cleanup.
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}
