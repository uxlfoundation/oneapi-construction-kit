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

#include <cstring>
#include <vector>

#include "Common.h"
#include "EventWaitList.h"

const size_t good_alignment = 4096;
class clEnqueueMapBufferTest : public ucl::CommandQueueTest,
                               TestWithEventWaitList {
 protected:
  enum { FACTOR = 2 };

  clEnqueueMapBufferTest(bool use_host_ptr = false)
      : useHostPtr(use_host_ptr) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    size = getDeviceMemBaseAddrAlign();
    // we need an allocation FACTOR times as large as the alignment
    size *= FACTOR;

    int_size = sizeof(int) * size;

    inBuffer.resize(size);
    outBuffer.resize(size);

    cl_int errorcode;
    if (useHostPtr) {
      // Create a host buffer to be used to hold the contents. Deliberately give
      // poor alignment so we need to create a copy
      hostBuffer.resize(size + 1);
      void *useptr = static_cast<void *>(hostBuffer.data() + 1);
      inMem = clCreateBuffer(context, CL_MEM_USE_HOST_PTR, int_size, useptr,
                             &errorcode);
    } else {
      inMem = clCreateBuffer(context, 0, int_size, nullptr, &errorcode);
    }
    EXPECT_TRUE(inMem);
    ASSERT_SUCCESS(errorcode);
    outMem = clCreateBuffer(context, 0, int_size, nullptr, &errorcode);
    EXPECT_TRUE(outMem);
    ASSERT_SUCCESS(errorcode);

    for (cl_uint i = 0; i < size; i++) {
      inBuffer[i] = i;
      outBuffer[i] = 0xFFFFFFFF;
    }

    writeEvent = nullptr;
    mapEvent = nullptr;
    unMapEvent = nullptr;
    readEvent = nullptr;

    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, CL_TRUE, 0,
                                        int_size, inBuffer.data(), 0, nullptr,
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
    if (outMem) {
      EXPECT_SUCCESS(clReleaseMemObject(outMem));
    }
    if (inMem) {
      EXPECT_SUCCESS(clReleaseMemObject(inMem));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    cl_int errcode = !CL_SUCCESS;
    void *const map = clEnqueueMapBuffer(
        command_queue, inMem, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
        int_size, num_events, events, event, &errcode);
    EXPECT_EQ_ERRCODE(err, errcode);
    EXPECT_FALSE(map);
  }

  cl_uint size;
  cl_uint int_size;
  std::vector<int> inBuffer;
  std::vector<int> outBuffer;
  cl_mem inMem;
  cl_mem outMem;
  cl_event writeEvent;
  cl_event mapEvent;
  cl_event unMapEvent;
  cl_event readEvent;

  std::vector<int, UCL::aligned_allocator<int, good_alignment>> hostBuffer;
  bool useHostPtr;
};

class clEnqueueMapBufferTestHostPtr : public clEnqueueMapBufferTest {
 public:
  clEnqueueMapBufferTestHostPtr() : clEnqueueMapBufferTest(true) {}
};

TEST_F(clEnqueueMapBufferTest, DefaultRead) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(
      clEnqueueMapBuffer(command_queue, inMem, CL_FALSE, CL_MAP_READ, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode));
  ASSERT_SUCCESS(errcode);
  EXPECT_TRUE(mapEvent);
  ASSERT_TRUE(map);

  EXPECT_SUCCESS(clWaitForEvents(1, &mapEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueMapBufferTest, DefaultReadBlocking) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(
      clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0,
                         int_size, 1, &writeEvent, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &unMapEvent));
}

