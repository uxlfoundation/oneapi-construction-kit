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

#include <chrono>
#include <thread>

#include "Common.h"

class clReleaseCommandQueueTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    queue = clCreateCommandQueue(context, device, 0, &errorcode);
    EXPECT_TRUE(queue);
    ASSERT_SUCCESS(errorcode);
  }

  cl_command_queue queue = nullptr;
};

TEST_F(clReleaseCommandQueueTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clReleaseCommandQueue(nullptr));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

// Test that we can release the command queue and then read the associated event
// Based on the cts test test_api queue_flush_on_release
TEST_F(clReleaseCommandQueueTest, TestEventFlush) {
  if (!UCL::hasCompilerSupport(device)) {
    EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    GTEST_SKIP();
  }

  cl_int errorcode;

  const char *source = "void kernel test(){}";

  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  EXPECT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "test", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);

  // Enqueue the kernel
  const size_t global_size = 1;
  cl_event event;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size,
                                        nullptr, 0, nullptr, &event));
  EXPECT_SUCCESS(errorcode);

  // Release the queue
  EXPECT_SUCCESS(clReleaseCommandQueue(queue));

  cl_int status = 0;
  // Repeatedly check the event until it is complete or 2s has passed
  for (unsigned int i = 0; i < 20; i++) {
    // Wait for kernel to execute since the queue must flush on release
    const cl_int err = clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      sizeof(cl_int), &status, nullptr);
    if ((err == CL_SUCCESS) && (status == CL_COMPLETE)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100 * 1000));
  }

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
  EXPECT_TRUE(status == CL_COMPLETE);
}
