// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxUnmapMemoryTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxUnmapMemoryTest);

TEST_P(muxUnmapMemoryTest, Default) {
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(device, 1, 1,
                                   mux_memory_property_host_visible,
                                   allocation_type, 0, allocator, &memory));

  void *data;

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, &data));

  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}