TEST_F(clEnqueueMapBufferTest, DefaultWrite) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(
      clEnqueueMapBuffer(command_queue, inMem, CL_FALSE, CL_MAP_WRITE, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode));
  ASSERT_SUCCESS(errcode);
  EXPECT_TRUE(mapEvent);
  ASSERT_TRUE(map);

  EXPECT_SUCCESS(clWaitForEvents(1, &mapEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
    map[i] = -map[i];
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(-inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTest, DefaultWriteBlocking) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(
      clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0,
                         int_size, 1, &writeEvent, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
    map[i] = -map[i];
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(-inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTest, DefaultWriteInvalidate) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, &mapEvent, &errcode));
  EXPECT_TRUE(map);
  EXPECT_TRUE(mapEvent);
  ASSERT_SUCCESS(errcode);

  EXPECT_SUCCESS(clWaitForEvents(1, &mapEvent));

  for (int i = 0; i < static_cast<cl_int>(size); i++) {
    map[i] = -i;
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_int i = 0; i < static_cast<cl_int>(size); i++) {
    EXPECT_EQ(-i, outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTestHostPtr, DefaultWriteInvalidateHostPtr) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, &mapEvent, &errcode));
  EXPECT_TRUE(map);
  EXPECT_TRUE(mapEvent);
  ASSERT_SUCCESS(errcode);

  EXPECT_SUCCESS(clWaitForEvents(1, &mapEvent));

  for (int i = 0; i < static_cast<cl_int>(size); i++) {
    map[i] = -i;
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_int i = 0; i < static_cast<cl_int>(size); i++) {
    EXPECT_EQ(-i, outBuffer[i]);
  }
}
TEST_F(clEnqueueMapBufferTest, DefaultWriteInvalidateBlocking) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = static_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, nullptr, &errcode));
  EXPECT_TRUE(map);
  ASSERT_SUCCESS(errcode);

  for (cl_int i = 0; i < static_cast<cl_int>(size); i++) {
    map[i] = -i;
  }

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_TRUE, 0, int_size,
                                     outBuffer.data(), 1, &unMapEvent,
                                     nullptr));

  for (cl_int i = 0; i < static_cast<cl_int>(size); i++) {
    EXPECT_EQ(-i, outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTest, DefaultReadWrite) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = static_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_FALSE, CL_MAP_READ | CL_MAP_WRITE, 0, int_size,
      1, &writeEvent, &mapEvent, &errcode));
  ASSERT_SUCCESS(errcode);
  EXPECT_TRUE(mapEvent);
  ASSERT_TRUE(map);

  EXPECT_SUCCESS(clWaitForEvents(1, &mapEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
    map[i] = -map[i];
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(-inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTest, DefaultReadWriteBlocking) {
  cl_int errcode = !CL_SUCCESS;
  int *const map = reinterpret_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, int_size, 1,
      &writeEvent, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(inBuffer[i], map[i]);
    map[i] = -map[i];
  }

  EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr,
                                         &unMapEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, inMem, CL_FALSE, 0,
                                     int_size, outBuffer.data(), 1, &unMapEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  for (cl_uint i = 0; i < size; i++) {
    EXPECT_EQ(-inBuffer[i], outBuffer[i]);
  }
}

TEST_F(clEnqueueMapBufferTest, WithOffset) {
  cl_int errcode = !CL_SUCCESS;

  const size_t offset = 1;

  int *const map = static_cast<int *>(clEnqueueMapBuffer(
      command_queue, inMem, CL_TRUE, CL_MAP_READ, offset * sizeof(int),
      sizeof(int), 0, nullptr, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  ASSERT_EQ(inBuffer[offset], map[0]);

  ASSERT_SUCCESS(clFinish(command_queue));
}

TEST_F(clEnqueueMapBufferTest, MapSubBuffer) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  info.origin = int_size / FACTOR;
  info.size = sizeof(int);

  cl_mem subMem = clCreateSubBuffer(
      inMem, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  int *const map = static_cast<int *>(
      clEnqueueMapBuffer(command_queue, subMem, CL_TRUE, CL_MAP_READ, 0,
                         sizeof(int), 0, nullptr, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  ASSERT_EQ(inBuffer[(size / FACTOR)], map[0]);

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, subMem, map, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

TEST_F(clEnqueueMapBufferTest, MapSubBufferWithOffset) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  const size_t offset = 1;

  info.origin = int_size / FACTOR;
  info.size = sizeof(int) * (offset + 1);

  cl_mem subMem = clCreateSubBuffer(
      inMem, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  int *const map = static_cast<int *>(clEnqueueMapBuffer(
      command_queue, subMem, CL_TRUE, CL_MAP_READ, offset * sizeof(int),
      sizeof(int), 0, nullptr, nullptr, &errcode));
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(map);

  ASSERT_EQ(inBuffer[(size / FACTOR) + offset], map[0]);

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, subMem, map, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

TEST_F(clEnqueueMapBufferTest, OverlappingReadMappings) {
  // Create two maps with the same offset but different sizes.
  const auto map_b_size = size / 2;

  cl_int error = !CL_SUCCESS;
  clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_READ, 0, size, 0,
                     nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0, map_b_size,
                     0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
}

TEST_F(clEnqueueMapBufferTest, NonOverlappingWriteMappings) {
  const auto map_size = size / 2;
  cl_int error = !CL_SUCCESS;
  // Create two maps with non-overlapping ranges i.e. buffer = [map_a | map_b].
  auto map_a = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE,
                                  0, map_size, 0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  auto map_b =
      clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, map_size,
                         map_size, 0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map_a, 0,
                                         nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map_b, 0,
                                         nullptr, nullptr));
  // Flush the queue to ensure maps are no longer active.
  ASSERT_SUCCESS(clFinish(command_queue));

  // Then reverse the mapping order i.e. buffer = [map_b|map_a].
  map_a = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE,
                             map_size, map_size, 0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  map_b = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0,
                             map_size, 0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map_a, 0,
                                         nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, inMem, map_b, 0,
                                         nullptr, nullptr));
}

TEST_F(clEnqueueMapBufferTest, InvalidOverlappingWriteMappings) {
  // Create two maps with the same offset but different sizes.
  const auto map_b_size = size / 2;

  cl_int error = !CL_SUCCESS;
  clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0, size, 0,
                     nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0, map_b_size,
                     0, nullptr, nullptr, &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, error);
}

TEST_F(clEnqueueMapBufferTest, ValidOverlappingWriteMappings) {
  // Create two identical maps, seperated by an unmap.
  cl_int error = !CL_SUCCESS;
  auto *map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0,
                                 size, 0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);

  EXPECT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr, nullptr));

  map = clEnqueueMapBuffer(command_queue, inMem, CL_TRUE, CL_MAP_WRITE, 0, size,
                           0, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  EXPECT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, inMem, map, 0, nullptr, nullptr));
}

