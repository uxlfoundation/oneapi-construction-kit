// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxCommandUserCallbackTest : DeviceTest {
  mux_command_buffer_t command_buffer = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (device) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandUserCallbackTest);

TEST_P(muxCommandUserCallbackTest, Default) {
  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, nullptr));
}

TEST_P(muxCommandUserCallbackTest, InvalidUserCallback) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandUserCallback(command_buffer, nullptr, nullptr, 0,
                                         nullptr, nullptr));
}

TEST_P(muxCommandUserCallbackTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, 1, &wait, nullptr));
}
