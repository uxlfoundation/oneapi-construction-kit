// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
