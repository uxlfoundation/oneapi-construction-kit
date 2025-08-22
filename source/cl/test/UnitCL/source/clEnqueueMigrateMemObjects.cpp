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

class clEnqueueMigrateMemObjectsTest : public ucl::CommandQueueTest,
                                       TestWithEventWaitList {
protected:
  enum { SIZE = 128 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int errorcode;
    mem = clCreateBuffer(context, 0, SIZE, nullptr, &errorcode);
    EXPECT_TRUE(mem);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (mem) {
      EXPECT_SUCCESS(clReleaseMemObject(mem));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(err,
                      clEnqueueMigrateMemObjects(command_queue, 1, &mem, 0,
                                                 num_events, events, event));
  }

  cl_mem mem = nullptr;
};

TEST_F(clEnqueueMigrateMemObjectsTest, Default) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMigrateMemObjects(command_queue, 1, &mem, 0, 0,
                                            nullptr, &event));
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMigrateMemObjectsTest, WithWaitQueue) {
  cl_int errorcode = !CL_SUCCESS;
  cl_event waitEvent = clCreateUserEvent(context, &errorcode);
  ASSERT_TRUE(waitEvent);
  ASSERT_SUCCESS(errorcode);

  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMigrateMemObjects(command_queue, 1, &mem, 0, 1,
                                            &waitEvent, &event));
  ASSERT_TRUE(event);

  ASSERT_SUCCESS(clSetUserEventStatus(waitEvent, CL_SUCCESS));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(clReleaseEvent(waitEvent));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMigrateMemObjectsTest, MigrateHost) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMigrateMemObjects(
      command_queue, 1, &mem, CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr, &event));
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMigrateMemObjectsTest, MigrateUndefined) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMigrateMemObjects(
      command_queue, 1, &mem, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0,
      nullptr, &event));
  ASSERT_TRUE(event);
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMigrateMemObjectsTest, InvalidCommandQueue) {
  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueMigrateMemObjects(nullptr, 1, &mem, 0, 0, nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMigrateMemObjectsTest, InvalidContext) {
  cl_int errorcode = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(errorcode);

  errorcode = !CL_SUCCESS;
  cl_mem mem = clCreateBuffer(context, 0, SIZE, nullptr, &errorcode);
  EXPECT_TRUE(mem);
  ASSERT_SUCCESS(errorcode);

  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clEnqueueMigrateMemObjects(command_queue, 1, &mem, 0, 0,
                                               nullptr, &event));
  ASSERT_FALSE(event);

  ASSERT_SUCCESS(clReleaseMemObject(mem));
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clEnqueueMigrateMemObjectsTest, InvalidMemObject) {
  cl_mem mem = nullptr;
  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueMigrateMemObjects(command_queue, 1, &mem, 0, 0,
                                               nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMigrateMemObjectsTest, MemObjectsSizedZero) {
  cl_event event = nullptr;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueMigrateMemObjects(command_queue, 0, &mem, 0, 0,
                                               nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMigrateMemObjectsTest, MemObjectsNull) {
  cl_event event = nullptr;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueMigrateMemObjects(command_queue, 1, nullptr, 0, 0,
                                               nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMigrateMemObjectsTest, InvalidFlags) {
  cl_event event = nullptr;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueMigrateMemObjects(command_queue, 1, &mem,
                                 static_cast<cl_mem_migration_flags>(~(
                                     CL_MIGRATE_MEM_OBJECT_HOST |
                                     CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED)),
                                 0, nullptr, &event));
  ASSERT_FALSE(event);
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueMigrateMemObjectsTest)
