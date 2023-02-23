// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxCreateCommandBufferTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateCommandBufferTest);

TEST_P(muxCreateCommandBufferTest, Default) {
  mux_command_buffer_t command_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));

  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

TEST_P(muxCreateCommandBufferTest, NullOutBuffer) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateCommandBuffer(device, callback, allocator, 0));
}
