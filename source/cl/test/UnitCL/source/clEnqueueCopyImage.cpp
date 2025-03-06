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
#include <random>

#include "Common.h"
#include "EventWaitList.h"

struct clEnqueueCopyImageParamTest
    : ucl::CommandQueueTest,
      testing::WithParamInterface<cl_image_format> {
 protected:
  clEnqueueCopyImageParamTest() : format(GetParam()) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    if (dst_image) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_image));
    }
    if (src_image) {
      EXPECT_SUCCESS(clReleaseMemObject(src_image));
    }
    CommandQueueTest::TearDown();
  }

  const cl_image_format &format;
  cl_mem src_image = nullptr;
  cl_mem dst_image = nullptr;
  cl_event event = nullptr;
};

static void generate_data(UCL::vector<uint8_t> &data) {
  std::mt19937 range;
  range.seed();
  // Using a larger than necessary distribution integer type because
  // std::uniform_int_distribution behavior is undefined for uint8_t. Moreover,
  // we cannot use UINT8_MAX, because this limit does not seem to be defined on
  // Android even when __STDC_LIMIT_MACROS is defined.
  std::uniform_int_distribution<unsigned int> uint8_dist(
      0, std::numeric_limits<int8_t>::max());
  for (auto &elem : data) {
    elem = static_cast<uint8_t>(uint8_dist(range));
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default1D) {
  if (!UCL::isImageFormatSupported(
          context,
          {CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, CL_MEM_READ_WRITE},
          CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc.image_width = 16;
  UCL::vector<uint8_t> in(desc.image_width * UCL::getPixelSize(format));
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            &format, &desc, in.data(), &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, src_image);
  dst_image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                            &error);
  ASSERT_SUCCESS(error);
  EXPECT_NE(nullptr, dst_image);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, 1, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image, origin,
                                    origin, region, 0, nullptr, &event));
  UCL::vector<uint8_t> out(in.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE, origin,
                                    region, 0, 0, out.data(), 1, &event,
                                    nullptr));
  for (size_t i = 0; i < in.size(); ++i) {
    ASSERT_EQ(in[i], out[i]);
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default1D2D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE1D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc desc1d = {};
  desc1d.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc1d.image_width = 16;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(desc1d.image_width * pixel_size);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc1d, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc desc2d = {};
  desc2d.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc2d.image_width = 32;
  desc2d.image_height = 32;
  UCL::vector<uint8_t> zeros(desc2d.image_width * desc2d.image_height *
                             pixel_size);
  dst_image = clCreateImage(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc2d, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {16, 0, 0};
  size_t region[3] = {desc1d.image_width, 1, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc2d.image_width, desc2d.image_height, 1};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));
  const size_t region_begin = (pixel_size * dst_origin[0]) +
                              (pixel_size * desc2d.image_width * dst_origin[1]);
  const size_t region_end = region_begin + (pixel_size * region[0] * region[1]);
  for (size_t i = 0; i < out.size(); ++i) {
    if (i >= region_begin && i < region_end) {
      ASSERT_EQ(in[i - region_begin], out[i]);
    } else {
      ASSERT_EQ(0, out[i]);
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default1D3D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE1D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc desc1d = {};
  desc1d.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc1d.image_width = 27;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * desc1d.image_width);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc1d, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc desc3d = {};
  desc3d.image_type = CL_MEM_OBJECT_IMAGE3D;
  desc3d.image_width = desc1d.image_width;
  desc3d.image_height = 3;
  desc3d.image_depth = 3;
  UCL::vector<uint8_t> zeros(pixel_size * desc3d.image_width *
                             desc3d.image_height * desc3d.image_depth);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc3d, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {0, desc3d.image_height / 2, desc3d.image_depth / 2};
  size_t region[3] = {desc1d.image_width, 1, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc3d.image_width, desc3d.image_height,
                           desc3d.image_depth};
  UCL::vector<uint8_t> out(pixel_size * desc3d.image_width *
                           desc3d.image_height * desc3d.image_depth);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));
  const size_t region_begin =
      (pixel_size * dst_origin[0]) +
      (pixel_size * desc3d.image_width * dst_origin[1]) +
      (pixel_size * desc3d.image_width * desc3d.image_height * dst_origin[2]);
  const size_t region_end =
      region_begin + (pixel_size * region[0] * region[1] * region[2]);
  for (size_t i = 0; i < out.size(); ++i) {
    if (i >= region_begin && i < region_end) {
      ASSERT_EQ(in[i - region_begin], out[i]);
    } else {
      ASSERT_EQ(0, out[i]);
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default1D2DArray) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE1D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  cl_image_desc desc1d = {};
  desc1d.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc1d.image_width = 64;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * desc1d.image_width);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc1d, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc desc2darray = {};
  desc2darray.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc2darray.image_width = 128;
  desc2darray.image_height = 8;
  desc2darray.image_array_size = 4;
  UCL::vector<uint8_t> zeros(pixel_size * desc2darray.image_width *
                             desc2darray.image_height *
                             desc2darray.image_array_size);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc2darray, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {desc2darray.image_width / 2,
                          desc2darray.image_height - 1,
                          desc2darray.image_array_size - 1};
  size_t region[3] = {desc1d.image_width, 1, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc2darray.image_width, desc2darray.image_height,
                           desc2darray.image_depth};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));
  const size_t region_begin =
      (pixel_size * dst_origin[0]) +
      (pixel_size * desc2darray.image_width * dst_origin[1]) +
      (pixel_size * desc2darray.image_width * desc2darray.image_height *
       dst_origin[2]);
  const size_t region_end = pixel_size * region[0] * region[1] * region[2];
  for (size_t i = 0; i < out.size(); ++i) {
    if (i >= region_begin && i < region_end) {
      ASSERT_EQ(in[i - region_begin], out[i]);
    } else {
      ASSERT_EQ(0, out[i]);
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default2D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc src_desc = {};
  src_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  src_desc.image_width = 16;
  src_desc.image_height = 16;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * src_desc.image_width *
                          src_desc.image_height);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &src_desc, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc dst_desc = {};
  dst_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  dst_desc.image_width = 32;
  dst_desc.image_height = 32;

  UCL::vector<uint8_t> zeros(pixel_size * dst_desc.image_width *
                             dst_desc.image_height);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &dst_desc, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {dst_desc.image_width / 4, dst_desc.image_height / 4,
                          0};
  size_t region[3] = {src_desc.image_width, src_desc.image_height, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  UCL::vector<uint8_t> out(zeros.size());
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {dst_desc.image_width, dst_desc.image_height, 1};
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  size_t out_index = 0;
  size_t in_index = 0;
  for (size_t out_h = 0; out_h < dst_desc.image_height; ++out_h) {
    for (size_t out_w = 0; out_w < dst_desc.image_width; ++out_w) {
      for (size_t elem = 0; elem < pixel_size; ++elem) {
        if (out_h >= dst_origin[1] && out_h < dst_origin[1] + region[1] &&
            out_w >= dst_origin[0] && out_w < dst_origin[0] + region[0]) {
          ASSERT_EQ(in[in_index], out[out_index])
              << "out_w: " << out_w << ", out_h: " << out_h
              << ", out_index: " << out_index << ", in_index: " << in_index;
          ++in_index;
        } else {
          ASSERT_EQ(0, out[out_index])
              << "out_w: " << out_w << ", out_h: " << out_h
              << ", out_index: " << out_index;
        }
        ++out_index;
      }
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default2D3D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc src_desc = {};
  src_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  src_desc.image_width = 24;
  src_desc.image_height = 24;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * src_desc.image_width *
                          src_desc.image_height);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &src_desc, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc dst_desc = {};
  dst_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  dst_desc.image_width = 32;
  dst_desc.image_height = 32;
  dst_desc.image_depth = 4;
  UCL::vector<uint8_t> zeros(pixel_size * dst_desc.image_width *
                             dst_desc.image_height * dst_desc.image_depth);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &dst_desc, zeros.data(), &error);
  const size_t src_origin[3] = {8, 8, 0};
  const size_t dst_origin[3] = {16, 16, 2};
  const size_t region[3] = {16, 16, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  const size_t read_origin[3] = {0, 0, 0};
  const size_t read_region[3] = {dst_desc.image_width, dst_desc.image_height,
                                 dst_desc.image_depth};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  // TODO: Use row and slice pitches instead of widths and heights.
  const size_t region_row_size = region[0] * pixel_size;
  size_t in_row_elem_begin =
      pixel_size *
      (src_origin[0] + src_origin[1] * src_desc.image_width +
       src_origin[2] * src_desc.image_width * src_desc.image_height);
  size_t out_row_elem_begin =
      pixel_size *
      (dst_origin[0] + dst_origin[1] * dst_desc.image_width +
       dst_origin[2] * dst_desc.image_width * dst_desc.image_height);
  size_t out_row_elem_end = out_row_elem_begin + region_row_size;
  const size_t out_elem_end =
      pixel_size * (dst_origin[0] + region[0] +
                    (dst_origin[1] + region[1] - 1) * dst_desc.image_width +
                    (dst_origin[2] + region[2] - 1) * dst_desc.image_width *
                        dst_desc.image_height);
  size_t out_elem_index = 0;
  while (out_elem_index < out.size()) {
    if (out_elem_index == out_row_elem_begin && out_elem_index < out_elem_end) {
      // In region, compare full row.
      for (size_t i = 0; i < region_row_size; ++i) {
        ASSERT_EQ(in[in_row_elem_begin + i], out[out_row_elem_begin + i])
            << "out_elem_index: " << out_elem_index << " "
            << "out_row_elem_begin: " << out_row_elem_begin << " "
            << "out_row_elem_end: " << out_row_elem_end << " "
            << "out_elem_end: " << out_elem_end << " "
            << "in_row_elem_begin: " << in_row_elem_begin << " "
            << "i: " << i << " ";
      }
      out_elem_index += region_row_size;
      // Next in and out region row indices.
      in_row_elem_begin += pixel_size * src_desc.image_width;
      out_row_elem_begin += pixel_size * dst_desc.image_width;
      out_row_elem_end += pixel_size * dst_desc.image_width;
    } else {
      // Outside of region.
      ASSERT_EQ(0, out[out_elem_index])
          << "out_elem_index: " << out_elem_index << " "
          << "out_row_elem_begin: " << out_row_elem_begin << " "
          << "out_row_elem_end: " << out_row_elem_end << " "
          << "out_elem_end: " << out_elem_end << " "
          << "in_row_elem_begin: " << in_row_elem_begin << " ";
      ++out_elem_index;
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default2D2DArray) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  cl_image_desc src_desc = {};
  src_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  src_desc.image_width = 24;
  src_desc.image_height = 24;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * src_desc.image_width *
                          src_desc.image_height);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &src_desc, in.data(), &error);
  ASSERT_SUCCESS(error);

  cl_image_desc desc2darray = {};
  desc2darray.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc2darray.image_width = 24;
  desc2darray.image_height = 24;
  desc2darray.image_array_size = 4;
  UCL::vector<uint8_t> zeros(pixel_size * desc2darray.image_width *
                             desc2darray.image_height *
                             desc2darray.image_array_size);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc2darray, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {0, 0, desc2darray.image_array_size - 1};
  size_t region[3] = {src_desc.image_width, src_desc.image_height, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc2darray.image_width, desc2darray.image_height,
                           desc2darray.image_array_size};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  const size_t region_begin =
      pixel_size * region[0] * region[1] * dst_origin[2];
  const size_t region_end =
      (pixel_size * region[0] * region[1] * region[2]) + region_begin;

  for (size_t i = 0; i < out.size(); ++i) {
    if (i >= region_begin && i < region_end) {
      ASSERT_EQ(in[i - region_begin], out[i]);
    } else {
      ASSERT_EQ(0, out[i]);
    }
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default1DArray1D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE1D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  cl_image_desc desc1darray = {};
  desc1darray.image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
  desc1darray.image_width = 24;
  desc1darray.image_array_size = 8;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * desc1darray.image_width *
                          desc1darray.image_array_size);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc1darray, in.data(), &error);
  ASSERT_SUCCESS(error);

  cl_image_desc desc1d = {};
  desc1d.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc1d.image_width = 24;
  UCL::vector<uint8_t> zeros(pixel_size * desc1d.image_width);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc1d, zeros.data(), &error);
  ASSERT_SUCCESS(error);
  size_t src_origin[3] = {0, desc1darray.image_array_size / 2, 0};
  size_t dst_origin[3] = {0, 0, 0};
  size_t region[3] = {24, 1, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc1d.image_width, 1, 1};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  const size_t region_begin = pixel_size * region[0] * src_origin[1];

  for (size_t i = 0; i < out.size(); ++i) {
    ASSERT_EQ(in[region_begin + i], out[i]);
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default2DArray2D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D, format) ||
      !UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  cl_int error;
  cl_image_desc desc2darray = {};
  desc2darray.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc2darray.image_width = 24;
  desc2darray.image_height = 24;
  desc2darray.image_array_size = 4;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * desc2darray.image_width *
                          desc2darray.image_height *
                          desc2darray.image_array_size);
  generate_data(in);
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc2darray, in.data(), &error);
  ASSERT_SUCCESS(error);

  cl_image_desc desc2d = {};
  desc2d.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc2d.image_width = 24;
  desc2d.image_height = 24;

  UCL::vector<uint8_t> zeros(pixel_size * desc2d.image_width *
                             desc2d.image_height);

  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &desc2d, zeros.data(), &error);
  ASSERT_SUCCESS(error);

  size_t src_origin[3] = {0, 0, 2};
  size_t dst_origin[3] = {0, 0, 0};
  size_t region[3] = {desc2darray.image_width, desc2darray.image_height, 1};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  size_t read_origin[3] = {0, 0, 0};
  size_t read_region[3] = {desc2d.image_width, desc2d.image_height, 1};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  const size_t region_begin =
      pixel_size * region[0] * region[1] * src_origin[2];

  for (size_t i = 0; i < out.size(); ++i) {
    ASSERT_EQ(in[region_begin + i], out[i]);
  }
}

