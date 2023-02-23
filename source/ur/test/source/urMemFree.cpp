// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urMemFreeTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urMemFreeTest);

TEST_P(urMemFreeTest, Success) {
  void *ptr{nullptr};
  ur_usm_mem_flags_t flags;
  ASSERT_SUCCESS(
      urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0, &ptr));
  ASSERT_NE(ptr, nullptr);
  // TODO: add USMMemset Operation here to validate data going in and out when
  // the APIs are implemented
  ASSERT_SUCCESS(urMemFree(context, ptr));
}

TEST_P(urMemFreeTest, InvalidContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urMemFree(nullptr, nullptr));
}

TEST_P(urMemFreeTest, InvalidResultPtr) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urMemFree(context, nullptr));
}
