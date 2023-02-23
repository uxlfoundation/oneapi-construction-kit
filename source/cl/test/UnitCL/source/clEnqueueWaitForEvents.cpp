// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
