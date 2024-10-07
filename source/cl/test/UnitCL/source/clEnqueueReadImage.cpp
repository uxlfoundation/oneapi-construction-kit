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

template <cl_mem_object_type image_type, size_t Width, size_t Height,
          size_t Depth>
struct clEnqueueReadImageTestBase : ucl::CommandQueueTest,
                                    TestWithEventWaitList {
  static constexpr size_t width = Width;
  static constexpr size_t height = Height;
  static constexpr size_t depth = Depth;
  static constexpr size_t num_pixels = width * height * depth;

  clEnqueueReadImageTestBase() : image_data(num_pixels) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;
    image_desc.image_type = image_type;
    image_desc.image_width = width;
    image_desc.image_height = height;
    image_desc.image_depth = depth;
    for (size_t x = 0; x < width; ++x) {
      for (size_t y = 0; y < height; ++y) {
        for (size_t z = 0; z < depth; ++z) {
          const size_t i = x + (width * y) + (width * height * z);
          image_data[i].s[0] = static_cast<cl_uchar>(x);
          image_data[i].s[1] = static_cast<cl_uchar>(y);
          image_data[i].s[2] = static_cast<cl_uchar>(z);
          image_data[i].s[3] = 42;
        }
      }
    }
    cl_int status;
    image = clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                          &image_format, &image_desc, image_data, &status);
    ASSERT_SUCCESS(status);
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, depth};
    UCL::AlignedBuffer<cl_uchar4> result_data(num_pixels);
    ASSERT_EQ_ERRCODE(
        err, clEnqueueReadImage(command_queue, image, CL_TRUE, origin, region,
                                0, 0, result_data, num_events, events, event));
  }

  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  UCL::AlignedBuffer<cl_uchar4> image_data;
  cl_mem image = nullptr;
};

class clEnqueueReadImage2DTest
    : public clEnqueueReadImageTestBase<CL_MEM_OBJECT_IMAGE2D, 4, 4, 1> {};
class clEnqueueReadImage3DTest
    : public clEnqueueReadImageTestBase<CL_MEM_OBJECT_IMAGE3D, 4, 4, 4> {};

TEST_F(clEnqueueReadImage2DTest, DefaultWriteRegionReadWholeImage) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR},
                                   image_desc.image_type, image_format)) {
    GTEST_SKIP();
  }
  size_t write_origin[3] = {2, 1, 0};
  size_t write_region[3] = {2, 1, 1};
  UCL::AlignedBuffer<cl_uint4> region_data(write_region[0] * write_region[1]);
  memset(region_data, 0u, sizeof(region_data));
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_TRUE,
                                     write_origin, write_region, 0, 0,
                                     region_data, 0, nullptr, nullptr));
  cl_uint4 result_data[num_pixels];
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {width, height, 1};
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, read_origin,
                                    read_region, 0, 0, result_data, 0, nullptr,
                                    nullptr));
  // Verify
  for (size_t h = 0; h < height; ++h) {
    for (size_t w = 0; w < width; ++w) {
      printf(
          "{%u, %u, %u, %u}\t",
          // w + width * h,
          result_data[w + (width * h)].s[0], result_data[w + (width * h)].s[1],
          result_data[w + (width * h)].s[2], result_data[w + (width * h)].s[3]);
    }
    printf("\n");
  }
}

TEST_F(clEnqueueReadImage2DTest, DefaultReadRegion) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR},
                                   image_desc.image_type, image_format)) {
    GTEST_SKIP();
  }
  size_t write_origin[3] = {0, 0, 0};
  size_t write_region[3] = {width, height, 1};
  UCL::AlignedBuffer<cl_uint4> region_data(write_region[0] * write_region[1]);
  memset(region_data, 0u, sizeof(region_data));
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_TRUE,
                                     write_origin, write_region, 0, 0,
                                     region_data, 0, nullptr, nullptr));
  size_t read_origin[3] = {2, 2, 0};
  size_t read_region[3] = {2, 2, 1};
  UCL::Buffer<cl_uint4> result_data(write_region[0] * write_region[1]);
  memset(result_data, 255, sizeof(result_data));
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, read_origin,
                                    read_region, 0, 0, result_data, 0, nullptr,
                                    nullptr));
  // Verify
  for (size_t h = 0; h < read_region[0]; ++h) {
    for (size_t w = 0; w < read_region[1]; ++w) {
      printf("{%u, %u, %u, %u}\t",
             // w + read_region[0] * h,
             result_data[w + (read_region[0] * h)].s[0],
             result_data[w + (read_region[0] * h)].s[1],
             result_data[w + (read_region[0] * h)].s[2],
             result_data[w + (read_region[0] * h)].s[3]);
    }
    printf("\n");
  }
}

TEST_F(clEnqueueReadImage3DTest, DefaultReadWholeImage) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR},
                                   image_desc.image_type, image_format)) {
    GTEST_SKIP();
  }
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {width, height, depth};
  UCL::AlignedBuffer<cl_uchar4> result_data(num_pixels);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, result_data, 0, nullptr,
                                    nullptr));
  for (size_t x = 0; x < width; ++x) {
    for (size_t y = 0; y < height; ++y) {
      for (size_t z = 0; z < depth; ++z) {
        const size_t i = x + (width * y) + (width * height * z);
        for (int j = 0; j < 4; ++j) {
          ASSERT_EQ(image_data[i].s[j], result_data[i].s[j])
              << "image_data and result_data differ at: "
              << " x = " << x << " y = " << y << " z = " << z << " i = " << i
              << " j = " << j;
        }
      }
    }
  }
}

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueReadImage2DTest)
GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueReadImage3DTest)
