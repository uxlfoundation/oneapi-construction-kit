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
  ur_module_handle_t module = nullptr;
  const auto kernel_source = uur::Environment::instance->LoadSource("foo", 0);
  ASSERT_SUCCESS(kernel_source.status);
  ASSERT_SUCCESS(urModuleCreate(context, kernel_source.source,
                                kernel_source.source_length, "", nullptr,
                                nullptr, &module));
  ur_program_handle_t program = nullptr;
  EXPECT_SUCCESS(urProgramCreate(context, 1, &module, nullptr, &program));
  ur_kernel_handle_t kernel = nullptr;
  EXPECT_SUCCESS(urKernelCreate(program, kernel_source.kernel_name, &kernel));

  EXPECT_SUCCESS(urKernelRelease(kernel));
  EXPECT_SUCCESS(urProgramRelease(program));
  EXPECT_SUCCESS(urModuleRelease(module));
}
