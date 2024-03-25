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

#include <array>

#include "uur/fixtures.h"

struct urProgramLinkTest : uur::QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    kernel_foo_source =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_foo_source.status);

    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_foo_source.source,
                                         kernel_foo_source.source_length,
                                         nullptr, programs.data()));
    ASSERT_SUCCESS(urProgramCompile(context, programs[0], nullptr));

    kernel_goo_source =
        uur::Environment::instance->LoadSource("goo", this->GetParam());
    ASSERT_SUCCESS(kernel_goo_source.status);
    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_goo_source.source,
                                         kernel_goo_source.source_length,
                                         nullptr, &programs[1]));
    ASSERT_SUCCESS(urProgramCompile(context, programs[1], nullptr));
  }

  void TearDown() override {
    for (auto &program : programs) {
      if (program) {
        EXPECT_SUCCESS(urProgramRelease(program));
      }
    }
    if (linked_program) {
      ASSERT_SUCCESS(urProgramRelease(linked_program));
    }
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::TearDown());
  }

  uur::Environment::KernelSource kernel_foo_source, kernel_goo_source;
  std::array<ur_program_handle_t, 2> programs = {nullptr, nullptr};
  ur_program_handle_t linked_program = nullptr;
};
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urProgramLinkTest);

TEST_P(urProgramLinkTest, Success) {
  ASSERT_SUCCESS(urProgramLink(context, programs.size(), programs.data(),
                               nullptr, &linked_program));
  ASSERT_NE(nullptr, linked_program);
}

TEST_P(urProgramLinkTest, InvalidNullHandleContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urProgramLink(nullptr, programs.size(), programs.data(),
                                 nullptr, &linked_program));
}

TEST_P(urProgramLinkTest, InvalidValueCount) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_VALUE,
      urProgramLink(context, 0, programs.data(), nullptr, &linked_program));
}

TEST_P(urProgramLinkTest, InvalidNullHandleProgramList) {
  std::vector<ur_program_handle_t> broken_programs = {programs[0], nullptr};
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urProgramLink(context, broken_programs.size(), broken_programs.data(),
                    nullptr, &linked_program));
}

TEST_P(urProgramLinkTest, InvalidNullPointerProgramList) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urProgramLink(context, programs.size(), nullptr, nullptr,
                                 &linked_program));
}

TEST_P(urProgramLinkTest, InvalidNullPointerOutProgram) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urProgramLink(context, programs.size(), programs.data(),
                                 nullptr, nullptr));
}

TEST_P(urProgramLinkTest, LinkFailureUncompiled) {
  ur_program_handle_t uncompiled_program = nullptr;
  ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_goo_source.source,
                                       kernel_goo_source.source_length, nullptr,
                                       &uncompiled_program));

  std::vector<ur_program_handle_t> broken_programs = {programs[0],
                                                      uncompiled_program};
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_PROGRAM_LINK_FAILURE,
      urProgramLink(context, broken_programs.size(), broken_programs.data(),
                    nullptr, &linked_program));
  ASSERT_SUCCESS(urProgramRelease(uncompiled_program));
}
