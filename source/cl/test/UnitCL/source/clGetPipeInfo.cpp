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

class clGetPipeInfoTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int error{};
    buffer = clCreateBuffer(context, 0, 42, nullptr, &error);
    ASSERT_NE(buffer, nullptr);
    ASSERT_SUCCESS(error);
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }

  virtual void TearDown() {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

TEST_F(clGetPipeInfoTest, NotImplemented) {
  cl_bool pipe_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                 sizeof(pipe_support), &pipe_support, nullptr));
  if (CL_FALSE != pipe_support) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  const cl_pipe_info param_name{};
  const size_t param_value_size{};
  void *param_value{};
  size_t *param_value_size_ret{};
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clGetPipeInfo(buffer, param_name, param_value_size,
                                  param_value, param_value_size_ret));
}
