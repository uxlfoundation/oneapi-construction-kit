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

enum { MEMORY_SIZE = 128 };

struct muxCommandWriteBufferTest : DeviceTest {
  mux_memory_t memory = nullptr;
  mux_buffer_t buffer = nullptr;
  mux_command_buffer_t command_buffer = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxCreateBuffer(device, MEMORY_SIZE, allocator, &buffer));

    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;
    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer->memory_requirements.supported_heaps);
    ASSERT_SUCCESS(muxAllocateMemory(device, MEMORY_SIZE, heap,
                                     mux_memory_property_host_visible,
                                     allocation_type, 0, allocator, &memory));

    ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer, 0));

    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    if (buffer) {
      muxDestroyBuffer(device, buffer, allocator);
    }
    if (memory) {
      muxFreeMemory(device, memory, allocator);
    }
    DeviceTest::TearDown();
  }
};

TEST_P(muxCommandWriteBufferTest, Default) {
  char data[MEMORY_SIZE] = {0};

  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer, 0, data,
                                       MEMORY_SIZE, 0, nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, WithOffset) {
  char data[MEMORY_SIZE] = {0};

  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer, 1, data, 1, 0,
                                       nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, InvalidOffset) {
  char data[MEMORY_SIZE] = {0};

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandWriteBuffer(command_buffer, buffer, MEMORY_SIZE,
                                        data, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, InvalidOffsetPlusSize) {
  char data[MEMORY_SIZE] = {0};

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandWriteBuffer(command_buffer, buffer, 1, data,
                                        MEMORY_SIZE, 0, nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, InvalidHostPointer) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandWriteBuffer(command_buffer, buffer, 0, 0, 1, 0,
                                        nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, ZeroSize) {
  char data[MEMORY_SIZE] = {0};

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandWriteBuffer(command_buffer, buffer, 0, data, 0, 0,
                                        nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, InvalidSize) {
  char data[MEMORY_SIZE * 2] = {0};

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandWriteBuffer(command_buffer, buffer, 0, data,
                                        MEMORY_SIZE * 2, 0, nullptr, nullptr));
}

TEST_P(muxCommandWriteBufferTest, Sync) {
  char data[MEMORY_SIZE] = {0};

  mux_sync_point_t wait = nullptr;
  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer, 0, data,
                                       MEMORY_SIZE, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer, 0, data,
                                       MEMORY_SIZE, 1, &wait, nullptr));
}
