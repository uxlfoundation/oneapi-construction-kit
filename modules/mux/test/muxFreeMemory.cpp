// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxFreeMemoryTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxFreeMemoryTest);

TEST_P(muxFreeMemoryTest, AllocHost) {
  mux_memory_t memory;

  ASSERT_SUCCESS(
      muxAllocateMemory(device, 1, 1, mux_memory_property_host_coherent,
                        mux_allocation_type_alloc_host, 0, allocator, &memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxFreeMemoryTest, AllocDevice) {
  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1, mux_memory_property_device_local,
      mux_allocation_type_alloc_device, 0, allocator, &memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxFreeMemoryTest, CreateMemoryFromHost) {
  // muxCreateMemoryFromHost() requires this capability
  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_cached_host)) {
    GTEST_SKIP();
  }

  mux_memory_t memory;
  uint32_t data[4] = {0xA, 0xB, 0xC, 0xD};

  ASSERT_SUCCESS(
      muxCreateMemoryFromHost(device, sizeof(data), data, allocator, &memory));

  muxFreeMemory(device, memory, allocator);

  // Check data not deallocated by `muxFreeMemory()`
  EXPECT_EQ(data[0], 0xA);
  EXPECT_EQ(data[1], 0xB);
  EXPECT_EQ(data[2], 0xC);
  EXPECT_EQ(data[3], 0xD);
}
