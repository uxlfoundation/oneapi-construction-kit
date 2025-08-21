// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

class clCreateKernelsInProgramGoodTest : public ucl::ContextTest {
protected:
  enum { NUM_KERNELS = 10 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;} void "
        "kernel bar(global int * a, global int * b) {*a = *b;}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernels[NUM_KERNELS] = {};
};

class clCreateKernelsInProgramBadTest : public ucl::ContextTest {
protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source = "bad kernel";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clCreateKernelsInProgramGoodTest, Default) {
  ASSERT_SUCCESS(clCreateKernelsInProgram(program, 2, kernels, nullptr));
}

TEST_F(clCreateKernelsInProgramGoodTest, NumKernelsRet) {
  cl_uint num_kernels;
  ASSERT_SUCCESS(clCreateKernelsInProgram(program, 2, nullptr, &num_kernels));
  ASSERT_EQ(2u, num_kernels);
}

TEST_F(clCreateKernelsInProgramGoodTest, GoodProgramNumKernelsTooSmall) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCreateKernelsInProgram(program, 0, kernels, nullptr));
}

TEST_F(clCreateKernelsInProgramBadTest, BadProgram) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clCreateKernelsInProgram(nullptr, 0, nullptr, nullptr));
}

TEST_F(clCreateKernelsInProgramBadTest, BadProgramExecutable) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM_EXECUTABLE,
                    clCreateKernelsInProgram(program, 1, nullptr, nullptr));
}