TEST_P(clEnqueueCopyImageParamTest, Default3D) {
  if (!UCL::isImageFormatSupported(context,
                                   {CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  cl_image_desc src_desc = {};
  src_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  src_desc.image_width = 32;
  src_desc.image_height = 32;
  src_desc.image_depth = 4;
  const size_t pixel_size = UCL::getPixelSize(format);
  UCL::vector<uint8_t> in(pixel_size * src_desc.image_width *
                          src_desc.image_height * src_desc.image_depth);
  generate_data(in);
  cl_int error;
  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &src_desc, in.data(), &error);
  ASSERT_SUCCESS(error);
  cl_image_desc dst_desc = {};
  dst_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  dst_desc.image_width = 32;
  dst_desc.image_height = 32;
  dst_desc.image_depth = 8;
  UCL::vector<uint8_t> zeros(pixel_size * dst_desc.image_width *
                             dst_desc.image_height * dst_desc.image_depth);
  dst_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &format, &dst_desc, zeros.data(), &error);
  const size_t src_origin[3] = {0, 0, 0};
  const size_t dst_origin[3] = {0, 0, 4};
  const size_t region[3] = {32, 32, 4};
  ASSERT_SUCCESS(clEnqueueCopyImage(command_queue, src_image, dst_image,
                                    src_origin, dst_origin, region, 0, nullptr,
                                    &event));
  const size_t read_origin[3] = {0, 0, 0};
  const size_t read_region[3] = {dst_desc.image_width, dst_desc.image_height,
                                 dst_desc.image_depth};
  UCL::vector<uint8_t> out(zeros.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    1, &event, nullptr));

  // TODO: Use row and slice pitches instead of widths and heights.
  const size_t region_row_size = region[0] * pixel_size;
  size_t in_row_elem_begin =
      pixel_size *
      (src_origin[0] + src_origin[1] * src_desc.image_width +
       src_origin[2] * src_desc.image_width * src_desc.image_height);
  size_t out_row_elem_begin =
      pixel_size *
      (dst_origin[0] + dst_origin[1] * dst_desc.image_width +
       dst_origin[2] * dst_desc.image_width * dst_desc.image_height);
  size_t out_row_elem_end = out_row_elem_begin + region_row_size;
  const size_t out_elem_end =
      pixel_size * (dst_origin[0] + region[0] +
                    (dst_origin[1] + region[1] - 1) * dst_desc.image_width +
                    (dst_origin[2] + region[2] - 1) * dst_desc.image_width *
                        dst_desc.image_height);
  size_t out_elem_index = 0;
  while (out_elem_index < out.size()) {
    if (out_elem_index == out_row_elem_begin && out_elem_index < out_elem_end) {
      // In region, compare full row.
      for (size_t i = 0; i < region_row_size; ++i) {
        ASSERT_EQ(in[in_row_elem_begin + i], out[out_row_elem_begin + i])
            << "out_elem_index: " << out_elem_index << " "
            << "out_row_elem_begin: " << out_row_elem_begin << " "
            << "out_row_elem_end: " << out_row_elem_end << " "
            << "out_elem_end: " << out_elem_end << " "
            << "in_row_elem_begin: " << in_row_elem_begin << " "
            << "i: " << i << " ";
      }
      out_elem_index += region_row_size;
      // Next in and out region row indices.
      in_row_elem_begin += pixel_size * src_desc.image_width;
      out_row_elem_begin += pixel_size * dst_desc.image_width;
      out_row_elem_end += pixel_size * dst_desc.image_width;
    } else {
      // Outside of region.
      ASSERT_EQ(0, out[out_elem_index])
          << "out_elem_index: " << out_elem_index << " "
          << "out_row_elem_begin: " << out_row_elem_begin << " "
          << "out_row_elem_end: " << out_row_elem_end << " "
          << "out_elem_end: " << out_elem_end << " "
          << "in_row_elem_begin: " << in_row_elem_begin << " ";
      ++out_elem_index;
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    SNORM_INT8, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SNORM_INT8},
                      cl_image_format{CL_Rx, CL_SNORM_INT8},
                      cl_image_format{CL_A, CL_SNORM_INT8},
                      cl_image_format{CL_INTENSITY, CL_SNORM_INT8},
                      cl_image_format{CL_LUMINANCE, CL_SNORM_INT8},
                      cl_image_format{CL_RG, CL_SNORM_INT8},
                      cl_image_format{CL_RGx, CL_SNORM_INT8},
                      cl_image_format{CL_RA, CL_SNORM_INT8},
                      cl_image_format{CL_RGBA, CL_SNORM_INT8},
                      cl_image_format{CL_ARGB, CL_SNORM_INT8},
                      cl_image_format{CL_BGRA, CL_SNORM_INT8}));

