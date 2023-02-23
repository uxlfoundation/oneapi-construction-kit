// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cstdint>

#include "common.h"
#include "mux/mux.h"

struct muxTryWaitTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_queue_t queue = nullptr;
  mux_fence_t fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (device->info->queue_types[mux_queue_type_compute] == 0) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
      muxDestroyFence(device, fence, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxTryWaitTest);

TEST_P(muxTryWaitTest, Default) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  mux_result_t error = mux_success;

  do {
    error = muxTryWait(queue, 0, fence);
  } while (mux_fence_not_ready == error);

  ASSERT_SUCCESS(error);
}

TEST_P(muxTryWaitTest, Timeout) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  mux_result_t error = mux_success;

  // Since the target implementation of muxTryWait may wait longer than the
  // timeout parameter passed to the API we can't really test the API returns
  // within a given duration. Instead we just test we can successfully pass a
  // reasonable value for the timeout parameter.
  const uint64_t timeout = 1000000;
  do {
    error = muxTryWait(queue, timeout, fence);
  } while (mux_fence_not_ready == error);

  ASSERT_SUCCESS(error);
}

TEST_P(muxTryWaitTest, WaitOnUINT64_MAX) {
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));

  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
}
