// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urProgramReleaseTest = uur::ProgramTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramReleaseTest);

TEST_P(urProgramReleaseTest, Success) {
  ASSERT_SUCCESS(urProgramRetain(program));
  EXPECT_SUCCESS(urProgramRelease(program));
}

TEST_P(urProgramReleaseTest, InvalidNullHandle) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramRelease(nullptr));
}
