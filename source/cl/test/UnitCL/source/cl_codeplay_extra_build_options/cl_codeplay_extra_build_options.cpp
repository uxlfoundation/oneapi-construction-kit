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

class cl_codeplay_extra_build_options_BuildFlags : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!(isPlatformExtensionSupported("cl_codeplay_extra_build_options") &&
          getDeviceCompilerAvailable())) {
      GTEST_SKIP();
    }
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(cl_codeplay_extra_build_options_BuildFlags, clBuildPrevecNoneTest) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-cl-vec=none", nullptr, nullptr));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags, clBuildPrevecLoopTest) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-cl-vec=loop", nullptr, nullptr));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags, clBuildPrevecSlpTest) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-cl-vec=slp", nullptr, nullptr));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags, clBuildPrevecAllTest) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-cl-vec=all", nullptr, nullptr));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags,
       clCompilePrecacheLocalSizes) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr,
                                  "-cl-precache-local-sizes=16,:256,32,1", 0,
                                  nullptr, nullptr, nullptr, nullptr));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags,
       clBuildAndRunPrecacheLocalSizes) {
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr,
                                "-cl-precache-local-sizes=1,:256,32,1", nullptr,
                                nullptr));

  // Create and enqueue the kernel to make sure the flag didn't break
  // everything.
  cl_int errorcode = CL_SUCCESS;
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  ASSERT_SUCCESS(errorcode);

  cl_mem in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int),
                                    nullptr, &errorcode);
  ASSERT_TRUE(nullptr != in_buffer);
  EXPECT_SUCCESS(errorcode);

  cl_mem out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int),
                                     nullptr, &errorcode);
  ASSERT_TRUE(nullptr != out_buffer);
  EXPECT_SUCCESS(errorcode);

  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                   static_cast<void *>(&out_buffer)));
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                   static_cast<void *>(&in_buffer)));

  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &errorcode);
  ASSERT_TRUE(nullptr != command_queue);
  EXPECT_SUCCESS(errorcode);

  // We precached a local size of 1 above, so this should hit the cached kernel.
  const size_t work_size = 1;

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &work_size, &work_size, 0, nullptr,
                                        nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseMemObject(in_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(out_buffer));
  EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));
}

TEST_F(cl_codeplay_extra_build_options_BuildFlags,
       clBuildPrecacheLocalSizesInvalid) {
  // Local work group sizes only support up to three dimensions.
  ASSERT_EQ_ERRCODE(
      CL_INVALID_BUILD_OPTIONS,
      clBuildProgram(program, 0, nullptr,
                     "-cl-precache-local-sizes=16:256,32:1,2,3,4,5", nullptr,
                     nullptr));
  // Make sure it rejects characters that aren't numbers.
  ASSERT_EQ_ERRCODE(CL_INVALID_BUILD_OPTIONS,
                    clBuildProgram(program, 0, nullptr,
                                   "-cl-precache-local-sizes=8:16:4,apples",
                                   nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_BUILD_OPTIONS,
      clBuildProgram(program, 0, nullptr, "-cl-precache-local-sizes=4zz",
                     nullptr, nullptr));
  // Finally check extreme values: zero and negative.
  ASSERT_EQ_ERRCODE(
      CL_INVALID_BUILD_OPTIONS,
      clBuildProgram(program, 0, nullptr, "-cl-precache-local-sizes=0", nullptr,
                     nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_BUILD_OPTIONS,
      clBuildProgram(program, 0, nullptr, "-cl-precache-local-sizes=-4",
                     nullptr, nullptr));
}

// Disabled because this test sets the global variable `Enabled`
// from llvm::Statistics to true which causes later vecz runs to have
// Statistics printed, which we don't want to unless explicitely asked.
TEST_F(cl_codeplay_extra_build_options_BuildFlags,
       DISABLED_clCompileLLVMStatsTest) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "-cl-llvm-stats", 0,
                                  nullptr, nullptr, nullptr, nullptr));
}

// Disabled because this test sets the global variable `Enabled`
// from llvm::Statistics to true which causes later vecz runs to have
// Statistics printed, which we don't want to unless explicitely asked.
TEST_F(cl_codeplay_extra_build_options_BuildFlags,
       DISABLED_clBuildLLVMStatsTest) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-cl-llvm-stats", nullptr, nullptr));
}
