// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

class clWaitForEventsTest : public ucl::CommandQueueTest {
 protected:
  enum { NUM_BUFFERS = 4 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errorcode;
    size = 128;
    buffer = new char[size];
    ASSERT_TRUE(buffer);
    for (unsigned int i = 0; i < NUM_BUFFERS; i++) {
      mem[i] = clCreateBuffer(context, 0, size, nullptr, &errorcode);
      EXPECT_TRUE(mem[i]);
      ASSERT_SUCCESS(errorcode);
      ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, mem[i], true, 0, size,
                                          buffer, 0, nullptr, &(event[i])));
    }
  }

  void TearDown() override {
    if (buffer) {
      delete[] buffer;
    }
    for (unsigned int i = 0; i < NUM_BUFFERS; i++) {
      if (event[i]) {
        EXPECT_SUCCESS(clReleaseEvent(event[i]));
      }
      if (mem[i]) {
        EXPECT_SUCCESS(clReleaseMemObject(mem[i]));
      }
    }
    CommandQueueTest::TearDown();
  }

  unsigned size = 0;
  char *buffer = nullptr;
  cl_mem mem[NUM_BUFFERS] = {};
  cl_event event[NUM_BUFFERS] = {};
};

TEST_F(clWaitForEventsTest, Default) {
  ASSERT_SUCCESS(clWaitForEvents(NUM_BUFFERS, event));
}

TEST_F(clWaitForEventsTest, BadEventList) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clWaitForEvents(NUM_BUFFERS, nullptr));
}

TEST_F(clWaitForEventsTest, BadNumEvents) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clWaitForEvents(0, event));
}

TEST_F(clWaitForEventsTest, BadEventInList) {
  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT, clWaitForEvents(1, &event));
}

TEST_F(clWaitForEventsTest, EventFailed) {
  cl_int errorcode = !CL_SUCCESS;
  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetUserEventStatus(event, -1));

  ASSERT_EQ_ERRCODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
                    clWaitForEvents(1, &event));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clWaitForEventsTest, DependentEventFailed) {
  cl_int errorcode = !CL_SUCCESS;
  cl_event userEvent = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(userEvent);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;

  ASSERT_SUCCESS(
      clEnqueueMarkerWithWaitList(command_queue, 1, &userEvent, &markerEvent));

  ASSERT_SUCCESS(clSetUserEventStatus(userEvent, -1));

  ASSERT_EQ_ERRCODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
                    clWaitForEvents(1, &markerEvent));

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseEvent(userEvent));
}

// Redmine #5147: test contexts are the same
