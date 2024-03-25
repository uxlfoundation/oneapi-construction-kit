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

class clCommandBarrierWithWaitListTest : public cl_khr_command_buffer_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    cl_int err = !CL_SUCCESS;
    command_buffer = clCreateCommandBufferKHR(1, &command_queue, nullptr, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(command_buffer != nullptr);
  }

  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
};

TEST_F(clCommandBarrierWithWaitListTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clCommandBarrierWithWaitListKHR(nullptr, nullptr, 0,
                                                    nullptr, nullptr, nullptr));
}

TEST_F(clCommandBarrierWithWaitListTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clCommandBarrierWithWaitListKHR(command_buffer, command_queue, 0, nullptr,
                                      nullptr, nullptr));
}

TEST_F(clCommandBarrierWithWaitListTest, FinalizedCommandBuffer) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 0,
                                                    nullptr, nullptr, nullptr));
}

TEST_F(clCommandBarrierWithWaitListTest, InvalidMutableHandle) {
  cl_mutable_command_khr handle;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 0,
                                                    nullptr, nullptr, &handle));
}

TEST_F(clCommandBarrierWithWaitListTest, InvalidSyncPoints) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 1,
                                                    nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 0, &sync_point,
                                      nullptr, nullptr));
}

TEST_F(clCommandBarrierWithWaitListTest, Default) {
  ASSERT_SUCCESS(clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 0,
                                                 nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
}

TEST_F(clCommandBarrierWithWaitListTest, Sync) {
  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandBarrierWithWaitListKHR(
      command_buffer, nullptr, 0, nullptr, &sync_points[0], nullptr));
  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandBarrierWithWaitListKHR(
      command_buffer, nullptr, 0, nullptr, &sync_points[1], nullptr));
  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandBarrierWithWaitListKHR(
      command_buffer, nullptr, 2, sync_points, nullptr, nullptr));
}

TEST_F(clCommandBarrierWithWaitListTest, FillAndCopy) {
  constexpr size_t elements = 8;
  constexpr size_t data_size = elements * sizeof(cl_uint);
  cl_int error = CL_SUCCESS;
  cl_mem src_buffer =
      clCreateBuffer(context, CL_MEM_READ_ONLY, data_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem dst_buffer =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, data_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Add the fill command to the buffer and finalize it.
  cl_uint pattern = 42;
  EXPECT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, src_buffer,
                                        &pattern, sizeof(pattern), 0, data_size,
                                        0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clCommandBarrierWithWaitListKHR(command_buffer, nullptr, 0,
                                                 nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                        dst_buffer, 0, 0, data_size, 0, nullptr,
                                        nullptr, nullptr));

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  const std::vector<cl_uint> result(elements, 42);
  std::vector<cl_uint> output_data(elements);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size, output_data.data(), 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, output_data);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
}