INSTANTIATE_TEST_CASE_P(
    SNORM_INT16, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SNORM_INT16},
                      cl_image_format{CL_Rx, CL_SNORM_INT16},
                      cl_image_format{CL_A, CL_SNORM_INT16},
                      cl_image_format{CL_INTENSITY, CL_SNORM_INT16},
                      cl_image_format{CL_LUMINANCE, CL_SNORM_INT16},
                      cl_image_format{CL_RG, CL_SNORM_INT16},
                      cl_image_format{CL_RGx, CL_SNORM_INT16},
                      cl_image_format{CL_RA, CL_SNORM_INT16},
                      cl_image_format{CL_RGBA, CL_SNORM_INT16}));

INSTANTIATE_TEST_CASE_P(
    UNORM_INT8, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNORM_INT8},
                      cl_image_format{CL_Rx, CL_UNORM_INT8},
                      cl_image_format{CL_A, CL_UNORM_INT8},
                      cl_image_format{CL_INTENSITY, CL_UNORM_INT8},
                      cl_image_format{CL_LUMINANCE, CL_UNORM_INT8},
                      cl_image_format{CL_RG, CL_UNORM_INT8},
                      cl_image_format{CL_RGx, CL_UNORM_INT8},
                      cl_image_format{CL_RA, CL_UNORM_INT8},
                      cl_image_format{CL_RGBA, CL_UNORM_INT8},
                      cl_image_format{CL_ARGB, CL_UNORM_INT8},
                      cl_image_format{CL_BGRA, CL_UNORM_INT8}));

INSTANTIATE_TEST_CASE_P(
    UNORM_INT16, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNORM_INT16},
                      cl_image_format{CL_Rx, CL_UNORM_INT16},
                      cl_image_format{CL_A, CL_UNORM_INT16},
                      cl_image_format{CL_INTENSITY, CL_UNORM_INT16},
                      cl_image_format{CL_LUMINANCE, CL_UNORM_INT16},
                      cl_image_format{CL_RG, CL_UNORM_INT16},
                      cl_image_format{CL_RGx, CL_UNORM_INT16},
                      cl_image_format{CL_RA, CL_UNORM_INT16},
                      cl_image_format{CL_RGBA, CL_UNORM_INT16}));

INSTANTIATE_TEST_CASE_P(
    UNORM_SHORT_565, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_565},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_565}));

INSTANTIATE_TEST_CASE_P(
    UNORM_SHORT_555, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_555},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_555}));

INSTANTIATE_TEST_CASE_P(
    UNORM_INT_101010, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_INT_101010},
                      cl_image_format{CL_RGBx, CL_UNORM_INT_101010}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT8, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT8},
                      cl_image_format{CL_Rx, CL_SIGNED_INT8},
                      cl_image_format{CL_A, CL_SIGNED_INT8},
                      cl_image_format{CL_RG, CL_SIGNED_INT8},
                      cl_image_format{CL_RGx, CL_SIGNED_INT8},
                      cl_image_format{CL_RA, CL_SIGNED_INT8},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT8},
                      cl_image_format{CL_ARGB, CL_SIGNED_INT8},
                      cl_image_format{CL_BGRA, CL_SIGNED_INT8}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT16, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT16},
                      cl_image_format{CL_Rx, CL_SIGNED_INT16},
                      cl_image_format{CL_A, CL_SIGNED_INT16},
                      cl_image_format{CL_RG, CL_SIGNED_INT16},
                      cl_image_format{CL_RGx, CL_SIGNED_INT16},
                      cl_image_format{CL_RA, CL_SIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT32, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT32},
                      cl_image_format{CL_Rx, CL_SIGNED_INT32},
                      cl_image_format{CL_A, CL_SIGNED_INT32},
                      cl_image_format{CL_RG, CL_SIGNED_INT32},
                      cl_image_format{CL_RGx, CL_SIGNED_INT32},
                      cl_image_format{CL_RA, CL_SIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT8, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT8},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT8},
                      cl_image_format{CL_A, CL_UNSIGNED_INT8},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT8},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT8},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT8},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT8},
                      cl_image_format{CL_ARGB, CL_UNSIGNED_INT8},
                      cl_image_format{CL_BGRA, CL_UNSIGNED_INT8}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT16, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT16},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_A, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT32, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT32},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_A, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    HALF_FLOAT, clEnqueueCopyImageParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_HALF_FLOAT},
                      cl_image_format{CL_Rx, CL_HALF_FLOAT},
                      cl_image_format{CL_A, CL_HALF_FLOAT},
                      cl_image_format{CL_INTENSITY, CL_HALF_FLOAT},
                      cl_image_format{CL_LUMINANCE, CL_HALF_FLOAT},
                      cl_image_format{CL_RG, CL_HALF_FLOAT},
                      cl_image_format{CL_RGx, CL_HALF_FLOAT},
                      cl_image_format{CL_RA, CL_HALF_FLOAT},
                      cl_image_format{CL_RGBA, CL_HALF_FLOAT}));

