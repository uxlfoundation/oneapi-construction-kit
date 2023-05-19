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

#include "Common.h"

using clCreateProgramWithSourceTest = ucl::ContextTest;

TEST_F(clCreateProgramWithSourceTest, Default) {
  const char *buffer = "something";
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, &buffer, nullptr, &errorcode);
  ASSERT_TRUE(program);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clCreateProgramWithSourceTest, WithLength) {
  const char *buffer = "something";
  const size_t length = 4;
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, &buffer, &length, &errorcode);
  ASSERT_TRUE(program);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clCreateProgramWithSourceTest, ManyStrings) {
  const char *buffer[] = {"something", "else"};
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, buffer, nullptr, &errorcode);
  ASSERT_TRUE(program);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clCreateProgramWithSourceTest, ManyStringsWithLength) {
  const char *buffer[] = {"something", "else"};
  const size_t lengths[] = {4, 2};
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, buffer, lengths, &errorcode);
  ASSERT_TRUE(program);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clCreateProgramWithSourceTest, InvalidContext) {
  cl_int errorcode;
  EXPECT_FALSE(
      clCreateProgramWithSource(nullptr, 1, nullptr, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errorcode);
}

TEST_F(clCreateProgramWithSourceTest, InvalidCount) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  EXPECT_FALSE(
      clCreateProgramWithSource(context, 0, nullptr, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clCreateProgramWithSourceTest, InvalidStrings) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  EXPECT_FALSE(
      clCreateProgramWithSource(context, 1, nullptr, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clCreateProgramWithSourceTest, IndividualStringIsNull) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();
  }
  const char *buffer = nullptr;
  cl_int errorcode;
  EXPECT_FALSE(
      clCreateProgramWithSource(context, 1, &buffer, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

#if defined(CL_VERSION_3_0)
TEST_F(clCreateProgramWithSourceTest, IL) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  const char *buffer = "foo";
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, &buffer, nullptr, &errorcode);
  ASSERT_SUCCESS(errorcode);
  ASSERT_NE(program, nullptr);

  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, 0, nullptr, &size));

  //  If program is created with clCreateProgramWithSource,
  //  clCreateProgramWithBinary or clCreateProgramWithBuiltInKernels the memory
  //  pointed to by param_value will be unchanged and param_value_size_ret will
  //  be set to 0.
  ASSERT_EQ(size, 0);
  UCL::Buffer<char> param_val{1};
  param_val[0] = 42;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, param_val.size(),
                                  param_val.data(), nullptr));
  ASSERT_EQ(param_val[0], 42);

  EXPECT_SUCCESS(clReleaseProgram(program));
}
#endif
