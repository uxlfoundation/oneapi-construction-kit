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

namespace {
enum {
  DIMENSIONS2D = 2,
  DIMENSIONS = DIMENSIONS2D + 1,
  QUARTER_DIMENSION_LENGTH = 32,
  HALF_DIMENSION_LENGTH = QUARTER_DIMENSION_LENGTH + QUARTER_DIMENSION_LENGTH,
  DIMENSION_LENGTH = HALF_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH,
  TOTAL_LENGTH = DIMENSION_LENGTH * DIMENSION_LENGTH * DIMENSION_LENGTH
};

static const cl_uchar INITIAL_BUFFER_DATA = 0xFF;
}  // namespace

class clEnqueueWriteBufferRectTest : public ucl::CommandQueueTest,
                                     TestWithEventWaitList {
 protected:
  cl_uchar write_data[TOTAL_LENGTH] = {};
  cl_uchar buffer_data[TOTAL_LENGTH] = {};
  cl_mem buffer = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
      for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
        for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
          const unsigned int index =
              x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
          write_data[index] = static_cast<cl_char>(index);
          buffer_data[index] = INITIAL_BUFFER_DATA;
        }
      }
    }
    cl_int errorcode;
    buffer =
        clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_char) * TOTAL_LENGTH, buffer_data, &errorcode);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
    const size_t host_origin[DIMENSIONS] = {0, 0, 0};
    const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                       HALF_DIMENSION_LENGTH, 1};

    const size_t buffer_row_pitch = DIMENSION_LENGTH;
    const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
    const size_t host_row_pitch = DIMENSION_LENGTH;
    const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

    ASSERT_EQ_ERRCODE(
        err, clEnqueueWriteBufferRect(
                 command_queue, buffer, CL_TRUE, buffer_origin, host_origin,
                 region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
                 host_slice_pitch, write_data, num_events, events, event));
  }
};

TEST_F(clEnqueueWriteBufferRectTest, NullCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueWriteBufferRect(
                        nullptr, nullptr, CL_FALSE, nullptr, nullptr, nullptr,
                        0, 0, 0, 0, write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, NullBuffer) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueWriteBufferRect(command_queue, nullptr, CL_FALSE, buffer_origin,
                               host_origin, region, 0, 0, 0, 0, write_data, 0,
                               nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, NullPtr) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE, buffer_origin,
                               host_origin, region, buffer_row_pitch,
                               buffer_slice_pitch, host_row_pitch,
                               host_slice_pitch, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidRegion) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, nullptr,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidBufferOrigin) {
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, nullptr, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidHostOrigin) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, nullptr, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, RegionElementZero) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  region[0] = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
  region[1] = 0;
  region[0] = 8 * sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
  region[2] = 0;
  region[1] = 8 * sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidPitchValues) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  // Not zero, less than region[0]
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE, buffer_origin,
                               host_origin, region, buffer_row_pitch - 1, 0, 0,
                               0, write_data, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE, buffer_origin,
                               host_origin, region, 0, buffer_slice_pitch - 1,
                               0, 0, write_data, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE, buffer_origin,
                               host_origin, region, 0, 0, host_row_pitch - 1, 0,
                               write_data, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE,
                                             buffer_origin, host_origin, region,
                                             0, 0, 0, host_slice_pitch - 1,
                                             write_data, 0, nullptr, nullptr));

  // One more than valid, not multiple
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE, buffer_origin,
                               host_origin, region, 0, buffer_slice_pitch + 1,
                               0, 0, write_data, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueWriteBufferRect(command_queue, buffer, CL_FALSE,
                                             buffer_origin, host_origin, region,
                                             0, 0, 0, host_slice_pitch + 1,
                                             write_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidEventWaitList) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_EVENT_WAIT_LIST,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 1, nullptr, nullptr));

  cl_int errcode;
  cl_event event = clCreateUserEvent(context, &errcode);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(errcode);

  ASSERT_EQ_ERRCODE(
      CL_INVALID_EVENT_WAIT_LIST,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, &event, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidOperation) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  cl_int errcode;
  cl_mem host_read_only_buffer = clCreateBuffer(
      context, CL_MEM_HOST_READ_ONLY, TOTAL_LENGTH, nullptr, &errcode);
  EXPECT_TRUE(host_read_only_buffer);
  EXPECT_SUCCESS(errcode);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueWriteBufferRect(
          command_queue, host_read_only_buffer, CL_TRUE, buffer_origin,
          host_origin, region, buffer_row_pitch, buffer_slice_pitch,
          host_row_pitch, host_slice_pitch, write_data, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(host_read_only_buffer));

  cl_mem hostNoAccessBuffer = clCreateBuffer(context, CL_MEM_HOST_READ_ONLY,
                                             TOTAL_LENGTH, nullptr, &errcode);
  EXPECT_TRUE(hostNoAccessBuffer);
  EXPECT_SUCCESS(errcode);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueWriteBufferRect(
          command_queue, hostNoAccessBuffer, CL_FALSE, buffer_origin,
          host_origin, region, buffer_row_pitch, buffer_slice_pitch,
          host_row_pitch, host_slice_pitch, write_data, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(hostNoAccessBuffer));
}

