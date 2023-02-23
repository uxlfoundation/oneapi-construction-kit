// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
