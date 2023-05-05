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

#include <limits>

#include "Common.h"

struct clCreateBufferWithPropertiesTest : ucl::ContextTest {
  void TearDown() override {
    if (buffer) {
      ASSERT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

TEST_F(clCreateBufferWithPropertiesTest, Success) {
  cl_int error;
  const cl_mem_properties buff_properties[] = {0};
  buffer = clCreateBufferWithProperties(context, buff_properties, 0, 128,
                                        nullptr, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, buffer);
  size_t size;
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem_properties), size);
  std::vector<cl_mem_properties> properties(1);
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_PROPERTIES, size,
                                    properties.data(), nullptr));
}

TEST_F(clCreateBufferWithPropertiesTest, SuccessNull) {
  cl_int error;
  buffer =
      clCreateBufferWithProperties(context, nullptr, 0, 128, nullptr, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, buffer);
}

TEST_F(clCreateBufferWithPropertiesTest, InvalidProperty) {
  cl_int error;
  const cl_mem_properties buff_properties[] = {0xFFFF, 0};
  buffer = clCreateBufferWithProperties(context, buff_properties, 0, 128,
                                        nullptr, &error);
  ASSERT_EQ(nullptr, buffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, error);
}

TEST_F(clCreateBufferWithPropertiesTest, DoubleProperty) {
  cl_int error;
  const cl_mem_properties buff_properties[] = {0xFFFF, 0xFFFF};
  cl_mem buffer = clCreateBufferWithProperties(context, buff_properties, 0, 128,
                                               nullptr, &error);
  ASSERT_EQ(nullptr, buffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, error);
}

TEST_F(clCreateBufferWithPropertiesTest, IncorrectlyTerminated) {
  cl_int error;
  const cl_mem_properties buff_properties[] = {0xFFFF};
  cl_mem buffer = clCreateBufferWithProperties(context, buff_properties, 0, 128,
                                               nullptr, &error);
  ASSERT_EQ(nullptr, buffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_PROPERTY, error);
}
