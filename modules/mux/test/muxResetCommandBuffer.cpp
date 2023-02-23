// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"
#include "mux/mux.h"

struct muxResetCommandBufferTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_queue_t queue = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (device->info->queue_types[mux_queue_type_compute] == 0) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxResetCommandBufferTest);

TEST_P(muxResetCommandBufferTest, Default) {
  bool shouldNotBeHit = false;
  bool shouldBeHit = false;

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        *static_cast<bool *>(user_data) = true;
      },
      &shouldNotBeHit, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxResetCommandBuffer(command_buffer));

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        *static_cast<bool *>(user_data) = true;
      },
      &shouldBeHit, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(muxWaitAll(queue));

  ASSERT_FALSE(shouldNotBeHit);
  ASSERT_TRUE(shouldBeHit);
}
