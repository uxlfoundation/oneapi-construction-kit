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