TEST_F(clEnqueueMapBufferTest, InvalidCommandQueue) {
  cl_int errcode = !CL_SUCCESS;
  void *const map = clEnqueueMapBuffer(
      nullptr, inMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0, int_size, 1,
      &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, errcode);
  EXPECT_FALSE(map);
}

TEST_F(clEnqueueMapBufferTest, InvalidBuffer) {
  cl_int errcode = !CL_SUCCESS;
  void *const map = clEnqueueMapBuffer(
      command_queue, nullptr, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, errcode);
  EXPECT_FALSE(map);
}

TEST_F(clEnqueueMapBufferTest, InvalidValueOutOfBounds) {
  cl_int errcode = !CL_SUCCESS;
  void *const map = clEnqueueMapBuffer(
      command_queue, inMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, int_size,
      int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  EXPECT_FALSE(map);
}

TEST_F(clEnqueueMapBufferTest, InvalidValueSizeZero) {
  cl_int errcode = !CL_SUCCESS;
  void *const map = clEnqueueMapBuffer(command_queue, inMem, CL_FALSE,
                                       CL_MAP_WRITE_INVALIDATE_REGION, 0, 0, 1,
                                       &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  EXPECT_FALSE(map);
}

TEST_F(clEnqueueMapBufferTest, InvalidValueFlags) {
  cl_int errcode = !CL_SUCCESS;
  const auto all_valid_map_flags = static_cast<cl_map_flags>(
      CL_MAP_READ | CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION);
  void *const map =
      clEnqueueMapBuffer(command_queue, inMem, CL_FALSE, (~all_valid_map_flags),
                         0, int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  EXPECT_FALSE(map);
}

TEST_F(clEnqueueMapBufferTest, InvalidOperationBufferWriteOnlyWithReadMap) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  info.origin = 0;
  info.size = int_size;

  cl_mem subMem =
      clCreateSubBuffer(inMem, CL_MEM_HOST_WRITE_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  errcode = !CL_SUCCESS;

  void *const map =
      clEnqueueMapBuffer(command_queue, subMem, CL_FALSE, CL_MAP_READ, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map);

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

TEST_F(clEnqueueMapBufferTest, InvalidOperationBufferNoAccessWithReadMap) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  info.origin = 0;
  info.size = int_size;

  cl_mem subMem =
      clCreateSubBuffer(inMem, CL_MEM_HOST_NO_ACCESS,
                        CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  errcode = !CL_SUCCESS;

  void *const map =
      clEnqueueMapBuffer(command_queue, subMem, CL_FALSE, CL_MAP_READ, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map);

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

TEST_F(clEnqueueMapBufferTest, InvalidOperationBufferReadOnlyWithWriteMap) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  info.origin = 0;
  info.size = int_size;

  cl_mem subMem =
      clCreateSubBuffer(inMem, CL_MEM_HOST_READ_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  errcode = !CL_SUCCESS;

  void *const map1 =
      clEnqueueMapBuffer(command_queue, subMem, CL_FALSE, CL_MAP_WRITE, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map1);

  void *const map2 = clEnqueueMapBuffer(
      command_queue, subMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map2);

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

TEST_F(clEnqueueMapBufferTest, InvalidOperationBufferNoAccessWithWriteMap) {
  cl_int errcode = !CL_SUCCESS;

  struct {
    size_t origin;
    size_t size;
  } info;

  info.origin = 0;
  info.size = int_size;

  cl_mem subMem =
      clCreateSubBuffer(inMem, CL_MEM_HOST_NO_ACCESS,
                        CL_BUFFER_CREATE_TYPE_REGION, &info, &errcode);
  ASSERT_SUCCESS(errcode);

  errcode = !CL_SUCCESS;

  void *const map1 =
      clEnqueueMapBuffer(command_queue, subMem, CL_FALSE, CL_MAP_WRITE, 0,
                         int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map1);

  void *const map2 = clEnqueueMapBuffer(
      command_queue, subMem, CL_FALSE, CL_MAP_WRITE_INVALIDATE_REGION, 0,
      int_size, 1, &writeEvent, &mapEvent, &errcode);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
  EXPECT_FALSE(map2);

  ASSERT_SUCCESS(clReleaseMemObject(subMem));
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueMapBufferTest)

// Redmine #5142: CL_INVALID_CONTEXT if the context associated with
// command_queue and image are not the same.
// Redmine #5142: CL_MAP_FAILURE if there is a failure to map the requested
// region into the host address space. This error cannot occur for image
// objects created with CL_MEM_USE_HOST_PTR or CL_MEM_ALLOC_HOST_PTR.
// Redmine #5123: CL_MEM_OBJECT_ALLOCATION_FAILURE if there is a failure to
// allocate memory for data store associated with buffer.

struct clEnqueueMapBufferSubBuffer : ucl::CommandQueueTest {
  static constexpr size_t numRegions = 3;

  cl_uint regionSize = 0;
  size_t numElementsPerRegion = 0;
  size_t numElements = 0;
  size_t bufferSize = 0;
  std::vector<uint32_t> input;

  cl_mem buffer = nullptr;
  cl_mem subBuffer = nullptr;
  cl_mem resultBuffer = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    regionSize = getDeviceMemBaseAddrAlign();
    numElementsPerRegion = regionSize / sizeof(uint32_t);
    bufferSize = regionSize * numRegions;
    numElements = numElementsPerRegion * numRegions;
    input.resize(numElements);
    cl_int error;
    buffer = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, bufferSize,
                            input.data(), &error);
    ASSERT_SUCCESS(error);
    resultBuffer =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, bufferSize, nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (subBuffer) {
      EXPECT_SUCCESS(clReleaseMemObject(subBuffer));
    }
    if (resultBuffer) {
      EXPECT_SUCCESS(clReleaseMemObject(resultBuffer));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }
};

TEST_F(clEnqueueMapBufferSubBuffer, Default) {
  // Create subBuffer in the middle of buffer.
  cl_buffer_region bufferRegion = {regionSize, regionSize};
  cl_int error;
  subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &bufferRegion, &error);
  ASSERT_SUCCESS(error);

  // Kernel to generate test data, written into subBuffer.
  const char *source = R"OpenCLC(
kernel void generate(global uint* ptr) {
  size_t id = get_global_id(0);
  ptr[id] = 42 + id;
}
)OpenCLC";
  const size_t length = std::strlen(source);
  program = clCreateProgramWithSource(context, 1, &source, &length, &error);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  kernel = clCreateKernel(program, "generate", &error);
  ASSERT_SUCCESS(error);

  // Write the test data into subBuffer.
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&subBuffer)));
  cl_event ndRangeEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &numElementsPerRegion, nullptr, 0,
                                        nullptr, &ndRangeEvent));

  // Map subBuffer to test flushing of device to host.
  auto map = static_cast<cl_uint *>(
      clEnqueueMapBuffer(command_queue, subBuffer, CL_TRUE, CL_MAP_READ, 0,
                         regionSize, 1, &ndRangeEvent, nullptr, &error));
  ASSERT_SUCCESS(error);

  // Check the test data generated by the kernel is present, if the results are
  // all zeros no flush occured, if the results are at the wrong index the
  // flushing offset is wrong.
  for (size_t index = 0; index < numElementsPerRegion; index++) {
    ASSERT_EQ(42 + index, map[index]);
  }

  // This unmap is a noop since CL_MAP_READ was used above.
  cl_event unmapEvent;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, subBuffer, map, 0,
                                         nullptr, &unmapEvent));

  std::vector<cl_uint> output(numElements);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     bufferSize, output.data(), 1, &unmapEvent,
                                     nullptr));

  ASSERT_SUCCESS(clReleaseEvent(unmapEvent));
  ASSERT_SUCCESS(clReleaseEvent(ndRangeEvent));

  // Check that buffer contains the test data written into subBuffer.
  for (size_t index = 0; index < numElementsPerRegion; index++) {
    ASSERT_EQ(0, output[index]);
  }
  for (size_t index = 0; index < numElementsPerRegion; index++) {
    ASSERT_EQ(42 + index, output[index + numElementsPerRegion]);
  }
  for (size_t index = 0; index < numElementsPerRegion; index++) {
    ASSERT_EQ(0, output[index + (numElementsPerRegion * 2)]);
  }
}