INSTANTIATE_TEST_CASE_P(
    FLOAT, clEnqueueCopyImageParamTest,
    ::testing::Values(
        cl_image_format{CL_R, CL_FLOAT}, cl_image_format{CL_Rx, CL_FLOAT},
        cl_image_format{CL_A, CL_FLOAT},
        cl_image_format{CL_INTENSITY, CL_FLOAT},
        cl_image_format{CL_LUMINANCE, CL_FLOAT},
        cl_image_format{CL_RG, CL_FLOAT}, cl_image_format{CL_RGx, CL_FLOAT},
        cl_image_format{CL_RA, CL_FLOAT}, cl_image_format{CL_RGBA, CL_FLOAT}));

class clEnqueueCopyImageTest : public ucl::CommandQueueTest,
                               TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    cl_int error;
    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_FLOAT;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = 32;
    desc.image_height = 32;
    desc.image_depth = 1;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;
    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     CL_MEM_OBJECT_IMAGE2D,
                                     {CL_RGBA, CL_FLOAT})) {
      GTEST_SKIP();
    }
    src_image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc,
                              nullptr, &error);
    ASSERT_NE(nullptr, src_image);
    ASSERT_SUCCESS(error);
    dst_image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc,
                              nullptr, &error);
    ASSERT_NE(nullptr, dst_image);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (dst_image) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_image));
    }
    if (src_image) {
      EXPECT_SUCCESS(clReleaseMemObject(src_image));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    size_t src_origin[3] = {0, 0, 0};
    size_t dst_origin[3] = {0, 0, 0};
    size_t region[3] = {1, 1, 1};
    EXPECT_EQ_ERRCODE(
        err, clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                                dst_origin, region, num_events, events, event));
  }

  cl_image_desc desc = {};
  cl_mem src_image = nullptr;
  cl_mem dst_image = nullptr;
};

