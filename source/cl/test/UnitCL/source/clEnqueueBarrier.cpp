// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vector>

#include "Common.h"

using clEnqueueBarrierTest = ucl::CommandQueueTest;

TEST_F(clEnqueueBarrierTest, default) {
  ASSERT_SUCCESS(clEnqueueBarrier(command_queue));
}

TEST_F(clEnqueueBarrierTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clEnqueueBarrier(nullptr));
}
