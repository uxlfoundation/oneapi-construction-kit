// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urKernelCreateTest = uur::ProgramTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urKernelCreateTest);

TEST_P(urKernelCreateTest, Success) {
  ur_kernel_handle_t kernel = nullptr;
  ASSERT_SUCCESS(urKernelCreate(program, "foo", &kernel));
  ASSERT_NE(nullptr, kernel);
  EXPECT_SUCCESS(urKernelRelease(kernel));
}

TEST_P(urKernelCreateTest, InvalidNullHandle) {
  ur_kernel_handle_t kernel = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urKernelCreate(nullptr, "foo", &kernel));
}

TEST_P(urKernelCreateTest, InvalidNullPointerKernelName) {
  ur_kernel_handle_t kernel = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urKernelCreate(program, nullptr, &kernel));
}

TEST_P(urKernelCreateTest, InvalidNullPointerKernel) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urKernelCreate(program, "foo", nullptr));
}

using urKernelCreateMultiDeviceTest = uur::MultiDeviceContextTest;

TEST_F(urKernelCreateMultiDeviceTest, urKernelCreateTest) {
  const auto kernel_source = uur::Environment::instance->LoadSource("foo", 0);
  ur_program_handle_t program = nullptr;
  ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source.source,
                                       kernel_source.source_length, nullptr,
                                       &program));
  ASSERT_SUCCESS(urProgramBuild(context, program, nullptr));

  ur_kernel_handle_t kernel = nullptr;
  EXPECT_SUCCESS(urKernelCreate(program, kernel_source.kernel_name, &kernel));

  EXPECT_SUCCESS(urKernelRelease(kernel));
  EXPECT_SUCCESS(urProgramRelease(program));
}