TEST_F(clEnqueueCopyImageTest, InvalidCommandQueue) {
  cl_int error;
  cl_event beginEvent = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};
  cl_event copyEvent;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueCopyImage(nullptr, src_image, dst_image, origin, origin, region,
                         1, &beginEvent, &copyEvent));
  ASSERT_SUCCESS(clReleaseEvent(beginEvent));
}

TEST_F(clEnqueueCopyImageTest, InvalidContext) {
  cl_int error;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  EXPECT_NE(nullptr, otherContext);
  EXPECT_SUCCESS(error);
  cl_command_queue otherCommandQueue =
      clCreateCommandQueue(otherContext, device, 0, &error);
  EXPECT_NE(nullptr, otherCommandQueue);
  EXPECT_SUCCESS(error);

  cl_event beginEvent = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};
  cl_event copyEvent;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyImage(otherCommandQueue, src_image, dst_image, origin,
                         origin, region, 1, &beginEvent, &copyEvent));

  ASSERT_SUCCESS(clReleaseEvent(beginEvent));
  EXPECT_SUCCESS(clReleaseCommandQueue(otherCommandQueue));
  EXPECT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueCopyImageTest, InvalidMemObject) {
  cl_int error;
  cl_event beginEvent = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};
  cl_event copyEventThatShouldNeverBeSet = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueCopyImage(command_queue, nullptr, dst_image,
                                       origin, origin, region, 1, &beginEvent,
                                       &copyEventThatShouldNeverBeSet));
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clEnqueueCopyImage(command_queue, src_image, nullptr,
                                       origin, origin, region, 1, &beginEvent,
                                       &copyEventThatShouldNeverBeSet));

  ASSERT_SUCCESS(clReleaseEvent(beginEvent));
}

