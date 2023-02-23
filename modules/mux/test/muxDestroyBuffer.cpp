// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxDestroyBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyBufferTest);

TEST_P(muxDestroyBufferTest, Default) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidDevice) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(nullptr, buffer, allocator);
  // Actually destroy the buffer.
  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidBuffer) {
  muxDestroyBuffer(device, nullptr, allocator);
}

TEST_P(muxDestroyBufferTest, InvalidAllocator) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));
  muxDestroyBuffer(device, buffer, {nullptr, nullptr, nullptr});
  // Actually destroy the buffer.
  muxDestroyBuffer(device, buffer, allocator);
}
