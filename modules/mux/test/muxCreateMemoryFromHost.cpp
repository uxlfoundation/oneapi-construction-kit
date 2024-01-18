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

#include "common.h"

struct muxCreateMemoryFromHostTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateMemoryFromHostTest);

TEST_P(muxCreateMemoryFromHostTest, Default) {
  mux_memory_t memory;
  uint32_t data[1];

  if (!(device->info->allocation_capabilities &
        mux_allocation_capabilities_coherent_host)) {
    ASSERT_ERROR_EQ(mux_error_feature_unsupported,
                    muxCreateMemoryFromHost(device, sizeof(data), data,
                                            allocator, &memory));
  } else {
    ASSERT_SUCCESS(muxCreateMemoryFromHost(device, sizeof(data), data,
                                           allocator, &memory));

    // Properties` member of `memory` should be set to
    // `mux_memory_property_host_visible` and
    // `mux_memory_property_host_coherent`
    ASSERT_TRUE(memory->properties & mux_memory_property_host_visible);
    ASSERT_TRUE((memory->properties & mux_memory_property_host_coherent));
    ASSERT_TRUE(0 == (memory->properties & mux_memory_property_host_cached));
    ASSERT_TRUE(0 == (memory->properties & mux_memory_property_device_local));

    ASSERT_EQ(memory->size, sizeof(data));
    ASSERT_TRUE(memory->handle != 0);

    muxFreeMemory(device, memory, allocator);
  }
}

TEST_P(muxCreateMemoryFromHostTest, NullDevice) {
  mux_memory_t memory;
  uint32_t data[1];

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateMemoryFromHost(NULL, sizeof(data), data, allocator, &memory));
}

TEST_P(muxCreateMemoryFromHostTest, NullHostPointer) {
  mux_memory_t memory;
  uint32_t data[1];

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateMemoryFromHost(device, sizeof(data), NULL, allocator, &memory));
}

TEST_P(muxCreateMemoryFromHostTest, ZeroSize) {
  mux_memory_t memory;
  uint32_t data[1];

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateMemoryFromHost(device, 0, data, allocator, &memory));
}

TEST_P(muxCreateMemoryFromHostTest, BadAllocator) {
  mux_memory_t memory;
  uint32_t data[1];

  const mux_allocator_info_t bad_allocator = {nullptr, nullptr, nullptr};

  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateMemoryFromHost(device, sizeof(data), data,
                                          bad_allocator, &memory));
}

TEST_P(muxCreateMemoryFromHostTest, NullOutMemory) {
  uint32_t data[1];

  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxCreateMemoryFromHost(device, sizeof(data), data, allocator, 0));
}
