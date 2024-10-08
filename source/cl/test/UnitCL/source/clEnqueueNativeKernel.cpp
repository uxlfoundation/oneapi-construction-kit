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
#include "EventWaitList.h"

static void CL_CALLBACK user_func(void *args) {
  // Redmine #5130: do stuff
  (void)args;
}

class clEnqueueNativeKernelTest : public ucl::CommandQueueTest,
                                  TestWithEventWaitList {
 protected:
  struct Args {
    int a;
    int b;
  };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    hasNativeKernelSupport = UCL::hasNativeKernelSupport(device);
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    if (hasNativeKernelSupport) {
      ASSERT_EQ_ERRCODE(err,
                        clEnqueueNativeKernel(command_queue, &user_func, &args,
                                              sizeof(Args), 0, nullptr, nullptr,
                                              num_events, events, event));
    }
  }

  bool hasNativeKernelSupport = false;
  Args args = {};
};

TEST_F(clEnqueueNativeKernelTest, InvalidCommandQueue) {
  if (hasNativeKernelSupport) {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_COMMAND_QUEUE,
        clEnqueueNativeKernel(nullptr, &user_func, &args, sizeof(Args), 0,
                              nullptr, nullptr, 0, nullptr, nullptr));
  }
}

TEST_F(clEnqueueNativeKernelTest, InvalidValueUserFunc) {
  if (hasNativeKernelSupport) {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, nullptr, &args, sizeof(Args), 0,
                              nullptr, nullptr, 0, nullptr, nullptr));
  }
}

TEST_F(clEnqueueNativeKernelTest, InvalidValueArgs) {
  if (hasNativeKernelSupport) {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, &user_func, nullptr, sizeof(Args),
                              0, nullptr, nullptr, 0, nullptr, nullptr));

    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, &user_func, &args, 0, 0, nullptr,
                              nullptr, 0, nullptr, nullptr));
  }
}

TEST_F(clEnqueueNativeKernelTest, InvalidValueMemObjects) {
  if (hasNativeKernelSupport) {
    cl_int status;
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int),
                                   nullptr, &status);
    EXPECT_SUCCESS(status);

    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, &user_func, nullptr, 0, 1, &buffer,
                              nullptr, 0, nullptr, nullptr));

    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, &user_func, nullptr, 0, 1, nullptr,
                              nullptr, 0, nullptr, nullptr));

    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueNativeKernel(command_queue, &user_func, nullptr, 0, 0, &buffer,
                              nullptr, 0, nullptr, nullptr));

    EXPECT_SUCCESS(clReleaseMemObject(buffer));
  }
}

TEST_F(clEnqueueNativeKernelTest, InvalidMemObject) {
  if (hasNativeKernelSupport) {
    cl_int status;
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int),
                                   nullptr, &status);
    EXPECT_SUCCESS(status);
    cl_mem mems[] = {buffer, nullptr};
    const void *args_mem_loc[2] = {nullptr, nullptr};

    EXPECT_EQ_ERRCODE(
        CL_INVALID_MEM_OBJECT,
        clEnqueueNativeKernel(command_queue, &user_func, &args, sizeof(Args), 2,
                              mems, args_mem_loc, 0, nullptr, nullptr));

    ASSERT_SUCCESS(clReleaseMemObject(buffer));
  }
}

TEST_F(clEnqueueNativeKernelTest, Default) {
  if (hasNativeKernelSupport) {
    cl_event event;
    ASSERT_SUCCESS(clEnqueueNativeKernel(command_queue, &user_func, &args,
                                         sizeof(Args), 0, nullptr, nullptr, 0,
                                         nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    cl_int status;
    ASSERT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                  sizeof(cl_int), &status, nullptr));
    ASSERT_EQ_ERRCODE(CL_COMPLETE, status);
  } else {
    cl_event event = nullptr;
    ASSERT_EQ_ERRCODE(
        CL_INVALID_OPERATION,
        clEnqueueNativeKernel(command_queue, &user_func, &args, sizeof(Args), 0,
                              nullptr, nullptr, 0, nullptr, &event));
    ASSERT_FALSE(event);
  }
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueNativeKernelTest)
