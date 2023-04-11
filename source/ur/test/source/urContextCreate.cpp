// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urContextCreateTest = uur::DeviceTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urContextCreateTest);

TEST_P(urContextCreateTest, Success) {
  ur_context_handle_t context = nullptr;
  ASSERT_SUCCESS(urContextCreate(1, &device, nullptr, &context));
  ASSERT_NE(nullptr, context);
  ASSERT_SUCCESS(urContextRelease(context));
}

TEST_P(urContextCreateTest, InvalidNullPointerDevices) {
  ur_context_handle_t context = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urContextCreate(1, nullptr, nullptr, &context));
}

TEST_P(urContextCreateTest, InvalidNullPointerContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urContextCreate(1, &device, nullptr, nullptr));
}
