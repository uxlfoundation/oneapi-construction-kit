// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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

class clEnqueueUnMapMemObjectTest : public ucl::CommandQueueTest,
                                    TestWithEventWaitList {
 protected:
  enum { SIZE = 128, INT_SIZE = (SIZE * sizeof(cl_int)) };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errorcode;
    inMem = clCreateBuffer(context, 0, INT_SIZE, nullptr, &errorcode);
    EXPECT_TRUE(inMem);
    ASSERT_SUCCESS(errorcode);
    for (cl_uint i = 0; i < SIZE; i++) {
      inBuffer[i] = i;
    }
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, CL_TRUE, 0,
                                        INT_SIZE, inBuffer, 0, nullptr,
                                        &writeEvent));
  }

  void TearDown() override {
    if (writeEvent) {
      EXPECT_SUCCESS(clReleaseEvent(writeEvent));
    }
    if (mapEvent) {
      EXPECT_SUCCESS(clReleaseEvent(mapEvent));
    }
    if (unMapEvent) {
      EXPECT_SUCCESS(clReleaseEvent(unMapEvent));
    }
    if (readEvent) {
      EXPECT_SUCCESS(clReleaseEvent(readEvent));
    }
    if (inMem) {
      EXPECT_SUCCESS(clReleaseMemObject(inMem));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    cl_int errcode = !CL_SUCCESS;
    void *map =
        clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                           INT_SIZE, 1, &writeEvent, nullptr, &errcode);
    EXPECT_TRUE(map);
    ASSERT_SUCCESS(errcode);

    EXPECT_EQ_ERRCODE(err, clEnqueueUnmapMemObject(command_queue, inMem, map,
                                                   num_events, events, event));

    ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0,
                                           nullptr, &unMapEvent));

    ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));
  }

  cl_mem inMem = nullptr;
  cl_event writeEvent = nullptr;
  cl_event mapEvent = nullptr;
  cl_event unMapEvent = nullptr;
  cl_event readEvent = nullptr;
  int inBuffer[SIZE] = {};
};

TEST_F(clEnqueueUnMapMemObjectTest, Default) {
  cl_int errcode = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                                 INT_SIZE, 1, &writeEvent, nullptr, &errcode);
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueUnMapMemObjectTest, InvalidCommandQueue) {
  cl_int errcode = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                                 INT_SIZE, 1, &writeEvent, nullptr, &errcode);
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueUnmapMemObject(nullptr, inMem, map, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));
  ASSERT_TRUE(unMapEvent);

  ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueUnMapMemObjectTest, CommandQueueIsInDifferentContext) {
  cl_int errorCode = !CL_SUCCESS;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorCode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errorCode);

  cl_command_queue otherQueue =
      clCreateCommandQueue(otherContext, device, 0, &errorCode);
  EXPECT_TRUE(otherQueue);
  ASSERT_SUCCESS(errorCode);

  void *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                                 INT_SIZE, 1, &writeEvent, nullptr, &errorCode);
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errorCode);

  cl_event unMapEvent = nullptr;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueUnmapMemObject(otherQueue, inMem, map, 0, nullptr, &unMapEvent));
  ASSERT_FALSE(unMapEvent);

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));
  ASSERT_TRUE(unMapEvent);

  ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));

  ASSERT_SUCCESS(clReleaseEvent(unMapEvent));

  ASSERT_SUCCESS(clReleaseCommandQueue(otherQueue));
  ASSERT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueUnMapMemObjectTest, InvalidMemObject) {
  cl_int errcode = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                                 INT_SIZE, 1, &writeEvent, nullptr, &errcode);
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueUnmapMemObject(command_queue, nullptr, map, 0,
                                            nullptr, nullptr));

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));
  ASSERT_TRUE(unMapEvent);

  ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueUnMapMemObjectTest, InvalidMappedPtrIsNull) {
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueUnmapMemObject(command_queue, inMem, nullptr, 0,
                                            nullptr, &unMapEvent));
  ASSERT_FALSE(unMapEvent);
}

TEST_F(clEnqueueUnMapMemObjectTest, InvalidMappedPtr) {
  cl_int errcode = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                                 INT_SIZE, 1, &writeEvent, nullptr, &errcode);
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errcode);

  int foo;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueUnmapMemObject(command_queue, inMem, &foo, 0,
                                            nullptr, &unMapEvent));
  ASSERT_FALSE(unMapEvent);

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));
  ASSERT_TRUE(unMapEvent);

  ASSERT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueUnMapMemObjectTest, InvalidMappedPtrWithInvalidMemObject) {
  int foo;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueUnmapMemObject(command_queue, inMem, &foo, 0,
                                            nullptr, &unMapEvent));
  ASSERT_FALSE(unMapEvent);
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueUnMapMemObjectTest)
