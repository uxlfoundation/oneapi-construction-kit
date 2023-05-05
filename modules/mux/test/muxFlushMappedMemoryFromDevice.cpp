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

struct muxFlushMappedMemoryFromDeviceTest : DeviceTest {
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

INSTANTIATE_DEVICE_TEST_SUITE_P(muxFlushMappedMemoryFromDeviceTest);

TEST_P(muxFlushMappedMemoryFromDeviceTest, Default) {
  ASSERT_SUCCESS(
      muxFlushMappedMemoryFromDevice(device, memory, 0, memory->size));
}

TEST_P(muxFlushMappedMemoryFromDeviceTest, InvalidMemory) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxFlushMappedMemoryFromDevice(device, nullptr, 0, 128));
}

TEST_P(muxFlushMappedMemoryFromDeviceTest, InvalidOffset) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxFlushMappedMemoryFromDevice(device, memory, 1, memory->size));
}

TEST_P(muxFlushMappedMemoryFromDeviceTest, InvalidSize) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxFlushMappedMemoryFromDevice(device, memory, 0, memory->size + 1));
}

TEST_P(muxFlushMappedMemoryFromDeviceTest, CreateMemoryFromHost) {
  // muxCreateMemoryFromHost() requires this capability
  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_cached_host)) {
    GTEST_SKIP();
  }

  // Create memory from host
  mux_memory_t host_memory;
  uint32_t data[4] = {42, 42, 42, 42};
  ASSERT_SUCCESS(muxCreateMemoryFromHost(device, sizeof(data), data, allocator,
                                         &host_memory));

  // Create device buffer bound to host memory
  mux_buffer_t buffer;
  ASSERT_SUCCESS(muxCreateBuffer(device, sizeof(data), allocator, &buffer));
  ASSERT_SUCCESS(muxBindBufferMemory(device, host_memory, buffer, 0));

  // Create a command group to push write command onto
  mux_command_buffer_t command_buffer;
  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));

  // Overwrite whole memory allocation by writing 4 uint32_t to buffer.
  uint32_t write_data[4] = {0xA, 0xB, 0xC, 0xD};
  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer, 0, write_data,
                                       sizeof(write_data), 0, nullptr,
                                       nullptr));

  // Create a queue, dispatch command, and wait for it to complete
  mux_queue_t queue;
  ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
  mux_fence_t fence = nullptr;
  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));
  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));

  // Flush written data back from device and validate that the host
  // pointer used to create allocation was updated.
  ASSERT_SUCCESS(muxFlushMappedMemoryFromDevice(device, host_memory, 0,
                                                host_memory->size));
  ASSERT_TRUE(0 == std::memcmp(data, write_data, sizeof(data)));

  // Tidy up
  muxDestroyFence(device, fence, allocator);
  muxDestroyCommandBuffer(device, command_buffer, allocator);
  muxDestroyBuffer(device, buffer, allocator);
  muxFreeMemory(device, host_memory, allocator);
}
