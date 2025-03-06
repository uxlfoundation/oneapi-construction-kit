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
constexpr size_t DIMENSIONS2D = 2;
constexpr size_t DIMENSIONS = DIMENSIONS2D + 1;
constexpr size_t QUARTER_DIMENSION_LENGTH = 32;
constexpr size_t HALF_DIMENSION_LENGTH =
    QUARTER_DIMENSION_LENGTH + QUARTER_DIMENSION_LENGTH;
constexpr size_t DIMENSION_LENGTH =
    HALF_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH;
constexpr size_t TOTAL_LENGTH =
    DIMENSION_LENGTH * DIMENSION_LENGTH * DIMENSION_LENGTH;

constexpr cl_uchar INITIAL_SCRATCH = 0xFF;
}  // namespace

class clEnqueueReadBufferRectTest : public ucl::CommandQueueTest,
                                    TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
      for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
        for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
          const unsigned int index =
              x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
          payload[index] = static_cast<char>(index);
          scratch[index] = INITIAL_SCRATCH;
        }
      }
    }
    cl_int errcode;
    buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            sizeof(char) * TOTAL_LENGTH, payload, &errcode);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
    const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
    const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                       DIMENSION_LENGTH};

    ASSERT_EQ(err, clEnqueueReadBufferRect(
                       command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin,
                       region, 0, 0, 0, 0, scratch, num_events, events, event));
  }

  cl_uchar payload[TOTAL_LENGTH] = {};
  cl_uchar scratch[TOTAL_LENGTH] = {};
  cl_mem buffer;
};

