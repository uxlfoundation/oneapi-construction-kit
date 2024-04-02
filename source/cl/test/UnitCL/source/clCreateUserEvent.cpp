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

class clCreateUserEventTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    context =
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
    EXPECT_TRUE(context);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (context) {
      EXPECT_SUCCESS(clReleaseContext(context));
    }
    ContextTest::TearDown();
  }

  cl_context context = nullptr;
};

TEST_F(clCreateUserEventTest, Default) {
  cl_int errorcode;
  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clCreateUserEventTest, HasCorrectExecutionStatus) {
  cl_int errorcode;
  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  cl_int status = 0;
  ASSERT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(status), &status, nullptr));
  ASSERT_EQ_EXECSTATUS(CL_SUBMITTED, status);
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clCreateUserEventTest, BadContext) {
  cl_int errorcode;
  EXPECT_FALSE(clCreateUserEvent(nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errorcode);
}

TEST_F(clCreateUserEventTest, GetProfilingInfo) {
  cl_int errorcode;
  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_PROFILING_INFO_NOT_AVAILABLE,
                    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                            0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clCreateUserEventTest, SubsequentCommandsWaitOnUserEvents) {
  enum { NUM_EVENTS = 16 };

  cl_int errorcode;
  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent[NUM_EVENTS];

  for (unsigned int i = 0; i < NUM_EVENTS; i++) {
    ASSERT_SUCCESS(
        clEnqueueMarkerWithWaitList(queue, 1, &event, &(markerEvent[i])));
  }

  for (unsigned int i = 0; i < NUM_EVENTS; i++) {
    cl_int status = 0;
    ASSERT_SUCCESS(clGetEventInfo(markerEvent[i],
                                  CL_EVENT_COMMAND_EXECUTION_STATUS,
                                  sizeof(status), &status, nullptr));
    ASSERT_LE_EXECSTATUS(CL_SUBMITTED, status);
  }

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  for (unsigned int i = 0; i < NUM_EVENTS; i++) {
    ASSERT_SUCCESS(clReleaseEvent(markerEvent[i]));
  }

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

// This test assumes clSetUserEventStatus can happen before clEnqueueReadBuffer
// completes. If it cannot, it deadlocks. While we would like to let the
// clSetUserEventStatus happen first, this is not required, not what our
// implementation does, and not what all other implementations do either.
TEST_F(clCreateUserEventTest, DISABLED_OutOfOrderQueue) {
  cl_command_queue_properties properties = 0;

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES,
                                 sizeof(properties), &properties, nullptr));

  if (0 == (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE & properties)) {
    return;  // we are essentially skipping this test case as our queue does't
             // support out of order!
  }

  cl_int errorcode = !CL_SUCCESS;

  char initialData = 42;

  cl_mem mem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, 1, &initialData,
                              &errorcode);
  EXPECT_TRUE(mem);
  ASSERT_SUCCESS(errorcode);

  cl_command_queue queue =
      clCreateCommandQueue(context, device, properties, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  char readData = 0;
  ASSERT_SUCCESS(clEnqueueReadBuffer(queue, mem, CL_FALSE, 0, 1, &readData, 1,
                                     &event, nullptr));

  const char newData = 13;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, mem, CL_TRUE, 0, 1, &newData, 0,
                                      nullptr, nullptr));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clFinish(queue));

  ASSERT_EQ(newData, readData);

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
  ASSERT_SUCCESS(clReleaseMemObject(mem));
}
