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

#include <array>

#include "Common.h"
#include "EventWaitList.h"

template <typename T>
struct BufferRAII {
  T *payload;
  BufferRAII(const unsigned int size) : payload(new T[size]) {}
  ~BufferRAII() { delete[] payload; }
};

class clEnqueueFillBufferTest : public ucl::CommandQueueTest,
                                TestWithEventWaitList {
 protected:
  enum { size = 128 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errcode;
    buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
    ASSERT_SUCCESS(errcode);
    ASSERT_NE(nullptr, buffer);
    std::array<cl_uint, 4> pattern_data = {0u, 1u, 2u, 3u};
    memcpy(&pattern, pattern_data.data(), sizeof(pattern));
    pattern_size = sizeof(cl_uint4);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(
        err, clEnqueueFillBuffer(command_queue, buffer, &pattern, pattern_size,
                                 0, size, num_events, events, event));
  }

  cl_mem buffer = nullptr;
  cl_uint4 pattern = {};
  size_t pattern_size = 0;
};

TEST_F(clEnqueueFillBufferTest, NullCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueFillBuffer(nullptr, buffer, &pattern, pattern_size,
                                        0, size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, CommandQueueBufferContextMismatch) {
  cl_int errcode;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errcode);

  cl_mem otherBuffer = clCreateBuffer(otherContext, 0, size, nullptr, &errcode);
  EXPECT_TRUE(otherBuffer);
  ASSERT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueFillBuffer(command_queue, otherBuffer, &pattern, pattern_size, 0,
                          size, 0, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(otherBuffer));
  EXPECT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueFillBufferTest, NullBuffer) {
  cl_uint pattern[4] = {1u, 2u, 3u, 4u};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueFillBuffer(command_queue, nullptr, &pattern, pattern_size, 0,
                          size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, OffsetRangeOutOfBounds) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueFillBuffer(command_queue, buffer, &pattern, pattern_size,
                          size + 1, size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, OffsetSizeRangeOutOfBounds) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueFillBuffer(command_queue, buffer, &pattern, pattern_size,
                          (size / 2) + 1, size / 2, 0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, NullPattern) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueFillBuffer(command_queue, buffer, nullptr, pattern_size, 0, size,
                          0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, ZeroPatternSize) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueFillBuffer(command_queue, buffer, &pattern, 0, 0,
                                        size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueFillBufferTest, BadPatternSizes) {
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueFillBuffer(command_queue, buffer, &pattern, 3, 0,
                                        size, 0, nullptr, nullptr));
  for (size_t i = 5; i < 8; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
  for (size_t i = 9; i < 16; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
  for (size_t i = 17; i < 32; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
  for (size_t i = 33; i < 64; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
  for (size_t i = 65; i < 128; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
  for (size_t i = 129; i < 1024; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clEnqueueFillBuffer(command_queue, buffer, &pattern, i, 0,
                                          size, 0, nullptr, nullptr));
  }
}

TEST_F(clEnqueueFillBufferTest, OffsetNotMultipleOfPatternSize) {
  for (size_t i = 1; i < pattern_size; ++i) {
    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueFillBuffer(command_queue, buffer, &pattern, pattern_size, i,
                            size, 0, nullptr, nullptr));
  }
  for (size_t i = pattern_size + 1; i < pattern_size * 2; ++i) {
    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clEnqueueFillBuffer(command_queue, buffer, &pattern, pattern_size, i,
                            size, 0, nullptr, nullptr));
  }
}

// Redmine #5120: Test CL_MISALIGNED_SUB_BUFFER_OFFSET

TEST_F(clEnqueueFillBufferTest, Default) {
  cl_event event;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, buffer, &pattern,
                                     pattern_size, 0, size, 0, nullptr,
                                     &event));
  ASSERT_TRUE(event);

  const BufferRAII<cl_uint4> data(size / sizeof(cl_uint4));
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_FALSE, 0, size,
                                     data.payload, 1, &event, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));
  for (size_t i = 0; i < size / pattern_size; ++i) {
    EXPECT_EQ(0u, data.payload[i].s[0]);
    EXPECT_EQ(1u, data.payload[i].s[1]);
    EXPECT_EQ(2u, data.payload[i].s[2]);
    EXPECT_EQ(3u, data.payload[i].s[3]);
  }

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueFillBufferTest, CopyPatternData) {
  cl_int err;
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(user_event);

  cl_event event;
  cl_float pattern_d = 0.5f;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, buffer, &pattern_d,
                                     sizeof(cl_float), 0, size, 1, &user_event,
                                     &event));
  ASSERT_TRUE(event);

  // Change the pattern data and mark our user event as complete
  pattern_d = 1.5f;
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  cl_float result;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     sizeof(cl_float), &result, 1, &event,
                                     nullptr));
  EXPECT_EQ(0.5, result);

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseEvent(user_event));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueFillBufferTest)