// Tests enqueueing a buffer map command on buffers created with various
// flag combinations.
struct clEnqueueMapBufferFlagsTest
    : public ucl::CommandQueueTest,
      ::testing::WithParamInterface<cl_mem_flags> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int error;
    creation_flags = GetParam();
    buffer = clCreateBuffer(context, creation_flags, bytes, nullptr, &error);
    ASSERT_TRUE(buffer != nullptr);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (device) {
      EXPECT_SUCCESS(clReleaseDevice(device));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  static const size_t bytes = 512;

  cl_device_id device = nullptr;
  cl_mem buffer = nullptr;
  cl_mem_flags creation_flags = 0;
};

TEST_P(clEnqueueMapBufferFlagsTest, MapRead) {
  cl_int error = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, buffer, CL_FALSE, CL_MAP_READ,
                                 0, bytes, 0, nullptr, nullptr, &error);

  // CL_INVALID_OPERATION if buffer has been created with CL_MEM_HOST_WRITE_ONLY
  // or CL_MEM_HOST_NO_ACCESS when it is mapped with CL_MAP_READ set
  if ((CL_MEM_HOST_WRITE_ONLY & creation_flags) ||
      (CL_MEM_HOST_NO_ACCESS & creation_flags)) {
    ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, error);
  } else {
    ASSERT_SUCCESS(error);
    ASSERT_TRUE(nullptr != map);

    EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, map, 0,
                                           nullptr, nullptr));
  }
}

