// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
}

TEST_P(muxCreateFenceTest, NullFence) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateFence(device, allocator, nullptr));
}
