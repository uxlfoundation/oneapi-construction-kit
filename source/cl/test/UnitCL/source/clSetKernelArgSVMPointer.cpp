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

#include <cstring>

#include "Common.h"

class clSetKernelArgSVMPointerTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const char *code = R"(
kernel void test(global int* out) {
  size_t id = get_global_id(0);
  out[id] = (int)id;
}
)";
    const size_t length = std::strlen(code);
    cl_int error{};
    program = clCreateProgramWithSource(context, 1, &code, &length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_SUCCESS(
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "test", &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(kernel, nullptr);
  }

  virtual void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    ucl::ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clSetKernelArgSVMPointerTest, NotImplemented) {
  cl_device_svm_capabilities svm_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES,
                                 sizeof(svm_capabilities), &svm_capabilities,
                                 nullptr));
  if (0 != svm_capabilities) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  const cl_uint arg_index{};
  const void *arg_value{};
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clSetKernelArgSVMPointer(kernel, arg_index, arg_value));
}
