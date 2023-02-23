// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urKernelReleaseTest = uur::KernelTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urKernelReleaseTest);

TEST_P(urKernelReleaseTest, Success) {
  ASSERT_SUCCESS(urKernelRetain(kernel));
  EXPECT_SUCCESS(urKernelRelease(kernel));
}

TEST_P(urKernelReleaseTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urKernelRelease(nullptr));
}
