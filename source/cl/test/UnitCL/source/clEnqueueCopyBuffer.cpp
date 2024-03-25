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

#include "Common.h"
#include "EventWaitList.h"

class clEnqueueCopyBufferCheckTest : public ucl::CommandQueueTest {
 protected:
  enum { buffer_size = 4 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errcode;
    src_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr,
                                &errcode);
    EXPECT_TRUE(src_buffer);
    ASSERT_SUCCESS(errcode);
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                        buffer_size, src_data, 0, nullptr,
                                        nullptr));
  }

  void TearDown() override {
    if (src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }
    CommandQueueTest::TearDown();
  }

  char src_data[buffer_size] = {};
  cl_mem src_buffer = nullptr;
};

class clEnqueueCopyBufferTest : public ucl::CommandQueueTest,
                                TestWithEventWaitList {
 protected:
  enum { elements = 10, buffer_size = 40 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errcode;
    for (size_t i = 0; i < elements; ++i) {
      src_data[i] = 42;
    }
    src_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr,
                                &errcode);
    EXPECT_TRUE(src_buffer);
    ASSERT_SUCCESS(errcode);
    dst_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size,
                                nullptr, &errcode);
    EXPECT_TRUE(dst_buffer);
    ASSERT_SUCCESS(errcode);
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                        buffer_size, src_data, 0, nullptr,
                                        nullptr));
  }

  void TearDown() override {
    if (src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }
    if (dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(
        err, clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, 0, 0,
                                 buffer_size, num_events, events, event));
  }

  char src_data[buffer_size] = {};
  char dst_data[buffer_size] = {};
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
};

