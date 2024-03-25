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

class clEnqueueCopyBufferRectTest : public ucl::CommandQueueTest,
                                    TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    src_origin.resize(3, 0);
    dst_origin.resize(3, 0);
    region.resize(3, 32);
    size = region[0] * region[1] * region[2];
    src_row_pitch = region[0];
    src_slice_pitch = region[1] * src_row_pitch;
    dst_row_pitch = region[0];
    dst_slice_pitch = region[1] * dst_row_pitch;

    cl_int errcode;
    src_buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
    EXPECT_TRUE(src_buffer);
    ASSERT_SUCCESS(errcode);
    dst_buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
    EXPECT_TRUE(dst_buffer);
    ASSERT_SUCCESS(errcode);
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
        err,
        clEnqueueCopyBufferRect(
            command_queue, src_buffer, dst_buffer, src_origin.data(),
            dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
            dst_row_pitch, dst_slice_pitch, num_events, events, event));
  }

  UCL::vector<size_t> src_origin;
  UCL::vector<size_t> dst_origin;
  UCL::vector<size_t> region;
  size_t size = 0;
  size_t src_row_pitch = 0;
  size_t src_slice_pitch = 0;
  size_t dst_row_pitch = 0;
  size_t dst_slice_pitch = 0;

  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
};

TEST_F(clEnqueueCopyBufferRectTest, Default) {
  ASSERT_SUCCESS(clEnqueueCopyBufferRect(
      command_queue, src_buffer, dst_buffer, src_origin.data(),
      dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
      dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, ZeroPitch) {
  src_row_pitch = 0;
  src_slice_pitch = 0;
  dst_row_pitch = 0;
  dst_slice_pitch = 0;

  ASSERT_SUCCESS(clEnqueueCopyBufferRect(
      command_queue, src_buffer, dst_buffer, src_origin.data(),
      dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
      dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueCopyBufferRect(
          nullptr, src_buffer, dst_buffer, src_origin.data(), dst_origin.data(),
          region.data(), src_row_pitch, src_slice_pitch, dst_row_pitch,
          dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidContext) {
  cl_int errcode;

  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(otherContext);
  EXPECT_SUCCESS(errcode);

  cl_mem otherBuffer = clCreateBuffer(otherContext, 0, size, nullptr, &errcode);
  EXPECT_TRUE(otherBuffer);
  EXPECT_SUCCESS(errcode);

  cl_event event = clCreateUserEvent(otherContext, &errcode);
  EXPECT_TRUE(event);
  EXPECT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyBufferRect(
          command_queue, otherBuffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, otherBuffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 1, &event, nullptr));

  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseMemObject(otherBuffer));
  EXPECT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidMemObject) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueCopyBufferRect(
          command_queue, nullptr, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, nullptr, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueCopyBufferRect(
          command_queue, nullptr, nullptr, src_origin.data(), dst_origin.data(),
          region.data(), src_row_pitch, src_slice_pitch, dst_row_pitch,
          dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueSrcOrigin) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, nullptr,
                              dst_origin.data(), region.data(), src_row_pitch,
                              src_slice_pitch, dst_row_pitch, dst_slice_pitch,
                              0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueDstOrigin) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer,
                              src_origin.data(), nullptr, region.data(),
                              src_row_pitch, src_slice_pitch, dst_row_pitch,
                              dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueRegion) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer,
                              src_origin.data(), dst_origin.data(), nullptr,
                              src_row_pitch, src_slice_pitch, dst_row_pitch,
                              dst_slice_pitch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueOutOfBounds) {
  src_origin.assign(3, 33);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  src_origin.assign(3, 0);

  dst_origin.assign(3, 33);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  dst_origin.assign(3, 0);

  region.assign(3, 33);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  region.assign(3, 32);
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueRegionElementZero) {
  region[0] = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  region[0] = 32;

  region[1] = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  region[1] = 32;

  region[2] = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  region[2] = 32;
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueRowPitch) {
  src_row_pitch = 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  src_row_pitch = region[0] - 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  src_row_pitch = 0;

  dst_row_pitch = 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  dst_row_pitch = region[0] - 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  dst_row_pitch = 0;
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueSlicePitch) {
  src_slice_pitch = 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  src_slice_pitch = (region[1] * src_row_pitch) - 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  src_slice_pitch = 0;

  dst_slice_pitch = 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));

  dst_slice_pitch = (region[1] * dst_row_pitch) - 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  dst_slice_pitch = 0;
}

TEST_F(clEnqueueCopyBufferRectTest, InvalidValueSameBufferPitchMismatch) {
  dst_row_pitch -= 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, src_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  dst_row_pitch += 1;

  dst_slice_pitch -= 1;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferRect(
          command_queue, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
          dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr));
  dst_slice_pitch += 1;
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueCopyBufferRectTest)

/* Redmine #5139: Write additional tests in this list when supported
 * CL_MEM_COPY_OVERLAP
 * CL_MISALIGNED_SUB_BUFFER_OFFSET
 * CL_MEM_OBJECT_ALLOCATION_FAILURE
 * CL_OUT_OF_RESOURCES
 * CL_OUT_OF_HOST_MEMORY
 */
