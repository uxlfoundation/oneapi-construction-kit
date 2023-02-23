// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urKernelRetainTest = uur::KernelTest;

TEST_P(urKernelRetainTest, Success) {
  ASSERT_SUCCESS(urKernelRetain(kernel));
  EXPECT_SUCCESS(urKernelRelease(kernel));
}

TEST_P(urKernelRetainTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urKernelRetain(nullptr));
}
