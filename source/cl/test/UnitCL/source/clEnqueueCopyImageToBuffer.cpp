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

#include <cstdint>

#include "Common.h"
#include "EventWaitList.h"

template <cl_mem_object_type type, int Width, int Height, int Depth,
          int ArraySize>
class clEnqueueCopyImageToBufferTest : public ucl::CommandQueueTest,
                                       TestWithEventWaitList {
 public:
  enum {
    width = Width,
    height = Height,
    depth = Depth,
    arraySize = ArraySize,
    num_elements = Width * Height * Depth * arraySize
  };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;
    image_desc.image_type = type;
    image_desc.image_width = width;
    image_desc.image_height = height;
    image_desc.image_depth = depth;
    image_desc.image_array_size = arraySize;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
      cl_int status;
      buffer_in = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                 image_desc.image_width * num_elements, nullptr,
                                 &status);
      image_desc.buffer = buffer_in;
      ASSERT_SUCCESS(status);
    } else {
      image_desc.buffer = nullptr;
    }

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int status;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &status);
    ASSERT_SUCCESS(status);
    size_t pixel_size = 0;
    ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                  &pixel_size, nullptr));
    buffer_size = pixel_size * num_elements;
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr,
                            &status);
    ASSERT_SUCCESS(status);

    size_t region_y = height;
    size_t region_z = depth;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY) {
      region_y = arraySize;
      region_z = 1;
    } else if (image_desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
      region_z = arraySize;
    }

    const cl_uint color[4] = {0, 255, 127, 255};
    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {width, region_y, region_z};
    cl_event fill_event;
    ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, color, origin,
                                      region, 0, nullptr, &fill_event));
    ASSERT_SUCCESS(clWaitForEvents(1, &fill_event));
    ASSERT_SUCCESS(clReleaseEvent(fill_event));
  }

  void TearDown() override {
    if (event) {
      clReleaseEvent(event);
    }
    if (buffer_in) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer_in));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, depth};
    const size_t offset = 0;
    ASSERT_EQ_ERRCODE(err, clEnqueueCopyImageToBuffer(
                               command_queue, image, buffer, origin, region,
                               offset, num_events, events, event));
  }

  void test_body() {
    size_t region_y = height;
    size_t region_z = depth;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY) {
      region_y = arraySize;
      region_z = 1;
    } else if (image_desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
      region_z = arraySize;
    }

    const size_t origin[3] = {0, 0, 0};
    const size_t region[3] = {width, region_y, region_z};
    const size_t offset = 0;
    ASSERT_SUCCESS(clEnqueueCopyImageToBuffer(command_queue, image, buffer,
                                              origin, region, offset, 0,
                                              nullptr, &event));
    UCL::vector<uint8_t> out(buffer_size);
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, offset,
                                       buffer_size, out.data(), 1, &event,
                                       nullptr));
    for (int i = 0; i < num_elements * 4; i += 4) {
      EXPECT_EQ(0, out[i + 0]);
      EXPECT_EQ(255, out[i + 1]);
      EXPECT_EQ(127, out[i + 2]);
      EXPECT_EQ(255, out[i + 3]);
    }
  }

  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  cl_mem image = nullptr;
  size_t buffer_size = 0;
  cl_mem buffer = nullptr;
  cl_mem buffer_in = nullptr;
  cl_event event = nullptr;
};

// This one is just used for testing the invalid tests so we don't have to run
// the invalid tests for all the different image object types.
typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE1D, 1, 1, 1, 1>
    clEnqueueCopyImageToBufferInvalidTest;
typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE1D, 4, 1, 1, 1>
    clEnqueueCopyImageToBuffer1DTest;
typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE1D_BUFFER, 6, 1, 1, 1>
    clEnqueueCopyImageToBuffer1DBufferTest;
typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE1D_ARRAY, 4, 1, 1, 4>
    clEnqueueCopyImageToBuffer1DArrayTest;

typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE2D, 4, 4, 1, 1>
    clEnqueueCopyImageToBuffer2DTest;
typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE2D_ARRAY, 4, 4, 1, 4>
    clEnqueueCopyImageToBuffer2DArrayTest;

