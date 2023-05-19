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

struct CommandBufferCopyBufferTest : cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    cl_int error = CL_SUCCESS;
    src_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, data_size_in_bytes,
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    dst_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, data_size_in_bytes,
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }

    if (dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;

  static constexpr size_t elements = 256;
  static constexpr size_t data_size_in_bytes = elements * sizeof(cl_int);
};

#if __cplusplus < 201703L
// C++14 and below require static member definitions be defined outside the
// class even if they are initialized inline. TODO: Remove condition once we no
// longer support earlier than LLVM 15.
constexpr size_t CommandBufferCopyBufferTest::elements;
constexpr size_t CommandBufferCopyBufferTest::data_size_in_bytes;
#endif

TEST_F(CommandBufferCopyBufferTest, Default) {
  std::vector<cl_int> input_data(elements);

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      data_size_in_bytes, input_data.data(), 0,
                                      nullptr, nullptr));

  ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                        dst_buffer, 0, 0, data_size_in_bytes, 0,
                                        nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Check the results.
  std::vector<cl_int> output_data(elements);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  ASSERT_EQ(input_data, output_data);
}

TEST_F(CommandBufferCopyBufferTest, Sync) {
  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                        dst_buffer, 0, 0, data_size_in_bytes, 0,
                                        nullptr, &sync_points[0], nullptr));
  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                        dst_buffer, 0, 0, data_size_in_bytes, 0,
                                        nullptr, &sync_points[1], nullptr));
  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                        dst_buffer, 0, 0, data_size_in_bytes, 2,
                                        sync_points, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandCopyBufferKHR(nullptr, nullptr, src_buffer, dst_buffer, 0, 0,
                             data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           dst_buffer, 0, 0, data_size_in_bytes,
                                           0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidMemObjects) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyBufferKHR(command_buffer, nullptr, nullptr, dst_buffer, 0, 0,
                             data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer, nullptr, 0, 0,
                             data_size_in_bytes, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidContext) {
  cl_int error = CL_OUT_OF_RESOURCES;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  EXPECT_TRUE(other_context);
  EXPECT_SUCCESS(error);

  cl_mem other_buffer = clCreateBuffer(other_context, CL_MEM_WRITE_ONLY,
                                       data_size_in_bytes, nullptr, &error);
  EXPECT_TRUE(other_buffer);
  EXPECT_SUCCESS(error);

  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clCommandCopyBufferKHR(
                        command_buffer, nullptr, src_buffer, other_buffer, 0, 0,
                        data_size_in_bytes, 0, nullptr, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clCommandCopyBufferKHR(
                        command_buffer, nullptr, other_buffer, other_buffer, 0,
                        0, data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(CommandBufferCopyBufferTest, CopyOverlap) {
  const auto half_size = data_size_in_bytes / 2;
  ASSERT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           src_buffer, 0, 0, data_size_in_bytes,
                                           0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           src_buffer, half_size, 1, half_size,
                                           0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           src_buffer, half_size, 1, half_size,
                                           0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidOffset) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer, dst_buffer,
                             data_size_in_bytes + 1, 0, data_size_in_bytes, 0,
                             nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer, dst_buffer, 0,
                             data_size_in_bytes + 1, data_size_in_bytes, 0,
                             nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           dst_buffer, 0, 1, data_size_in_bytes,
                                           0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           dst_buffer, 1, 0, data_size_in_bytes,
                                           0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidSize) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer, dst_buffer, 0,
                             0, 0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferTest, InvalidSyncPoints) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,

                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           dst_buffer, 0, 0, data_size_in_bytes,
                                           1, nullptr, nullptr, nullptr));
  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                           dst_buffer, 0, 0, data_size_in_bytes,
                                           0, &sync_point, nullptr, nullptr));
}
