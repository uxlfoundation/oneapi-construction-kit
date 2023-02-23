// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "cl_khr_command_buffer.h"

using clFinalizeCommandBufferTest = cl_khr_command_buffer_Test;
TEST_F(clFinalizeCommandBufferTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clFinalizeCommandBufferKHR(nullptr));
}

TEST_F(clFinalizeCommandBufferTest, AlreadyFinalizedCommandBuffer) {
  cl_int error = ~CL_SUCCESS;
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clFinalizeCommandBufferKHR(command_buffer));

  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}
