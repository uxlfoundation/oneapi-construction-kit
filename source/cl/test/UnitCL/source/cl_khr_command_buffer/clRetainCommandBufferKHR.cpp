// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "cl_khr_command_buffer.h"

using clRetainCommandBufferTest = cl_khr_command_buffer_Test;
TEST_F(clRetainCommandBufferTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clRetainCommandBufferKHR(nullptr));
}