typedef clEnqueueCopyImageToBufferTest<CL_MEM_OBJECT_IMAGE3D, 4, 4, 4, 1>
    clEnqueueCopyImageToBuffer3DTest;

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidCommandQueue) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueCopyImageToBuffer(nullptr, image, buffer, origin, region, offset,
                                 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidContextCommandQueue) {
  cl_int status;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  cl_command_queue other_queue =
      clCreateCommandQueue(other_context, device, 0, &status);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyImageToBuffer(other_queue, image, buffer, origin, region,
                                 offset, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseCommandQueue(other_queue));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidContextImage) {
  cl_int status;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_SUCCESS(status);
  cl_mem other_image =
      clCreateImage(other_context, CL_MEM_READ_WRITE, &image_format,
                    &image_desc, nullptr, &status);
  ASSERT_SUCCESS(status);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyImageToBuffer(command_queue, other_image, buffer, origin,
                                 region, offset, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(other_image));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidContextBuffer) {
  cl_int status;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_SUCCESS(status);
  size_t pixel_size = 0;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                &pixel_size, nullptr));
  buffer_size = pixel_size * num_elements;
  cl_mem other_buffer = clCreateBuffer(other_context, CL_MEM_READ_WRITE,
                                       buffer_size, nullptr, &status);
  ASSERT_SUCCESS(status);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyImageToBuffer(command_queue, image, other_buffer, origin,
                                 region, offset, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidValueSrcOrigin) {
  size_t origin[3] = {width + 1, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, nullptr,
                                          region, offset, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest,
       InvalidValueSrcOriginPlusSrcRegion) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width + 1, height, depth};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidValueDstOffset) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = num_elements + 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidValueDstOffsetPlusDstCb) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidValueNullRegion) {
  size_t origin[3] = {0, 0, 0};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImageToBuffer(command_queue, image, buffer, origin, nullptr,
                                 offset, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageToBufferInvalidTest, InvalidValueOriginRegionRules) {
  size_t origin[3] = {0, 1, 0};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));
}

// This is the test for the 2D version of InvalidValueOriginRegionRules
TEST_F(clEnqueueCopyImageToBuffer2DTest, InvalidValueOriginRegionRules) {
  size_t origin[3] = {0, 0, 1};
  size_t region[3] = {width, height, depth};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyImageToBuffer(
                                          command_queue, image, buffer, origin,
                                          region, offset, 0, nullptr, nullptr));
}

// CA-1823: Test disabled because we currently cannot check for a
// `CL_MISALIGNED_SUB_BUFFER_OFFSET` while doing `clEnqueueCopyImageToBuffer`
// because we would detect the invalid offset while create the sub buffer in
// `clCreateSubBuffer`; and we cannot create the sub buffer with a correct
// offset and then modify the latter into an invalid offset.
// One way to trigger `CL_MISALIGNED_SUB_BUFFER_OFFSET` in
// `clEnqueueCopyImageToBuffer` would be to have two devices, one for which
// the offset has a valid alignment (which then would not trigger the
// `CL_MISALIGNED_SUB_BUFFER_OFFSET` in `clCreateSubBuffer`), and the other
// that would have an invalid one, and then call `clEnqueueCopyImageToBuffer`
// on the one that has a bad offset.
TEST_F(clEnqueueCopyImageToBuffer1DBufferTest,
       DISABLED_InvalidSubBufferOffset) {
  cl_buffer_region buff_region;
  buff_region.origin = 3;  // Set invalid offset for sub buffer
  buff_region.size = sizeof(width) * width / 2;

  cl_int status;
  auto sub_buff =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &buff_region, &status);
  EXPECT_SUCCESS(status);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width / 2, height, depth};
  const size_t offset = 0;
  EXPECT_EQ_ERRCODE(
      CL_MISALIGNED_SUB_BUFFER_OFFSET,
      clEnqueueCopyImageToBuffer(command_queue, image, sub_buff, origin, region,
                                 offset, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseMemObject(sub_buff));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueCopyImageToBufferInvalidTest)

TEST_F(clEnqueueCopyImageToBuffer1DTest, Default) { test_body(); }
TEST_F(clEnqueueCopyImageToBuffer1DBufferTest, Default) { test_body(); }
TEST_F(clEnqueueCopyImageToBuffer1DArrayTest, Default) { test_body(); }

TEST_F(clEnqueueCopyImageToBuffer2DTest, Default) { test_body(); }
TEST_F(clEnqueueCopyImageToBuffer2DArrayTest, Default) { test_body(); }

TEST_F(clEnqueueCopyImageToBuffer3DTest, Default) { test_body(); }

// CL_MEM_OBJECT_ALLOCATION_FAILURE, CL_OUT_OF_RESOURCES ,
// CL_OUT_OF_HOST_MEMORY, CL_INVALID_OPERATION, CL_INVALID_IMAGE_SIZE and
// CL_INVALID_IMAGE_FORMAT are not being tested as they require seperate devices
// to allow creation of an invalid image with which to test.As such, we cannot
// test clEnqueueCopyImage correctly to get the correct error codes
// Redmine #5123, #5125, #5117, #5114
