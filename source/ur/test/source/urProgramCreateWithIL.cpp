// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

struct urProgramCreateWithILTest : uur::QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    kernel_source =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source.status);
  }

  uur::Environment::KernelSource kernel_source;
};
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramCreateWithILTest);

TEST_P(urProgramCreateWithILTest, Success) {
  ur_program_handle_t program = nullptr;
  ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source.source,
                                       kernel_source.source_length, nullptr,
                                       &program));
  EXPECT_SUCCESS(urProgramRelease(program));
}

TEST_P(urProgramCreateWithILTest, InvalidNullHandle) {
  ur_program_handle_t program = nullptr;
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urProgramCreateWithIL(nullptr, kernel_source.source,
                            kernel_source.source_length, nullptr, &program));
}

TEST_P(urProgramCreateWithILTest, InvalidNullPointerSource) {
  ur_program_handle_t program = nullptr;
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urProgramCreateWithIL(context, nullptr, kernel_source.source_length,
                            nullptr, &program));
}

TEST_P(urProgramCreateWithILTest, InvalidSizeLength) {
  ur_program_handle_t program = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_SIZE,
                   urProgramCreateWithIL(context, kernel_source.source, 0,
                                         nullptr, &program));
}

TEST_P(urProgramCreateWithILTest, InvalidNullPointerProgram) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urProgramCreateWithIL(context, kernel_source.source,
                            kernel_source.source_length, nullptr, nullptr));
}
