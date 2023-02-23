// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urQueueCreateTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urQueueCreateTest);

TEST_P(urQueueCreateTest, Success) {
  ur_queue_handle_t queue = nullptr;
  ASSERT_SUCCESS(urQueueCreate(context, device, nullptr, &queue));
  ASSERT_NE(nullptr, queue);
  ASSERT_SUCCESS(urQueueRelease(queue));
}

TEST_P(urQueueCreateTest, InvalidNullHandleContext) {
  ur_queue_handle_t queue = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urQueueCreate(nullptr, device, nullptr, &queue));
}

TEST_P(urQueueCreateTest, InvalidNullHandleDevice) {
  ur_queue_handle_t queue = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urQueueCreate(context, nullptr, nullptr, &queue));
}

TEST_P(urQueueCreateTest, InvalidEnumerationProps) {
  ur_queue_handle_t queue = nullptr;
  const ur_queue_property_t property_list[] = {UR_QUEUE_PROPERTIES_FLAGS,
                                               UR_QUEUE_FLAG_FORCE_UINT32, 0};
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                   urQueueCreate(context, device, property_list, &queue));
}

TEST_P(urQueueCreateTest, InvalidNullPointerQueue) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urQueueCreate(context, device, nullptr, nullptr));
}
