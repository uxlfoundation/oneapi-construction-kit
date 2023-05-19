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

#include <vector>

#include "Common.h"

using clEnqueueWaitForEventsTest = ucl::CommandQueueTest;

TEST_F(clEnqueueWaitForEventsTest, Default) {
  cl_int errorcode = !CL_SUCCESS;

  cl_event event = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clEnqueueWaitForEvents(command_queue, 1, &event));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWaitForEventsTest, InvalidCommandQueue) {
  cl_int errorcode = !CL_SUCCESS;

  cl_event event = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueWaitForEvents(nullptr, 1, &event));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWaitForEventsTest, InvalidContext) {
  cl_int errorcode = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
  ASSERT_TRUE(context);
  ASSERT_SUCCESS(errorcode);

  errorcode = !CL_SUCCESS;
  cl_event event = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clEnqueueWaitForEvents(command_queue, 1, &event));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clEnqueueWaitForEventsTest, NumEventsZero) {
  cl_int errorcode = !CL_SUCCESS;

  cl_event event = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWaitForEvents(command_queue, 0, &event));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWaitForEventsTest, EventListNull) {
  cl_int errorcode = !CL_SUCCESS;

  cl_event event = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWaitForEvents(command_queue, 1, nullptr));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWaitForEventsTest, EventListElementInvalid) {
  cl_event event = nullptr;

  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT,
                    clEnqueueWaitForEvents(command_queue, 1, &event));
}