TEST_F(clEnqueueCopyBufferCheckTest, NullCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueCopyBuffer(nullptr, nullptr, nullptr, 0, 0, 0, 0,
                                        nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, NullMemBuffers) {
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueCopyBuffer(command_queue, nullptr, src_buffer, 0,
                                        0, buffer_size, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueCopyBuffer(command_queue, src_buffer, nullptr, 0,
                                        0, buffer_size, 0, nullptr, nullptr));
  ASSERT_EQ(CL_INVALID_MEM_OBJECT,
            clEnqueueCopyBuffer(command_queue, nullptr, nullptr, 0, 0,
                                buffer_size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, BufferContextMismatch) {
  cl_int errcode = CL_OUT_OF_RESOURCES;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errcode);

  cl_mem dst_buffer = clCreateBuffer(otherContext, CL_MEM_WRITE_ONLY,
                                     buffer_size, nullptr, &errcode);
  EXPECT_TRUE(dst_buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, 0, 0,
                          buffer_size, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(dst_buffer));
  ASSERT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueCopyBufferCheckTest, BufferComandQueueContextMismatch) {
  cl_int errcode = CL_OUT_OF_RESOURCES;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errcode);

  cl_mem buffer = clCreateBuffer(otherContext, CL_MEM_READ_ONLY, buffer_size,
                                 nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clEnqueueCopyBuffer(command_queue, buffer, buffer, 0, 0,
                                        buffer_size, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueCopyBufferCheckTest, CopyOverlap) {
  static_assert(buffer_size == 4, "This test assumes a buffer size of 4");

  // src_buffer [0, 4) -> src_buffer [0, 4)
  // OVERLAP!
  ASSERT_EQ_ERRCODE(
      CL_MEM_COPY_OVERLAP,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, 0, 0,
                          buffer_size, 0, nullptr, nullptr));

  auto halfSize = buffer_size / 2;

  // src_buffer [1, 3) -> src_buffer [2, 4)
  // OVERLAP!
  ASSERT_EQ_ERRCODE(
      CL_MEM_COPY_OVERLAP,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, 1, halfSize,
                          halfSize, 0, nullptr, nullptr));

  // src_buffer [2, 4) -> src_buffer [1, 3)
  // OVERLAP!
  ASSERT_EQ_ERRCODE(
      CL_MEM_COPY_OVERLAP,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, halfSize, 1,
                          halfSize, 0, nullptr, nullptr));

  // src_buffer [2, 3) -> src_buffer [3, 4)
  // NO OVERLAP!
  ASSERT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer,
                                     halfSize, halfSize + 1, halfSize - 1, 0,
                                     nullptr, nullptr));

  // src_buffer [3, 4) -> src_buffer [2, 3)
  // NO OVERLAP!
  ASSERT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer,
                                     halfSize + 1, halfSize, halfSize - 1, 0,
                                     nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, SubBufferCopyOverlap) {
  static_assert(buffer_size == 4, "This test assumes a buffer size of 4");

  const size_t halfSize = buffer_size / 2;
  cl_buffer_region region = {0, halfSize};
  cl_int error;

  // src_buffer --> [0, 4)
  // subBuffer  --> [0, 2) of src_buffer
  cl_mem subBuffer =
      clCreateSubBuffer(src_buffer, CL_MEM_READ_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &error);
  ASSERT_SUCCESS(error);

  // Copy src_buffer [2, 4) -> subBuffer [0, 2)
  //  <=> src_buffer [2, 4) -> src_buffer [0, 2)
  // NO OVERLAP!
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, subBuffer,
                                     halfSize, 0, halfSize, 0, nullptr,
                                     nullptr));

  // Copy src_buffer [1, 2) -> subBuffer [0, 1)
  //  <=> src_buffer [1, 2) -> src_buffer [0, 1)
  // NO OVERLAP!
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, subBuffer, 1, 0,
                                     halfSize - 1, 0, nullptr, nullptr));

  // Copy subBuffer [0, 2) -> src_buffer [0, 2)
  //  <=> src_buffer [0, 2) -> src_buffer [0, 2)
  // OVERLAP!
  EXPECT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clEnqueueCopyBuffer(command_queue, subBuffer, src_buffer, 0,
                                        0, halfSize, 0, nullptr, nullptr));

  // Copy subBuffer [1, 2) -> src_buffer [2, 3)
  //  <=> src_buffer [1, 2) -> src_buffer [2, 3)
  // NO OVERLAP!
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, subBuffer, src_buffer, 1,
                                     halfSize, halfSize - 1, 0, nullptr,
                                     nullptr));

  // Copy subBuffer [0, 2) -> src_buffer [1, 3)
  //  <=> src_buffer [0, 2) -> src_buffer [1, 3)
  // OVERLAP!
  EXPECT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clEnqueueCopyBuffer(command_queue, subBuffer, src_buffer, 0,
                                        1, halfSize, 0, nullptr, nullptr));

  // Copy src_buffer [1, 3) -> subBuffer [0, 2)
  //  <=> src_buffer [1, 3) -> src_buffer [0, 2)
  // OVERLAP!
  EXPECT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clEnqueueCopyBuffer(command_queue, src_buffer, subBuffer, 1,
                                        0, halfSize, 0, nullptr, nullptr));

  // Copy subBuffer [0, 2) -> subBuffer [0, 2)
  //  <=> src_buffer [0, 2) -> src_buffer [0, 2)
  // OVERLAP!
  EXPECT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clEnqueueCopyBuffer(command_queue, subBuffer, subBuffer, 0,
                                        0, halfSize, 0, nullptr, nullptr));

  // Copy subBuffer [0, 1) -> subBuffer [0, 1)
  //  <=> src_buffer [0, 1) -> src_buffer [0, 1)
  // OVERLAP!
  EXPECT_EQ_ERRCODE(CL_MEM_COPY_OVERLAP,
                    clEnqueueCopyBuffer(command_queue, subBuffer, subBuffer, 0,
                                        0, halfSize - 1, 0, nullptr, nullptr));

  // Copy subBuffer [0, 1) -> subBuffer [1, 2)
  //  <=> src_buffer [0, 1) -> src_buffer [1, 2)
  // NO OVERLAP!
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, subBuffer, subBuffer, 0, 1,
                                     halfSize - 1, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

TEST_F(clEnqueueCopyBufferCheckTest, src_bufferOffsetTooLarge) {
  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer,
                                buffer_size + 1, 0, buffer_size, 0, nullptr,
                                nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, dst_bufferOffsetTooLarge) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, 0,
                          buffer_size + 1, buffer_size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, src_bufferSizePlusOffsetTooLarge) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, 1, 0,
                          buffer_size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, dst_bufferSizePlusOffsetTooLarge) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer, 0, 1,
                          buffer_size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferCheckTest, BufferSizeZero) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueCopyBuffer(command_queue, src_buffer, src_buffer,
                                        0, 0, 0, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferTest, CopyBufferNoEvents) {
  ASSERT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, 0,
                                     0, buffer_size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferTest, VarifyCopiedData) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, 0,
                                     0, buffer_size, 0, nullptr, &event));
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     buffer_size, dst_data, 1, &event,
                                     nullptr));

  for (size_t i = 0; i < elements; ++i) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }

  ASSERT_SUCCESS(clReleaseEvent(event));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueCopyBufferTest)
