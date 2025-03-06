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

struct clCreateCommandQueueTest
    : ucl::ContextTest,
      testing::WithParamInterface<cl_context_properties> {};

TEST_F(clCreateCommandQueueTest, default) {
  cl_int errcode;
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errcode);
  EXPECT_TRUE(queue);
  EXPECT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clCreateCommandQueueTest, nullptrErrorCode) {
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, nullptr);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clCreateCommandQueueTest, WithBadContext) {
  cl_int errcode;
  EXPECT_FALSE(clCreateCommandQueue(nullptr, device, 0, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errcode);
}

TEST_F(clCreateCommandQueueTest, WithBadDevice) {
  cl_int errcode;
  EXPECT_FALSE(clCreateCommandQueue(context, nullptr, 0, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, errcode);
}

TEST_F(clCreateCommandQueueTest, WithBadProperties) {
  cl_int errcode;
  const auto all_valid_properties = static_cast<cl_command_queue_properties>(
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
  const auto properties = ~all_valid_properties;
  EXPECT_FALSE(clCreateCommandQueue(context, device, properties, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateCommandQueueTest, WithBadAndGoodProperties) {
  cl_int errcode;
  const auto all_valid_properties = static_cast<cl_command_queue_properties>(
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
  const auto properties = static_cast<cl_command_queue_properties>(
      (~all_valid_properties) | all_valid_properties);
  EXPECT_FALSE(clCreateCommandQueue(context, device, properties, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateCommandQueueTest, FloodCommandQueue) {
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  enum { ITERATIONS = 256, SIZE = 1024 };

  cl_int errcode;
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errcode);

  const char *buffer = "void kernel foo() {}";
  cl_program program =
      clCreateProgramWithSource(context, 1, &buffer, nullptr, &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "foo", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);

  const size_t global_size = SIZE;

  cl_event event[ITERATIONS];

  for (cl_uint i = 0; i < ITERATIONS; i++) {
    ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr,
                                          &global_size, nullptr, 0, nullptr,
                                          &(event[i])));
  }

  cl_event markerEvent = nullptr;
  ASSERT_SUCCESS(
      clEnqueueMarkerWithWaitList(queue, ITERATIONS, event, &markerEvent));

  for (cl_uint i = 0; i < ITERATIONS; i++) {
    ASSERT_SUCCESS(clReleaseEvent(event[i]));
  }

  ASSERT_SUCCESS(clWaitForEvents(1, &markerEvent));
  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}
