// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <gtest/gtest.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include "common.h"

enum { MEMORY_SIZE = 128 };

struct muxBindBufferMemoryTest : public DeviceTest {
  mux_memory_t memory = nullptr;

  mux_result_t allocateMemory(uint32_t supported_heaps) {
    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;

    uint32_t heap = mux::findFirstSupportedHeap(supported_heaps);
    return muxAllocateMemory(device, MEMORY_SIZE, heap,
                             mux_memory_property_host_visible, allocation_type,
                             0, allocator, &memory);
  }

  void TearDown() override {
    if (memory) {
      muxFreeMemory(device, memory, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxBindBufferMemoryTest);

TEST_P(muxBindBufferMemoryTest, Default) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  ASSERT_SUCCESS(allocateMemory(buffer->memory_requirements.supported_heaps));

  ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer, 0));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxBindBufferMemoryTest, WithOffset) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  ASSERT_SUCCESS(allocateMemory(buffer->memory_requirements.supported_heaps));

  ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer, MEMORY_SIZE / 2));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxBindBufferMemoryTest, InvalidBufferSize) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, MEMORY_SIZE * 2, allocator, &buffer));
  ASSERT_SUCCESS(allocateMemory(buffer->memory_requirements.supported_heaps));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxBindBufferMemory(device, memory, buffer, 0));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxBindBufferMemoryTest, InvalidBufferSizePlusOffset) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, MEMORY_SIZE - 1, allocator, &buffer));
  ASSERT_SUCCESS(allocateMemory(buffer->memory_requirements.supported_heaps));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxBindBufferMemory(device, memory, buffer, 2));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxBindBufferMemoryTest, InvalidMemory) {
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;

  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, MEMORY_SIZE * 2, allocator, &buffer));

  uint32_t heap =
      mux::findFirstSupportedHeap(buffer->memory_requirements.supported_heaps);
  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(device, MEMORY_SIZE, heap,
                                   mux_memory_property_host_visible,
                                   allocation_type, 0, allocator, &memory));

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxBindBufferMemory(device, memory, buffer, 0));

  muxDestroyBuffer(device, buffer, allocator);

  muxFreeMemory(device, memory, allocator);
}
