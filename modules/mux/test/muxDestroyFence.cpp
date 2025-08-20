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

using muxDestroyFenceTest = DeviceTest;

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyFenceTest);

TEST_P(muxDestroyFenceTest, Default) {
  mux_fence_t fence;

  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  muxDestroyFence(device, fence, allocator);
}

TEST_P(muxDestroyFenceTest, InvalidDevice) {
  mux_fence_t fence;

  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  muxDestroyFence(nullptr, fence, allocator);
  // Actually destroy the fence.
  muxDestroyFence(device, fence, allocator);
}

TEST_P(muxDestroyFenceTest, InvalidFence) {
  muxDestroyFence(device, nullptr, allocator);
}

TEST_P(muxDestroyFenceTest, InvalidAllocator) {
  mux_fence_t fence;

  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));

  allocator.alloc = nullptr;
  allocator.free = nullptr;
  muxDestroyFence(device, fence, allocator);

  allocator.alloc = mux::alloc;
  allocator.free = nullptr;
  muxDestroyFence(device, fence, allocator);

  allocator.alloc = nullptr;
  allocator.free = mux::free;
  muxDestroyFence(device, fence, allocator);

  // Actually destroy the fence.
  allocator.alloc = mux::alloc;
  allocator.free = mux::free;
  muxDestroyFence(device, fence, allocator);
}
