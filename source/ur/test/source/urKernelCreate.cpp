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
