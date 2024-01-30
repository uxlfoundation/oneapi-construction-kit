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

#include "common.h"

enum { BUFFER_SIZE = 128, MEMORY_SIZE = 2 * BUFFER_SIZE };

struct muxCommandCopyBufferTest : DeviceTest {
  mux_memory_t memory = nullptr;
  mux_buffer_t src_buffer = nullptr;
  mux_buffer_t dst_buffer = nullptr;
  mux_command_buffer_t command_buffer = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    ASSERT_SUCCESS(
        muxCreateBuffer(device, BUFFER_SIZE, allocator, &src_buffer));

    ASSERT_SUCCESS(
        muxCreateBuffer(device, BUFFER_SIZE, allocator, &dst_buffer));

    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;

    const uint32_t heap = mux::findFirstSupportedHeap(
        src_buffer->memory_requirements.supported_heaps);

    ASSERT_SUCCESS(muxAllocateMemory(device, MEMORY_SIZE, heap,
                                     mux_memory_property_host_visible,
                                     allocation_type, 0, allocator, &memory));

    ASSERT_SUCCESS(muxBindBufferMemory(device, memory, dst_buffer, 0));

    ASSERT_SUCCESS(
        muxBindBufferMemory(device, memory, src_buffer, BUFFER_SIZE));

    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (src_buffer) {
      muxDestroyBuffer(device, src_buffer, allocator);
    }
    if (dst_buffer) {
      muxDestroyBuffer(device, dst_buffer, allocator);
    }
    if (command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    if (memory) {
      muxFreeMemory(device, memory, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandCopyBufferTest);

TEST_P(muxCommandCopyBufferTest, Default) {
  ASSERT_SUCCESS(muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer,
                                      0, BUFFER_SIZE, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, WithOffset) {
  ASSERT_SUCCESS(muxCommandCopyBuffer(command_buffer, src_buffer, 1, dst_buffer,
                                      1, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, BadSrcOffset) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandCopyBuffer(command_buffer, src_buffer, BUFFER_SIZE,
                                       dst_buffer, 0, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, BadSrcOffsetPlusSize) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandCopyBuffer(command_buffer, src_buffer, 1, dst_buffer, 0,
                           BUFFER_SIZE, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, BadDstOffset) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer,
                           BUFFER_SIZE, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, BadDstOffsetPlusSize) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer, 1,
                           BUFFER_SIZE, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, ZeroSize) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandCopyBuffer(command_buffer, src_buffer, 0,
                                       dst_buffer, 0, 0, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, BadSize) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer, 0,
                           BUFFER_SIZE + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandCopyBufferTest, Sync) {
  mux_sync_point_t wait = nullptr;
  ASSERT_SUCCESS(muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer,
                                      0, BUFFER_SIZE, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandCopyBuffer(command_buffer, src_buffer, 0, dst_buffer,
                                      0, BUFFER_SIZE, 1, &wait, nullptr));
}
