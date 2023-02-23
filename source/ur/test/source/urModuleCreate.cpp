// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cstring>

#include "uur/fixtures.h"

using urModuleCreateTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urModuleCreateTest);

TEST_P(urModuleCreateTest, Success) {
  ur_module_handle_t module = nullptr;
  const auto kernel_source =
      uur::Environment::instance->LoadSource("foo", this->GetParam());
  ASSERT_SUCCESS(kernel_source.status);
  const char *options = "";
  ASSERT_SUCCESS(urModuleCreate(context, kernel_source.source,
                                kernel_source.source_length, options, nullptr,
                                nullptr, &module));
  ASSERT_NE(nullptr, module);
  EXPECT_SUCCESS(urModuleRelease(module));
}

TEST_P(urModuleCreateTest, InvalidNullHandle) {
  ur_module_handle_t module = nullptr;
  const auto kernel_source =
      uur::Environment::instance->LoadSource("foo", this->GetParam());
  ASSERT_SUCCESS(kernel_source.status);
  const char *options = "";
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urModuleCreate(nullptr, kernel_source.source, kernel_source.source_length,
                     options, nullptr, nullptr, &module));
}

TEST_P(urModuleCreateTest, InvalidNullPointerIL) {
  ur_module_handle_t module = nullptr;
  const char *source = nullptr;
  const auto source_length = 0;
  const char *options = "";
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urModuleCreate(context, static_cast<const void *>(source), source_length,
                     options, nullptr, nullptr, &module));
}

TEST_P(urModuleCreateTest, InvalidNullPointerOptions) {
  ur_module_handle_t module = nullptr;
  const auto kernel_source =
      uur::Environment::instance->LoadSource("foo", this->GetParam());
  ASSERT_SUCCESS(kernel_source.status);
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urModuleCreate(context, kernel_source.source, kernel_source.source_length,
                     nullptr, nullptr, nullptr, &module));
}

TEST_P(urModuleCreateTest, InvalidNullPointerModule) {
  const auto kernel_source =
      uur::Environment::instance->LoadSource("foo", this->GetParam());
  ASSERT_SUCCESS(kernel_source.status);
  const char *options = "";
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urModuleCreate(context, kernel_source.source, kernel_source.source_length,
                     options, nullptr, nullptr, nullptr));
}

using urModuleCreateMultiDeviceTest = uur::MultiDeviceContextTest;
TEST_F(urModuleCreateMultiDeviceTest, urModuleCreateTest) {
  ur_module_handle_t module = nullptr;
  const auto kernel_source = uur::Environment::instance->LoadSource("foo", 0);
  ASSERT_SUCCESS(kernel_source.status);
  ASSERT_SUCCESS(urModuleCreate(context, kernel_source.source,
                                kernel_source.source_length, "", nullptr,
                                nullptr, &module));
}
