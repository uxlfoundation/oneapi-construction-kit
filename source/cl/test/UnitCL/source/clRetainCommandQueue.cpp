// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

using clRetainCommandQueueTest = ucl::CommandQueueTest;

TEST_F(clRetainCommandQueueTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clRetainCommandQueue(nullptr));
}

TEST_F(clRetainCommandQueueTest, Default) {
  ASSERT_SUCCESS(clRetainCommandQueue(command_queue));
  ASSERT_SUCCESS(clReleaseCommandQueue(command_queue));
}
