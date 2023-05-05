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

struct muxDestroyBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyBufferTest);

TEST_P(muxDestroyBufferTest, Default) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidDevice) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(nullptr, buffer, allocator);
  // Actually destroy the buffer.
  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidBuffer) {
  muxDestroyBuffer(device, nullptr, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidAllocator) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(device, buffer, {nullptr, nullptr, nullptr});
  // Actually destroy the buffer.
  muxDestroyBuffer(device, buffer, allocator);
}
