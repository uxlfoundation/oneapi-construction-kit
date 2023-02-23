// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxAllocateMemoryTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxAllocateMemoryTest);

TEST_P(muxAllocateMemoryTest, AllocCoherentHost) {
  mux_memory_t memory;

  ASSERT_SUCCESS(
      muxAllocateMemory(device, 1, 1, mux_memory_property_host_coherent,
                        mux_allocation_type_alloc_host, 0, allocator, &memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxAllocateMemoryTest, AllocCachedHost) {
  mux_memory_t memory;

  ASSERT_SUCCESS(
      muxAllocateMemory(device, 1, 1, mux_memory_property_host_cached,
                        mux_allocation_type_alloc_host, 0, allocator, &memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxAllocateMemoryTest, AllocDevice) {
  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1, mux_memory_property_device_local,
      mux_allocation_type_alloc_device, 0, allocator, &memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxAllocateMemoryTest, InvalidMemoryType) {
  mux_memory_t memory;

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxAllocateMemory(device, 1, 1, 0, mux_allocation_type_alloc_host, 0,
                        allocator, &memory));
}

TEST_P(muxAllocateMemoryTest, NullOutMemory) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxAllocateMemory(device, 1, 1, mux_memory_property_device_local,
                        mux_allocation_type_alloc_device, 0, allocator, 0));
}

TEST_P(muxAllocateMemoryTest, InvalidAlignment) {
  mux_memory_t memory;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxAllocateMemory(
                      device, 1, 1, mux_memory_property_device_local,
                      mux_allocation_type_alloc_device, 6, allocator, &memory));
}

TEST_P(muxAllocateMemoryTest, AllocHostAlignment) {
  mux_memory_t memory;

  int property = mux_memory_property_host_visible;

  if (device->info->allocation_capabilities &
      mux_allocation_capabilities_coherent_host) {
    property |= mux_memory_property_host_coherent;
  } else if (device->info->allocation_capabilities &
             mux_allocation_capabilities_cached_host) {
    property |= mux_memory_property_host_cached;
  } else {
    GTEST_SKIP();
  }

  const std::array<uint32_t, 5> alignments = {16, 32, 64, 128, 256};
  const size_t size = 4;  // Smaller than desired alignment
  for (uint32_t align : alignments) {
    ASSERT_SUCCESS(muxAllocateMemory(device, size, 1, property,
                                     mux_allocation_type_alloc_host, align,
                                     allocator, &memory));

    const uint64_t address = memory->handle;
    EXPECT_TRUE((address % align) == 0)
        << "For alignment " << align << " at address 0x" << std::hex << address
        << std::dec;

    muxFreeMemory(device, memory, allocator);
  }
}

TEST_P(muxAllocateMemoryTest, DeviceAlignment) {
  mux_memory_t memory;

  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_alloc_device)) {
    GTEST_SKIP();
  }

  const std::array<uint32_t, 5> alignments = {16, 32, 64, 128, 256};
  const size_t size = 4;  // Smaller than desired alignment
  for (uint32_t align : alignments) {
    ASSERT_SUCCESS(muxAllocateMemory(
        device, size, 1, mux_memory_property_host_visible,
        mux_allocation_type_alloc_device, align, allocator, &memory));

    const uint64_t address = memory->handle;
    EXPECT_TRUE((address % align) == 0)
        << "For alignment " << align << " at address 0x" << std::hex << address
        << std::dec;

    muxFreeMemory(device, memory, allocator);
  }
}
