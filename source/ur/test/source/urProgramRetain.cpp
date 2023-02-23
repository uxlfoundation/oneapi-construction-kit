// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urProgramRetainTest = uur::ProgramTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramRetainTest);

TEST_P(urProgramRetainTest, Success) {
  ASSERT_SUCCESS(urProgramRetain(program));
  EXPECT_SUCCESS(urProgramRelease(program));
}

TEST_P(urProgramRetainTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramRetain(nullptr));
}
