// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "common.h"

struct muxMapMemoryTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxMapMemoryTest);

TEST_P(muxMapMemoryTest, MapCoherentHost) {
  if (!(mux_allocation_capabilities_coherent_host &
        device->info->allocation_capabilities)) {
    GTEST_SKIP();
  }

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1,
      mux_memory_property_host_visible | mux_memory_property_host_coherent,
      mux_allocation_type_alloc_host, 0, allocator, &memory));

  char *data;

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));

  *data = 42;

  ASSERT_SUCCESS(muxFlushMappedMemoryToDevice(device, memory, 0, memory->size));
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));
  ASSERT_SUCCESS(
      muxFlushMappedMemoryFromDevice(device, memory, 0, memory->size));

  ASSERT_EQ(42, *data);

  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, MapCachedHost) {
  if (!(mux_allocation_capabilities_cached_host &
        device->info->allocation_capabilities)) {
    GTEST_SKIP();
  }

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1,
      mux_memory_property_host_visible | mux_memory_property_host_cached,
      mux_allocation_type_alloc_host, 0, allocator, &memory));

  char *data;

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));

  *data = 42;

  ASSERT_SUCCESS(muxFlushMappedMemoryToDevice(device, memory, 0, memory->size));
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));
  ASSERT_SUCCESS(
      muxFlushMappedMemoryFromDevice(device, memory, 0, memory->size));

  ASSERT_EQ(42, *data);

  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, MapAllocDevice) {
  if (!(mux_allocation_capabilities_alloc_device &
        device->info->allocation_capabilities)) {
    GTEST_SKIP();
  }

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1, mux_memory_property_host_visible,
      mux_allocation_type_alloc_device, 0, allocator, &memory));

  char *data;

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));

  *data = 42;

  ASSERT_SUCCESS(muxFlushMappedMemoryToDevice(device, memory, 0, memory->size));
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 1, (void **)&data));
  ASSERT_SUCCESS(
      muxFlushMappedMemoryFromDevice(device, memory, 0, memory->size));

  ASSERT_EQ(42, *data);

  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, MapWithOffset) {
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(device, 3, 1,
                                   mux_memory_property_host_visible,
                                   allocation_type, 0, allocator, &memory));

  char *data;

  ASSERT_SUCCESS(muxMapMemory(device, memory, 0, 3, (void **)&data));

  data[0] = 13;
  data[1] = 42;
  data[2] = 67;

  ASSERT_SUCCESS(muxFlushMappedMemoryToDevice(device, memory, 0, memory->size));
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  ASSERT_SUCCESS(muxMapMemory(device, memory, 1, 1, (void **)&data));
  ASSERT_SUCCESS(muxFlushMappedMemoryFromDevice(device, memory, 1, 1));

  ASSERT_EQ(42, *data);

  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, InvalidOffset) {
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

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxMapMemory(device, memory, 1, 1, &data));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, InvalidSizeToLarge) {
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

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxMapMemory(device, memory, 0, 2, &data));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, InvalidSizePlusOffset) {
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(device, 4, 1,
                                   mux_memory_property_host_visible,
                                   allocation_type, 0, allocator, &memory));

  void *data;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxMapMemory(device, memory, 2, 3, &data));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, InvalidSize) {
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

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxMapMemory(device, memory, 0, 0, &data));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, NullOutData) {
  const mux_allocation_type_e allocation_type =
      (mux_allocation_capabilities_alloc_device &
       device->info->allocation_capabilities)
          ? mux_allocation_type_alloc_device
          : mux_allocation_type_alloc_host;

  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(device, 1, 1,
                                   mux_memory_property_host_visible,
                                   allocation_type, 0, allocator, &memory));

  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxMapMemory(device, memory, 0, 1, nullptr));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, MapCreateMemFromHost) {
  // muxCreateMemoryFromHost() requires this capability
  if (!(mux_allocation_capabilities_cached_host &
        device->info->allocation_capabilities)) {
    GTEST_SKIP();
  }

  mux_memory_t memory;
  uint32_t data[4] = {0xA, 0xB, 0xC, 0xD};

  ASSERT_SUCCESS(
      muxCreateMemoryFromHost(device, sizeof(data), data, allocator, &memory));
  EXPECT_EQ(reinterpret_cast<uintptr_t>(data), memory->handle);

  // Map whole memory allocation
  uint32_t *mapped_ptr;
  ASSERT_SUCCESS(
      muxMapMemory(device, memory, 0, sizeof(data), (void **)&mapped_ptr));

  EXPECT_EQ(mapped_ptr, data);
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  // Map single uint32_t element from an offset
  ASSERT_SUCCESS(muxMapMemory(device, memory, sizeof(uint32_t) * 2,
                              sizeof(uint32_t), (void **)&mapped_ptr));
  EXPECT_EQ(mapped_ptr, &data[2]);
  ASSERT_SUCCESS(muxUnmapMemory(device, memory));

  muxFreeMemory(device, memory, allocator);
}

TEST_P(muxMapMemoryTest, InvalidMemoryProperty) {
  mux_memory_t memory;

  ASSERT_SUCCESS(muxAllocateMemory(
      device, 1, 1, mux_memory_property_device_local,
      mux_allocation_type_alloc_device, 0, allocator, &memory));

  // Mapping a memory object with the `mux_memory_property_device_local`
  // property is defined as `mux_error_invalid_value`.
  void *data;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxMapMemory(device, memory, 0, 1, (void **)&data));

  muxFreeMemory(device, memory, allocator);
}
