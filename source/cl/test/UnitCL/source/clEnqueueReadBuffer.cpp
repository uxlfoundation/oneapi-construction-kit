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

class clEnqueueReadBufferTest : public ucl::CommandQueueTest,
                                TestWithEventWaitList {
 protected:
  enum { SIZE = 128, INT_SIZE = (SIZE * sizeof(cl_int)) };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errorcode;
    inMem = clCreateBuffer(context, 0, INT_SIZE, nullptr, &errorcode);
    EXPECT_TRUE(inMem);
    ASSERT_SUCCESS(errorcode);
    outMem = clCreateBuffer(context, 0, INT_SIZE, nullptr, &errorcode);
    EXPECT_TRUE(outMem);
    ASSERT_SUCCESS(errorcode);
    for (cl_uint i = 0; i < SIZE; i++) {
      inBuffer[i] = i;
      outBuffer[i] = 0xFFFFFFFF;
    }
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, CL_TRUE, 0,
                                        INT_SIZE, inBuffer, 0, nullptr,
                                        &writeEvent));
  }

  void TearDown() override {
    if (writeEvent) {
      EXPECT_SUCCESS(clReleaseEvent(writeEvent));
    }
    if (ndRangeEvent) {
      EXPECT_SUCCESS(clReleaseEvent(ndRangeEvent));
    }
    if (readEvent) {
      EXPECT_SUCCESS(clReleaseEvent(readEvent));
    }
    if (outMem) {
      EXPECT_SUCCESS(clReleaseMemObject(outMem));
    }
    if (inMem) {
      EXPECT_SUCCESS(clReleaseMemObject(inMem));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(
        err, clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, INT_SIZE,
                                 inBuffer, num_events, events, event));
  }

  cl_mem inMem = nullptr;
  cl_mem outMem = nullptr;
  cl_event readEvent = nullptr;
  cl_event ndRangeEvent = nullptr;
  cl_event writeEvent = nullptr;
  int inBuffer[SIZE] = {};
  int outBuffer[SIZE] = {};
};

TEST_F(clEnqueueReadBufferTest, Default) {
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     INT_SIZE, outBuffer, 1, &writeEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < SIZE; i++) {
    EXPECT_EQ(inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueReadBufferTest, WithNDRangeInBetween) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {a[get_global_id(0)] = "
      "b[get_global_id(0)];}";
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outMem));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inMem));

  const size_t global_size = SIZE;

  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 1, &writeEvent,
                                        &ndRangeEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, CL_FALSE, 0,
                                     INT_SIZE, outBuffer, 1, &ndRangeEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < SIZE; i++) {
    EXPECT_EQ(inBuffer[i], outBuffer[i]);
  }

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

// Test that doing the read and enqueue on two different queues
// using non-user events to synchronize works
// Identical to WithNDRangeInBetween test except for the queue change
TEST_F(clEnqueueReadBufferTest, WithNDRangeInBetweenTwoQueues) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {a[get_global_id(0)] = "
      "b[get_global_id(0)];}";
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  EXPECT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outMem));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inMem));

  const size_t global_size = SIZE;
  cl_command_queue command_queue_b =
      clCreateCommandQueue(context, device, 0, &errorcode);

  EXPECT_SUCCESS(errorcode);

  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue_b, kernel, 1, nullptr,
                                        &global_size, nullptr, 1, &writeEvent,
                                        &ndRangeEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, CL_FALSE, 0,
                                     INT_SIZE, outBuffer, 1, &ndRangeEvent,
                                     &readEvent));
  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < SIZE; i++) {
    EXPECT_EQ(inBuffer[i], outBuffer[i]);
  }

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseCommandQueue(command_queue_b));
}

TEST_F(clEnqueueReadBufferTest, Blocking) {
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, INT_SIZE,
                                     outBuffer, 1, &writeEvent, nullptr));

  for (cl_int i = 0; i < SIZE; i++) {
    EXPECT_EQ(inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueReadBufferTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueReadBuffer(nullptr, inMem, CL_TRUE, 0, INT_SIZE,
                                        inBuffer, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferTest, InvalidMemObject) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueReadBuffer(command_queue, nullptr, CL_TRUE, 0, INT_SIZE,
                          inBuffer, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferTest, InvalidBufferSize) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, INT_SIZE, INT_SIZE,
                          inBuffer, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferTest, InvalidBuffer) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, INT_SIZE, nullptr,
                          0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferTest, SizeZero) {
  // An error when size == 0 was removed starting with OpenCL 2.1.
  if (UCL::isDeviceVersionAtLeast({2, 1})) {
    ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, 0,
                                       inBuffer, 0, nullptr, nullptr));
  } else {
    ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, 0,
                                          inBuffer, 0, nullptr, nullptr));
  }
}

TEST_F(clEnqueueReadBufferTest, NullWaitList) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_EVENT_WAIT_LIST,
      clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, INT_SIZE, inBuffer,
                          1, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferTest, WaitListWithBadNumber) {
  cl_event list;
  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT_WAIT_LIST,
                    clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0,
                                        INT_SIZE, inBuffer, 0, &list, nullptr));
}

TEST_F(clEnqueueReadBufferTest, ReadFromWriteOnly) {
  cl_int errorcode;
  cl_mem otherMem = clCreateBuffer(context, CL_MEM_HOST_WRITE_ONLY, INT_SIZE,
                                   nullptr, &errorcode);
  ASSERT_TRUE(otherMem);
  EXPECT_SUCCESS(errorcode);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueReadBuffer(command_queue, otherMem, CL_TRUE, 0, INT_SIZE,
                          inBuffer, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(otherMem));
}

TEST_F(clEnqueueReadBufferTest, ReadFromHostNoAccess) {
  cl_int errorcode;
  cl_mem otherMem = clCreateBuffer(context, CL_MEM_HOST_NO_ACCESS, INT_SIZE,
                                   nullptr, &errorcode);
  ASSERT_TRUE(otherMem);
  EXPECT_SUCCESS(errorcode);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueReadBuffer(command_queue, otherMem, CL_TRUE, 0, INT_SIZE,
                          inBuffer, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(otherMem));
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueReadBufferTest)
