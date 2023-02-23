// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urModuleRetainTest = uur::ModuleTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urModuleRetainTest);

TEST_P(urModuleRetainTest, Success) {
  ASSERT_SUCCESS(urModuleRetain(module));
  EXPECT_SUCCESS(urModuleRelease(module));
}

TEST_P(urModuleRetainTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urModuleRetain(nullptr));
}
