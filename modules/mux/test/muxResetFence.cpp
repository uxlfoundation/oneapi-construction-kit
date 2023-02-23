// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

using muxResetFenceTest = DeviceTest;

TEST_P(muxResetFenceTest, Default) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  EXPECT_SUCCESS(muxResetSemaphore(semaphore));
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxResetFenceTest, InvalidSemaphore) {
  ASSERT_ERROR_EQ(mux_error_invalid_value, muxResetSemaphore(nullptr));
}