TEST_F(clEnqueueCopyImageTest, ImageFormatMismatch) {
  cl_image_format otherFormat;
  otherFormat.image_channel_order = CL_RGBA;
  otherFormat.image_channel_data_type = CL_SNORM_INT8;
  cl_int error;
  cl_mem otherImage = clCreateImage(context, CL_MEM_READ_WRITE, &otherFormat,
                                    &desc, nullptr, &error);
  EXPECT_NE(nullptr, otherImage);
  EXPECT_SUCCESS(error);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};
  EXPECT_EQ_ERRCODE(
      CL_IMAGE_FORMAT_MISMATCH,
      clEnqueueCopyImage(command_queue, otherImage, dst_image, origin, origin,
                         region, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_IMAGE_FORMAT_MISMATCH,
      clEnqueueCopyImage(command_queue, src_image, otherImage, origin, origin,
                         region, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherImage));

  otherFormat.image_channel_order = CL_BGRA;
  otherFormat.image_channel_data_type = CL_SNORM_INT8;
  otherImage = clCreateImage(context, CL_MEM_READ_WRITE, &otherFormat, &desc,
                             nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_NE(nullptr, otherImage);

  EXPECT_EQ_ERRCODE(
      CL_IMAGE_FORMAT_MISMATCH,
      clEnqueueCopyImage(command_queue, otherImage, dst_image, origin, origin,
                         region, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_IMAGE_FORMAT_MISMATCH,
      clEnqueueCopyImage(command_queue, src_image, otherImage, origin, origin,
                         region, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(otherImage));
}

TEST_F(clEnqueueCopyImageTest, InvalidValueSrcOrigin) {
  size_t src_origin[3] = {desc.image_width + 1, 0, 0};
  size_t dst_origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                         dst_origin, region, 0, nullptr, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, nullptr,
                         dst_origin, region, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageTest, InvalidValueDstOrigin) {
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {desc.image_width + 1, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                         dst_origin, region, 0, nullptr, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                         nullptr, region, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyImageTest, InvalidValueRegion) {
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width + 1, desc.image_height,
                      desc.image_depth};

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                         dst_origin, region, 0, nullptr, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin,
                         dst_origin, nullptr, 0, nullptr, nullptr));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueCopyImageTest)

TEST_F(clEnqueueCopyImageTest, MemCopyOverlap) {
  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_SNORM_INT8;

  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  desc.image_width = 16;
  desc.image_height = 4;
  desc.image_depth = 4;
  UCL::vector<uint8_t> in(desc.image_width * desc.image_height *
                          desc.image_depth * UCL::getPixelSize(format));
  generate_data(in);
  cl_int error;
  cl_mem src_image =
      clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &format,
                    &desc, in.data(), &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, src_image);
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {8, 0, 0};
  size_t region[3] = {8, 1, 1};
  ASSERT_EQ_ERRCODE(
      CL_MEM_COPY_OVERLAP,
      clEnqueueCopyImage(command_queue, src_image, src_image, src_origin,
                         dst_origin, region, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseMemObject(src_image));
}

// CL_MEM_OBJECT_ALLOCATION_FAILURE, CL_OUT_OF_RESOURCES ,
// CL_OUT_OF_HOST_MEMORY, CL_INVALID_OPERATION, CL_INVALID_IMAGE_SIZE and
// CL_INVALID_IMAGE_FORMAT are not being tested as they require seperate devices
// to allow creation of an invalid image with which to test.As such, we cannot
// test clEnqueueCopyImage correctly to get the correct error codes
