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

#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include "common.h"

using muxCreateFenceTest = DeviceTest;

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateFenceTest);

TEST_P(muxCreateFenceTest, Default) {
  mux_fence_t fence;

  ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  muxDestroyFence(device, fence, allocator);
}

TEST_P(muxCreateFenceTest, InvalidDevice) {
  mux_fence_t fence;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateFence(nullptr, allocator, &fence));
}

TEST_P(muxCreateFenceTest, InvalidAllocator) {
  mux_fence_t fence;

  const mux_allocator_info_t saved_allocator = allocator;

  allocator.alloc = nullptr;
  allocator.free = nullptr;
  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateFence(device, allocator, &fence));

  allocator.alloc = nullptr;
  allocator.free = mux::free;
  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateFence(device, allocator, &fence));

  allocator.alloc = mux::alloc;
  allocator.free = nullptr;
  ASSERT_ERROR_EQ(mux_error_null_allocator_callback,
                  muxCreateFence(device, allocator, &fence));

  // Restore allocator to properly tear down test
  allocator = saved_allocator;
}

TEST_P(muxCreateFenceTest, NullFence) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateFence(device, allocator, nullptr));
}
