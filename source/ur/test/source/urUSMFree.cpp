// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urUSMFreeTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urUSMFreeTest);

TEST_P(urUSMFreeTest, Success) {
  void *ptr{nullptr};
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, nullptr, nullptr,
                                  sizeof(int), 0, &ptr));
  ASSERT_NE(ptr, nullptr);
  // TODO: add USMMemset Operation here to validate data going in and out when
  // the APIs are implemented
  ASSERT_SUCCESS(urUSMFree(context, ptr));
}

TEST_P(urUSMFreeTest, InvalidContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urUSMFree(nullptr, nullptr));
}

TEST_P(urUSMFreeTest, InvalidResultPtr) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urUSMFree(context, nullptr));
}
