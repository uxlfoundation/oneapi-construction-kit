// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxDestroyCommandBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyCommandBufferTest);

TEST_P(muxDestroyCommandBufferTest, Default) {
  mux_command_buffer_t command_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

TEST_P(muxDestroyCommandBufferTest, InvalidDevice) {
  mux_command_buffer_t command_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  muxDestroyCommandBuffer(nullptr, command_buffer, allocator);
  // Actually destroy the command buffer.
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

TEST_P(muxDestroyCommandBufferTest, InvalidCommandBuffer) {
  muxDestroyCommandBuffer(device, nullptr, allocator);
}

TEST_P(muxDestroyCommandBufferTest, InvalidAllocator) {
  mux_command_buffer_t command_buffer;

  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  muxDestroyCommandBuffer(device, command_buffer, {nullptr, nullptr, nullptr});
  // Actually destroy the command buffer.
  muxDestroyCommandBuffer(device, command_buffer, allocator);
}
