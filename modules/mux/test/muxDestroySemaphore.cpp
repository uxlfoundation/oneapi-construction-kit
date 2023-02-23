// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxDestroySemaphoreTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroySemaphoreTest);

TEST_P(muxDestroySemaphoreTest, Default) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidDevice) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(nullptr, semaphore, allocator);
  // Actually destroy the semaphore.
  muxDestroySemaphore(device, semaphore, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidSemaphore) {
  muxDestroySemaphore(device, nullptr, allocator);
}

TEST_P(muxDestroySemaphoreTest, InvalidAllocator) {
  mux_semaphore_t semaphore;

  ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  muxDestroySemaphore(device, semaphore, {nullptr, nullptr, nullptr});
  // Actually destroy the semaphore.
  muxDestroySemaphore(device, semaphore, allocator);
}
