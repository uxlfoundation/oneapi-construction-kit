// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vector>

#include "uur/fixtures.h"

using urProgramCreateTest = uur::ModuleTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramCreateTest);

TEST_P(urProgramCreateTest, Success) {
  ur_program_handle_t program = nullptr;
  ASSERT_SUCCESS(urProgramCreate(context, 1, &module, nullptr, &program));
  EXPECT_SUCCESS(urProgramRelease(program));
}

TEST_P(urProgramCreateTest, InvalidNullHandle) {
  ur_program_handle_t program = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramCreate(nullptr, 1, &module, nullptr, &program));
}

TEST_P(urProgramCreateTest, InvalidNullPointerModule) {
  ur_program_handle_t program = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urProgramCreate(context, 1, nullptr, nullptr, &program));
}

TEST_P(urProgramCreateTest, InvalidNullPointerProgram) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urProgramCreate(context, 1, &module, nullptr, nullptr));
}

using urProgramCreateMultiDeviceTest = uur::MultiDeviceContextTest;
TEST_F(urProgramCreateMultiDeviceTest, urProgramCreateTest) {
  ur_module_handle_t module = nullptr;
  auto kernel_source = uur::Environment::instance->LoadSource("foo", 0);
  ASSERT_SUCCESS(kernel_source.status);
  ASSERT_SUCCESS(urModuleCreate(context, kernel_source.source,
                                kernel_source.source_length, "", nullptr,
                                nullptr, &module));
  ur_program_handle_t program = nullptr;
  EXPECT_SUCCESS(urProgramCreate(context, 1, &module, nullptr, &program));

  EXPECT_SUCCESS(urProgramRelease(program));
  EXPECT_SUCCESS(urModuleRelease(module));
}

using urMultiModuleProgramCreateTest = uur::MultiModuleTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urMultiModuleProgramCreateTest);
TEST_P(urMultiModuleProgramCreateTest, Success) {
  ur_program_handle_t program = nullptr;
  std::vector<ur_module_handle_t> modules = {module1, module2};
  ASSERT_SUCCESS(
      urProgramCreate(context, 2, modules.data(), nullptr, &program));
  EXPECT_SUCCESS(urProgramRelease(program));
}
