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

struct muxDestroySemaphoreTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroySemaphoreTest);

TEST_P(muxDestroySemaphoreTest, Default) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidDevice) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(nullptr, semaphore, allocator);
  // Actually destroy the semaphore.
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidSemaphore) {
  muxDestroySemaphore(device, nullptr, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidAllocator) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(device, semaphore, {nullptr, nullptr, nullptr});
  // Actually destroy the semaphore.
  muxDestroySemaphore(device, semaphore, allocator);
}
