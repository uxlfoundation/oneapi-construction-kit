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

class clSetKernelExecInfoTest : public ucl::ContextTest {
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
  cl_bool param_value = false;
};

TEST_F(clSetKernelExecInfoTest, InvalidOperation) {
  // We need to check our device doesn't support SVM in any capacity before we
  // can be sure of getting this error code.
  cl_device_svm_capabilities svm_capabilities;
  clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_capabilities),
                  &svm_capabilities, nullptr);
  if (svm_capabilities != 0) {
    GTEST_SKIP();
  }

  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM,
                          sizeof(param_value), &param_value));
}

TEST_F(clSetKernelExecInfoTest, InvalidKernel) {
  EXPECT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clSetKernelExecInfo(nullptr, CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM,
                          sizeof(param_value), &param_value));
}

TEST_F(clSetKernelExecInfoTest, InvalidValue) {
  // We need to check our device supports SVM before we can be sure of getting
  // this error code, otherwise CL_INVALID_OPERATION will be the return value.
  cl_device_svm_capabilities svm_capabilities;
  clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_capabilities),
                  &svm_capabilities, nullptr);
  if (svm_capabilities == 0) {
    GTEST_SKIP();
  }

  // CL_INVALID_VALUE can result from either invalid param_name, invalid
  // param_value_size or invalid param_value. We use param_name
  // CL_KERNEL_EXEC_INFO_SVM_PTRS to test the latter two cases as
  // CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM has additional wording
  // around returning CL_INVALID_OPERATION when fine-grained SVM isn't
  // supported. This complicates which error code gets priority, and there
  // aren't any CTS tests yet to use as a reference.
  void *svm_ptr[1] = {nullptr};

  // Invalid param_name
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clSetKernelExecInfo(kernel, 0, sizeof(param_value), &param_value));

  // Invalid param_size
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_SVM_PTRS, 0, &svm_ptr));
  // Invalid param_value
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_SVM_PTRS,
                                        sizeof(svm_ptr), nullptr));
}