TEST_F(clEnqueueWriteBufferRectTest, InvalidContext) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  cl_int errcode;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_NE(nullptr, other_context);
  EXPECT_SUCCESS(errcode);

  cl_mem other_buffer = clCreateBuffer(other_context, CL_MEM_READ_WRITE,
                                       TOTAL_LENGTH, nullptr, &errcode);
  EXPECT_NE(nullptr, other_buffer);
  EXPECT_SUCCESS(errcode);

  // check command_queue/buffer context mismatch
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueWriteBufferRect(
          command_queue, other_buffer, CL_FALSE, buffer_origin, host_origin,
          region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 0, nullptr, nullptr));

  cl_event event = clCreateUserEvent(other_context, &errcode);
  EXPECT_TRUE(event);
  EXPECT_SUCCESS(errcode);

  // check command_queue/event_wait_list context mismatch
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueWriteBufferRect(
          command_queue, buffer, CL_TRUE, buffer_origin, host_origin, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
          host_slice_pitch, write_data, 1, &event, nullptr));

  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  ASSERT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteFull2D) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH, 1};

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region, 0, 0,
      0, 0, write_data, 0, nullptr, &write_event));
  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(buffer_data[index], write_data[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteStart2D) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));
  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < HALF_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < HALF_DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(buffer_data[index], write_data[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  for (unsigned int x = HALF_DIMENSION_LENGTH; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = HALF_DIMENSION_LENGTH; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteMiddle2D) {
  const size_t buffer_origin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                            QUARTER_DIMENSION_LENGTH, 0};
  const size_t host_origin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                          QUARTER_DIMENSION_LENGTH, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < QUARTER_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < QUARTER_DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  for (unsigned int x = QUARTER_DIMENSION_LENGTH;
       x < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH; x++) {
    for (unsigned int y = QUARTER_DIMENSION_LENGTH;
         y < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(buffer_data[index], write_data[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  for (unsigned int x = QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH;
       x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH;
         y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteEnd2D) {
  const size_t buffer_origin[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                            HALF_DIMENSION_LENGTH, 0};
  const size_t host_origin[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                          HALF_DIMENSION_LENGTH, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = host_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < HALF_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < HALF_DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  for (unsigned int x = HALF_DIMENSION_LENGTH; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = HALF_DIMENSION_LENGTH; y < DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(buffer_data[index], write_data[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_BUFFER_DATA, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteFull3D) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region, 0, 0,
      0, 0, write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(buffer_data[index], write_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteStart3D) {
  const size_t buffer_origin[DIMENSIONS] = {0, 0, 0};
  const size_t host_origin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool in_region = (x < HALF_DIMENSION_LENGTH) &&
                               (y < HALF_DIMENSION_LENGTH) &&
                               (z < HALF_DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compare_value =
            (in_region) ? buffer_data[index] : INITIAL_BUFFER_DATA;
        ASSERT_EQ(compare_value, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteEnd3D) {
  const size_t buffer_origin[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};
  const size_t host_origin[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool in_region =
            (HALF_DIMENSION_LENGTH <= x) && (x < DIMENSION_LENGTH) &&
            (HALF_DIMENSION_LENGTH <= y) && (y < DIMENSION_LENGTH) &&
            (HALF_DIMENSION_LENGTH <= z) && (z < DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compare_value =
            (in_region) ? buffer_data[index] : INITIAL_BUFFER_DATA;
        ASSERT_EQ(compare_value, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

TEST_F(clEnqueueWriteBufferRectTest, WriteMiddle3D) {
  const size_t buffer_origin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                            QUARTER_DIMENSION_LENGTH,
                                            QUARTER_DIMENSION_LENGTH};
  const size_t host_origin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                          QUARTER_DIMENSION_LENGTH,
                                          QUARTER_DIMENSION_LENGTH};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t buffer_row_pitch = DIMENSION_LENGTH;
  const size_t buffer_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;
  const size_t host_row_pitch = DIMENSION_LENGTH;
  const size_t host_slice_pitch = buffer_row_pitch * DIMENSION_LENGTH;

  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(
      command_queue, buffer, CL_FALSE, buffer_origin, host_origin, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      write_data, 0, nullptr, &write_event));

  cl_int error;
  ASSERT_EQ(buffer_data,
            clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                               TOTAL_LENGTH, 1, &write_event, nullptr, &error));
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseEvent(write_event));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool in_region =
            (QUARTER_DIMENSION_LENGTH <= x) &&
            (x < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH) &&
            (QUARTER_DIMENSION_LENGTH <= y) &&
            (y < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH) &&
            (QUARTER_DIMENSION_LENGTH <= z) &&
            (z < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compare_value =
            (in_region) ? buffer_data[index] : INITIAL_BUFFER_DATA;
        ASSERT_EQ(compare_value, buffer_data[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, buffer_data, 0,
                                         nullptr, &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueWriteBufferRectTest)
