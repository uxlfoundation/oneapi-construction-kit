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

#include <vector>

#include "Common.h"
#include "EventWaitList.h"

class clEnqueueMarkerWithWaitListTest : public ucl::CommandQueueTest,
                                        TestWithEventWaitList {
protected:
  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(err, clEnqueueMarkerWithWaitList(
                               command_queue, num_events, events, event));
  }
};

TEST_F(clEnqueueMarkerWithWaitListTest, default) {
  cl_int errorcode = !CL_SUCCESS;

  cl_event event = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;

  ASSERT_SUCCESS(
      clEnqueueMarkerWithWaitList(command_queue, 1, &event, &markerEvent));
  ASSERT_TRUE(markerEvent);

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
}

TEST_F(clEnqueueMarkerWithWaitListTest, EmptyList) {
  cl_event markerEvent = nullptr;

  ASSERT_SUCCESS(
      clEnqueueMarkerWithWaitList(command_queue, 0, nullptr, &markerEvent));
  ASSERT_TRUE(markerEvent);

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
}

TEST_F(clEnqueueMarkerWithWaitListTest, InvalidCommandQueue) {
  cl_event markerEvent = nullptr;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueMarkerWithWaitList(nullptr, 0, nullptr, &markerEvent));
  ASSERT_FALSE(markerEvent);
}

TEST_F(clEnqueueMarkerWithWaitListTest, NumEventsButWaitListNull) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_EVENT_WAIT_LIST,
      clEnqueueMarkerWithWaitList(command_queue, 1, nullptr, nullptr));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueMarkerWithWaitListTest)
