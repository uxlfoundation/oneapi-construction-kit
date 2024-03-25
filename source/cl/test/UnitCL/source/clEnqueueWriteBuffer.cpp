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

#include <vector>

#include "Common.h"
#include "EventWaitList.h"

class clEnqueueWriteBufferTest : public ucl::CommandQueueTest,
                                 TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errorcode;
    mem = clCreateBuffer(context, 0, size, nullptr, &errorcode);
    EXPECT_TRUE(mem);
    ASSERT_SUCCESS(errorcode);
    buffer.assign(size, 0);
  }

  void TearDown() override {
    if (mem) {
      EXPECT_SUCCESS(clReleaseMemObject(mem));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(
        err, clEnqueueWriteBuffer(command_queue, mem, CL_TRUE, 0, size,
                                  buffer.data(), num_events, events, event));
  }

  size_t size = 128;
  cl_mem mem = nullptr;
  UCL::vector<char> buffer;
};

TEST_F(clEnqueueWriteBufferTest, Default) {
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, mem, true, 0, size,
                                      buffer.data(), 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, NonBlocking) {
  cl_event event;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, mem, false, 0, size,
                                      buffer.data(), 0, nullptr, &event));
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(clWaitForEvents(1, &event));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWriteBufferTest, ChainTwo) {
  cl_event event;
  cl_int errorcode;
  cl_mem otherMem = clCreateBuffer(context, 0, size, nullptr, &errorcode);
  EXPECT_TRUE(otherMem);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, otherMem, false, 0, size,
                                      buffer.data(), 0, nullptr, &event));
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, mem, true, 0, size,
                                      buffer.data(), 1, &event, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherMem));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWriteBufferTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueWriteBuffer(nullptr, mem, true, 0, size,
                                         buffer.data(), 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, InvalidMemObject) {
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueWriteBuffer(command_queue, nullptr, true, 0, size,
                                         buffer.data(), 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, InvalidBufferSize) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWriteBuffer(command_queue, mem, true, size, size,
                                         buffer.data(), 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, InvalidBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWriteBuffer(command_queue, mem, true, 0, size,
                                         nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, InvalidSize) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWriteBuffer(command_queue, mem, true, 0, 0,
                                         buffer.data(), 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferTest, WriteToReadOnly) {
  cl_int errorcode;
  cl_mem otherMem =
      clCreateBuffer(context, CL_MEM_HOST_READ_ONLY, size, nullptr, &errorcode);
  EXPECT_TRUE(otherMem);
  EXPECT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clEnqueueWriteBuffer(command_queue, otherMem, true, 0, size,
                                         buffer.data(), 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherMem));
}

TEST_F(clEnqueueWriteBufferTest, WriteToHostNoAccess) {
  cl_int errorcode;
  cl_mem otherMem =
      clCreateBuffer(context, CL_MEM_HOST_NO_ACCESS, size, nullptr, &errorcode);
  EXPECT_TRUE(otherMem);
  EXPECT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clEnqueueWriteBuffer(command_queue, otherMem, true, 0, size,
                                         buffer.data(), 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherMem));
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueWriteBufferTest)
