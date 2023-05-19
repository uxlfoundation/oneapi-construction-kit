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
#include <cstring>

#include "common.h"
#include "mux/mux.h"

struct muxFlushMappedMemoryToDeviceTest : DeviceTest {
  mux_memory_t memory = nullptr;
  void *host_pointer = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxAllocateMemory(
        device, 128, 1, mux_memory_property_host_cached,
        mux_allocation_type_alloc_device, 0, allocator, &memory));
    ASSERT_SUCCESS(
        muxMapMemory(device, memory, 0, memory->size, &host_pointer));
  }

  void TearDown() override {
    if (device) {
      EXPECT_SUCCESS(muxUnmapMemory(device, memory));
      muxFreeMemory(device, memory, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxFlushMappedMemoryToDeviceTest);

TEST_P(muxFlushMappedMemoryToDeviceTest, Default) {
  ASSERT_SUCCESS(muxFlushMappedMemoryToDevice(device, memory, 0, memory->size));
}

TEST_P(muxFlushMappedMemoryToDeviceTest, InvalidMemory) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxFlushMappedMemoryToDevice(device, nullptr, 0, 128));
  mux_memory_s invalid_memory = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxFlushMappedMemoryToDevice(device, &invalid_memory, 0, 128));
}

TEST_P(muxFlushMappedMemoryToDeviceTest, InvalidOffset) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxFlushMappedMemoryToDevice(device, memory, 1, memory->size));
}

TEST_P(muxFlushMappedMemoryToDeviceTest, InvalidSize) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxFlushMappedMemoryToDevice(device, memory, 0, memory->size + 1));
}

TEST_P(muxFlushMappedMemoryToDeviceTest, CreateMemoryFromHost) {
  // muxCreateMemoryFromHost() requires this capability
  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_cached_host)) {
    GTEST_SKIP();
  }

  // Create memory from host and flush to device
  mux_memory_t host_memory;
  uint32_t data[4] = {42, 42, 42, 42};
  ASSERT_SUCCESS(muxCreateMemoryFromHost(device, sizeof(data), data, allocator,
                                         &host_memory));
  ASSERT_SUCCESS(
      muxFlushMappedMemoryToDevice(device, host_memory, 0, host_memory->size));

  // Create device buffer bound to host memory
  mux_buffer_t buffer;
  ASSERT_SUCCESS(muxCreateBuffer(device, sizeof(data), allocator, &buffer));
  ASSERT_SUCCESS(muxBindBufferMemory(device, host_memory, buffer, 0));

  // Create a command group to push write command onto
  mux_command_buffer_t command_buffer;
  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));

  // Read complete memory allocation into buffer.
  uint32_t read_data[4] = {0, 0, 0, 0};
  ASSERT_SUCCESS(muxCommandReadBuffer(command_buffer, buffer, 0, read_data,
                                      sizeof(read_data), 0, nullptr, nullptr));

  // Create a queue, dispatch command, and wait for it to complete
  mux_queue_t queue;
  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
  mux_fence_t fence = nullptr;
  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));
  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));

  // Verify we could read back flushed data
  ASSERT_TRUE(0 == std::memcmp(data, read_data, sizeof(data)));

  // Tidy up
  muxDestroyFence(device, fence, allocator);
  muxDestroyCommandBuffer(device, command_buffer, allocator);
  muxDestroyBuffer(device, buffer, allocator);
  muxFreeMemory(device, host_memory, allocator);
}