TEST_P(clEnqueueMapBufferFlagsTest, MapWrite) {
  cl_int error = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, buffer, CL_FALSE, CL_MAP_WRITE,
                                 0, bytes, 0, nullptr, nullptr, &error);

  // CL_INVALID_OPERATION if buffer has been created with CL_MEM_HOST_READ_ONLY
  // or CL_MEM_HOST_NO_ACCESS when it is mapped with CL_MAP_WRITE set
  if ((CL_MEM_HOST_READ_ONLY & creation_flags) ||
      (CL_MEM_HOST_NO_ACCESS & creation_flags)) {
    ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, error);
  } else {
    ASSERT_SUCCESS(error);
    ASSERT_TRUE(nullptr != map);

    EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, map, 0,
                                           nullptr, nullptr));
  }
}

TEST_P(clEnqueueMapBufferFlagsTest, MapInvalidate) {
  cl_int error = !CL_SUCCESS;
  void *map = clEnqueueMapBuffer(command_queue, buffer, CL_FALSE,
                                 CL_MAP_WRITE_INVALIDATE_REGION, 0, bytes, 0,
                                 nullptr, nullptr, &error);

  // CL_INVALID_OPERATION if buffer has been created with CL_MEM_HOST_READ_ONLY
  // or CL_MEM_HOST_NO_ACCESS when it is mapped with
  // CL_MAP_WRITE_INVALIDATE_REGION set in map_flags
  if ((CL_MEM_HOST_READ_ONLY & creation_flags) ||
      (CL_MEM_HOST_NO_ACCESS & creation_flags)) {
    ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, error);
  } else {
    ASSERT_SUCCESS(error);
    ASSERT_TRUE(nullptr != map);

    EXPECT_SUCCESS(clEnqueueUnmapMemObject(command_queue, buffer, map, 0,
                                           nullptr, nullptr));
  }
}

INSTANTIATE_TEST_SUITE_P(
    clEnqueueMapBufferTest, clEnqueueMapBufferFlagsTest,
    ::testing::Values(CL_MEM_READ_WRITE, CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY,
                      CL_MEM_HOST_WRITE_ONLY, CL_MEM_HOST_READ_ONLY,
                      CL_MEM_HOST_NO_ACCESS,
                      CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
                      CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                      CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS));
