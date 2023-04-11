// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

struct urProgramCompileTest : uur::QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    kernel_source =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source.status);

    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source.source,
                                         kernel_source.source_length, nullptr,
                                         &program));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(urProgramRelease(program));
    }
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::TearDown());
  }

  uur::Environment::KernelSource kernel_source;
  ur_program_handle_t program = nullptr;
};
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramCompileTest);

TEST_P(urProgramCompileTest, Success) {
  ASSERT_SUCCESS(urProgramCompile(context, program, nullptr));
}

TEST_P(urProgramCompileTest, InvalidNullHandleContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramCompile(nullptr, program, nullptr));
}

TEST_P(urProgramCompileTest, InvalidNullHandleProgram) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramCompile(context, nullptr, nullptr));
}
