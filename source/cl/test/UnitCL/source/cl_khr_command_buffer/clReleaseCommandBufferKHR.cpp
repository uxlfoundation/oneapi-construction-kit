// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "cl_khr_command_buffer.h"

using clReleaseCommandBufferTest = cl_khr_command_buffer_Test;
TEST_F(clReleaseCommandBufferTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clReleaseCommandBufferKHR(nullptr));
}

// Test the ordering of releasing a command-buffer after
// releasing its queue.
TEST_F(clReleaseCommandBufferTest, ReleaseAfterQueue) {
  cl_int error = ~CL_SUCCESS;
  cl_command_queue test_queue =
      clCreateCommandQueue(context, device, 0, &error);
  ASSERT_SUCCESS(error);

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &test_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clReleaseCommandQueue(test_queue));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}
