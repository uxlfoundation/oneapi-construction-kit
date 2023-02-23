// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urModuleReleaseTest = uur::ModuleTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urModuleReleaseTest);

TEST_P(urModuleReleaseTest, Success) {
  ASSERT_SUCCESS(urModuleRetain(module));
  EXPECT_SUCCESS(urModuleRelease(module));
}

TEST_P(urModuleReleaseTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urModuleRelease(nullptr));
}
