// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxCreateBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateBufferTest);

TEST_P(muxCreateBufferTest, Default) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxCreateBufferTest, InvalidSize) {
  mux_buffer_t buffer;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateBuffer(device, 0, allocator, &buffer));
}

TEST_P(muxCreateBufferTest, InvalidOutBuffer) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateBuffer(device, 1, allocator, 0));
}