TEST_F(clEnqueueReadBufferRectTest, ReadFull2D) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH, 1};

  ASSERT_SUCCESS(clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                         bufferOrigin, hostOrigin, region, 0, 0,
                                         0, 0, scratch, 0, nullptr, nullptr));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(payload[index], scratch[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadStart2D) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < HALF_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < HALF_DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(payload[index], scratch[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
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
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadEnd2D) {
  const size_t bufferOrigin[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                           HALF_DIMENSION_LENGTH, 0};
  const size_t hostOrigin[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                         HALF_DIMENSION_LENGTH, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < HALF_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < HALF_DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }

  for (unsigned int x = HALF_DIMENSION_LENGTH; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = HALF_DIMENSION_LENGTH; y < DIMENSION_LENGTH; y++) {
      const unsigned int index = x + (DIMENSION_LENGTH * y);
      ASSERT_EQ(payload[index], scratch[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadMiddle2D) {
  const size_t bufferOrigin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                           QUARTER_DIMENSION_LENGTH, 0};
  const size_t hostOrigin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                         QUARTER_DIMENSION_LENGTH, 0};
  const size_t region[DIMENSIONS] = {HALF_DIMENSION_LENGTH,
                                     HALF_DIMENSION_LENGTH, 1};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < QUARTER_DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < QUARTER_DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
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
      ASSERT_EQ(payload[index], scratch[index])
          << "Coordinates (" << x << ", " << y << ", 0) linearized to ("
          << index << ")";

      for (unsigned int z = 1; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
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
        ASSERT_EQ(INITIAL_SCRATCH, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadFull3D) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_SUCCESS(clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                         bufferOrigin, hostOrigin, region, 0, 0,
                                         0, 0, scratch, 0, nullptr, nullptr));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        ASSERT_EQ(payload[index], scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadStart3D) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool inRegion = (x < HALF_DIMENSION_LENGTH) &&
                              (y < HALF_DIMENSION_LENGTH) &&
                              (z < HALF_DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compareValue =
            (inRegion) ? payload[index] : INITIAL_SCRATCH;
        ASSERT_EQ(compareValue, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadEnd3D) {
  const size_t bufferOrigin[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};
  const size_t hostOrigin[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool inRegion =
            (HALF_DIMENSION_LENGTH <= x) && (x < DIMENSION_LENGTH) &&
            (HALF_DIMENSION_LENGTH <= y) && (y < DIMENSION_LENGTH) &&
            (HALF_DIMENSION_LENGTH <= z) && (z < DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compareValue =
            (inRegion) ? payload[index] : INITIAL_SCRATCH;
        ASSERT_EQ(compareValue, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, ReadMiddle3D) {
  const size_t bufferOrigin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                           QUARTER_DIMENSION_LENGTH,
                                           QUARTER_DIMENSION_LENGTH};
  const size_t hostOrigin[DIMENSIONS] = {QUARTER_DIMENSION_LENGTH,
                                         QUARTER_DIMENSION_LENGTH,
                                         QUARTER_DIMENSION_LENGTH};
  const size_t region[DIMENSIONS] = {
      HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH, HALF_DIMENSION_LENGTH};

  const size_t bufferRowPitch = DIMENSION_LENGTH;
  const size_t bufferSlicePitch = bufferRowPitch * DIMENSION_LENGTH;
  const size_t hostRowPitch = DIMENSION_LENGTH;
  const size_t hostSlicePitch = hostRowPitch * DIMENSION_LENGTH;

  ASSERT_SUCCESS(clEnqueueReadBufferRect(
      command_queue, buffer, CL_TRUE, bufferOrigin, hostOrigin, region,
      bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, scratch,
      0, nullptr, nullptr));

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
      for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
        const bool inRegion =
            (QUARTER_DIMENSION_LENGTH <= x) &&
            (x < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH) &&
            (QUARTER_DIMENSION_LENGTH <= y) &&
            (y < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH) &&
            (QUARTER_DIMENSION_LENGTH <= z) &&
            (z < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const cl_uchar compareValue =
            (inRegion) ? payload[index] : INITIAL_SCRATCH;
        ASSERT_EQ(compareValue, scratch[index])
            << "Coordinates (" << x << ", " << y << ", " << z
            << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_F(clEnqueueReadBufferRectTest, InvalidCommandQueue) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_COMMAND_QUEUE,
            clEnqueueReadBufferRect(nullptr, buffer, CL_TRUE, bufferOrigin,
                                    hostOrigin, region, 0, 0, 0, 0, scratch, 0,
                                    nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, CommandQueueIsInDifferentContext) {
  cl_int errorCode = !CL_SUCCESS;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorCode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errorCode);

  cl_command_queue otherQueue =
      clCreateCommandQueue(otherContext, device, 0, &errorCode);
  EXPECT_TRUE(otherQueue);
  ASSERT_SUCCESS(errorCode);

  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clEnqueueReadBufferRect(
                        otherQueue, buffer, CL_TRUE, bufferOrigin, hostOrigin,
                        region, 0, 0, 0, 0, scratch, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseCommandQueue(otherQueue));
  ASSERT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueReadBufferRectTest, EventIsInDifferentContext) {
  cl_int errorCode = !CL_SUCCESS;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorCode);
  EXPECT_TRUE(otherContext);
  ASSERT_SUCCESS(errorCode);

  cl_event userEvent = clCreateUserEvent(otherContext, &errorCode);
  EXPECT_TRUE(userEvent);
  ASSERT_SUCCESS(errorCode);

  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, 0, 0, scratch, 1,
                              &userEvent, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(userEvent));
  ASSERT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidMemObject) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_MEM_OBJECT,
            clEnqueueReadBufferRect(command_queue, nullptr, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionInXAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH + 1, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionInYAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH + 1,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionInZAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH + 1};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferOriginInXAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {1, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferOriginInYAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 1, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferOriginInZAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 1};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferRowPitchIsOutOfBounds) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidRowPitch = DIMENSION_LENGTH * 2;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, invalidRowPitch, 0, 0, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferSlicePitchIsOutOfBounds) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidSlicePitch = DIMENSION_LENGTH * DIMENSION_LENGTH * 2;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, invalidSlicePitch, 0, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegion) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, nullptr, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferOrigin) {
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, nullptr,
                                    hostOrigin, region, 0, 0, 0, 0, scratch, 0,
                                    nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidHostOrigin) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, nullptr, region, 0, 0, 0, 0,
                                    scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidHostPointer) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionIsZeroInXAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {0, DIMENSION_LENGTH, DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionIsZeroInYAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, 0, DIMENSION_LENGTH};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidRegionIsZeroInZAxis) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH, 0};

  ASSERT_EQ(CL_INVALID_VALUE,
            clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE,
                                    bufferOrigin, hostOrigin, region, 0, 0, 0,
                                    0, scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferRowPitchIsLessThanRegion) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidRowPitch = 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, invalidRowPitch, 0, 0, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidBufferSlicePitchIsLessThanRegion) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidSlicePitch = 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, invalidSlicePitch, 0, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest,
       InvalidBufferSlicePitchNotAMultipleOfBufferRowPitch) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidSlicePitch = (DIMENSION_LENGTH * DIMENSION_LENGTH) + 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, invalidSlicePitch, 0, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidHostRowPitchIsLessThanRegion) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidRowPitch = 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, invalidRowPitch, 0,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidHostSlicePitchIsLessThanRegion) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidSlicePitch = 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, 0, invalidSlicePitch,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest,
       InvalidHostSlicePitchNotAMultipleOfHostRowPitch) {
  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  const size_t invalidSlicePitch = (DIMENSION_LENGTH * DIMENSION_LENGTH) + 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueReadBufferRect(command_queue, buffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, 0, invalidSlicePitch,
                              scratch, 0, nullptr, nullptr));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidMemObjectIsHostWriteOnly) {
  cl_int errorCode = !CL_SUCCESS;

  cl_mem otherBuffer =
      clCreateBuffer(context, CL_MEM_HOST_WRITE_ONLY,
                     sizeof(char) * TOTAL_LENGTH, nullptr, &errorCode);
  EXPECT_TRUE(otherBuffer);
  ASSERT_SUCCESS(errorCode);

  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueReadBufferRect(command_queue, otherBuffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, 0, 0, scratch, 0,
                              nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherBuffer));
}

TEST_F(clEnqueueReadBufferRectTest, InvalidMemObjectIsHostNoAccess) {
  cl_int errorCode = !CL_SUCCESS;

  cl_mem otherBuffer =
      clCreateBuffer(context, CL_MEM_HOST_NO_ACCESS,
                     sizeof(char) * TOTAL_LENGTH, nullptr, &errorCode);
  EXPECT_TRUE(otherBuffer);
  ASSERT_SUCCESS(errorCode);

  const size_t bufferOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t hostOrigin[DIMENSIONS] = {0, 0, 0};
  const size_t region[DIMENSIONS] = {DIMENSION_LENGTH, DIMENSION_LENGTH,
                                     DIMENSION_LENGTH};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueReadBufferRect(command_queue, otherBuffer, CL_TRUE, bufferOrigin,
                              hostOrigin, region, 0, 0, 0, 0, scratch, 0,
                              nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherBuffer));
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueReadBufferRectTest)

// Redmine #5120: Check cannot test CL_MISALIGNED_SUB_BUFFER_OFFSET without
// multiple devices (as clCreateSubBuffer will catch it before here!)
