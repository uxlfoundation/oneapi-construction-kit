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

class clEnqueueBarrierWithWaitListTest : public ucl::CommandQueueTest,
                                         TestWithEventWaitList {
 protected:
  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(err, clEnqueueBarrierWithWaitList(
                               command_queue, num_events, events, event));
  }
};

TEST_F(clEnqueueBarrierWithWaitListTest, InvalidCommandQueue) {
  cl_int status;
  cl_event event_wait_list = clCreateUserEvent(context, &status);
  EXPECT_TRUE(event_wait_list);
  ASSERT_SUCCESS(status);

  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueBarrierWithWaitList(nullptr, 1, &event_wait_list, &event));
  ASSERT_FALSE(event);

  EXPECT_SUCCESS(clReleaseEvent(event_wait_list));
}

TEST_F(clEnqueueBarrierWithWaitListTest, DefaultEventWaitList) {
  cl_int status;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float),
                                 nullptr, &status);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(status);

  cl_float pattern = 0.0f;
  cl_event fill_event;
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueFillBuffer(command_queue, buffer, &pattern, sizeof(cl_float), 0,
                          sizeof(cl_float), 0, nullptr, &fill_event));
  EXPECT_TRUE(fill_event);
  cl_event barrier_event;
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueBarrierWithWaitList(command_queue, 1, &fill_event,
                                                 &barrier_event));
  EXPECT_TRUE(barrier_event);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_SUCCESS(clReleaseEvent(fill_event));
  ASSERT_SUCCESS(clReleaseEvent(barrier_event));
}

TEST_F(clEnqueueBarrierWithWaitListTest, DefaultNoEventWaitList) {
  cl_int status;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float),
                                 nullptr, &status);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(status);

  cl_float pattern = 0.0f;
  cl_event fill_event;
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueFillBuffer(command_queue, buffer, &pattern, sizeof(cl_float), 0,
                          sizeof(cl_float), 0, nullptr, &fill_event));
  EXPECT_TRUE(fill_event);
  cl_event barrier_event;
  EXPECT_SUCCESS(
      clEnqueueBarrierWithWaitList(command_queue, 0, nullptr, &barrier_event));
  EXPECT_TRUE(barrier_event);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_SUCCESS(clReleaseEvent(fill_event));
  ASSERT_SUCCESS(clReleaseEvent(barrier_event));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueBarrierWithWaitListTest)
