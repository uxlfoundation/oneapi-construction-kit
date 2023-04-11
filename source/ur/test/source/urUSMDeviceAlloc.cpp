// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urUSMDeviceAllocTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urUSMDeviceAllocTest);

TEST_P(urUSMDeviceAllocTest, Success) {
  void *ptr{nullptr};
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0, &ptr));
  ASSERT_NE(ptr, nullptr);

  ur_event_handle_t event = nullptr;
  int zero_val = 0;
  ASSERT_SUCCESS(urEnqueueUSMFill(queue, ptr, sizeof(zero_val), &zero_val,
                                  sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  // TODO Make sure memset on device happened.

  ASSERT_SUCCESS(urUSMFree(context, ptr));
  EXPECT_SUCCESS(urEventRelease(event));
}

TEST_P(urUSMDeviceAllocTest, InvalidNullContext) {
  void *ptr{nullptr};
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urUSMDeviceAlloc(nullptr, device, nullptr, nullptr,
                                    sizeof(int), 0, &ptr));
}

TEST_P(urUSMDeviceAllocTest, InvalidDevice) {
  void *ptr{nullptr};
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_DEVICE,
                   urUSMDeviceAlloc(context, nullptr, nullptr, nullptr,
                                    sizeof(int), 0, &ptr));
}

TEST_P(urUSMDeviceAllocTest, InvalidNullPtrResult) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                    sizeof(int), 0, nullptr));
}
