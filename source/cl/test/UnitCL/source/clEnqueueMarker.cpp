// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vector>

#include "Common.h"

using clEnqueueMarkerTest = ucl::CommandQueueTest;

TEST_F(clEnqueueMarkerTest, Default) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMarker(command_queue, &event));
  ASSERT_TRUE(event);

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMarkerTest, InvalidCommandQueue) {
  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clEnqueueMarker(nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMarkerTest, InvalidEvent) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueMarker(command_queue, nullptr));
}
