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
#include "cl_khr_command_buffer.h"

struct CommandBufferCopyImageTest : cl_khr_command_buffer_Test {
  CommandBufferCopyImageTest()
      : src_data(sizeof(cl_uint4) * dimension_length),
        dst_data(sizeof(cl_uint4) * dimension_length) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests assume images are supported by the device
    if (!UCL::hasImageSupport(device)) {
      GTEST_SKIP();
    }

    const cl_image_format image_format = {CL_RGBA, CL_SIGNED_INT32};
    const cl_mem_object_type image_type = CL_MEM_OBJECT_IMAGE1D;
    const cl_mem_flags image_flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
    if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                     image_format)) {
      GTEST_SKIP();
    }

    auto &generator = ucl::Environment::instance->GetInputGenerator();
    generator.GenerateIntData(src_data);
    generator.GenerateIntData(dst_data);

    // 1D image
    image_desc.image_type = image_type;
    image_desc.image_width = dimension_length;
    image_desc.image_height = 0;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 1;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    cl_int err = !CL_SUCCESS;
    src_image = clCreateImage(context, image_flags, &image_format, &image_desc,
                              src_data.data(), &err);
    ASSERT_SUCCESS(err);

    dst_image = clCreateImage(context, image_flags, &image_format, &image_desc,
                              dst_data.data(), &err);
    ASSERT_SUCCESS(err);

    command_buffer = clCreateCommandBufferKHR(1, &command_queue, nullptr, &err);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (dst_image) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_image));
    }
    if (src_image) {
      EXPECT_SUCCESS(clReleaseMemObject(src_image));
    }

    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }

  std::vector<uint8_t> src_data;
  std::vector<uint8_t> dst_data;
  static constexpr size_t half_dimension = 8;
  static constexpr size_t dimension_length = half_dimension + half_dimension;

  cl_image_desc image_desc;
  cl_mem src_image = nullptr;
  cl_mem dst_image = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
};

TEST_F(CommandBufferCopyImageTest, Sync) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {dimension_length, 1, 1};

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 0,
                                       nullptr, &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 0,
                                       nullptr, &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 2,
                                       sync_points, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageTest, CopyFull) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {dimension_length, 1, 1};

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 0,
                                       nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  std::vector<uint8_t> out(src_data.size());
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE, origin,
                                    region, 0, 0, out.data(), 0, nullptr,
                                    nullptr));
  ASSERT_EQ(src_data, out);
}

TEST_F(CommandBufferCopyImageTest, CopyStart) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {half_dimension, 1, 1};

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 0,
                                       nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  std::vector<uint8_t> out(src_data.size());
  const size_t read_region[] = {dimension_length, 1, 1};
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE, origin,
                                    read_region, 0, 0, out.data(), 0, nullptr,
                                    nullptr));

  for (size_t i = 0; i < src_data.size(); ++i) {
    const bool in_region = i < (half_dimension * sizeof(cl_uint4));
    const uint8_t compare = (in_region) ? src_data[i] : dst_data[i];
    ASSERT_EQ(compare, out[i]) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferCopyImageTest, CopyEnd) {
  const size_t origin[] = {half_dimension, 0, 0};
  const size_t region[] = {half_dimension, 1, 1};

  ASSERT_SUCCESS(clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                       dst_image, origin, origin, region, 0,
                                       nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  std::vector<uint8_t> out(src_data.size());
  const size_t read_origin[] = {0, 0, 0};
  const size_t read_region[] = {dimension_length, 1, 1};
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_TRUE,
                                    read_origin, read_region, 0, 0, out.data(),
                                    0, nullptr, nullptr));

  for (size_t i = 0; i < src_data.size(); ++i) {
    const bool in_region = i >= (half_dimension * sizeof(cl_uint4));
    const uint8_t compare = (in_region) ? src_data[i] : dst_data[i];
    ASSERT_EQ(compare, out[i]) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferCopyImageTest, InvalidCommandBuffer) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandCopyImageKHR(nullptr, nullptr, src_image, dst_image, origin,
                            origin, region, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          dst_image, origin, origin, region, 0,
                                          nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageTest, InvalidMemObject) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyImageKHR(command_buffer, nullptr, nullptr, dst_image, origin,
                            origin, region, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyImageKHR(command_buffer, nullptr, src_image, nullptr, origin,
                            origin, region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageTest, ImageFormatMismatch) {
  cl_image_format other_format;
  other_format.image_channel_order = CL_RGBA;
  other_format.image_channel_data_type = CL_SNORM_INT8;
  cl_int error;
  cl_mem other_image = clCreateImage(context, CL_MEM_READ_WRITE, &other_format,
                                     &image_desc, nullptr, &error);
  EXPECT_NE(nullptr, other_image);
  EXPECT_SUCCESS(error);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(CL_IMAGE_FORMAT_MISMATCH,
                    clCommandCopyImageKHR(command_buffer, nullptr, other_image,
                                          dst_image, origin, origin, region, 0,
                                          nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_IMAGE_FORMAT_MISMATCH,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          other_image, origin, origin, region,
                                          0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseMemObject(other_image));
}
TEST_F(CommandBufferCopyImageTest, InvalidSrcOrigin) {
  size_t src_origin[3] = {dimension_length + 1, 0, 0};
  size_t dst_origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyImageKHR(command_buffer, nullptr, src_image, dst_image,
                            src_origin, dst_origin, region, 0, nullptr, nullptr,
                            nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageKHR(
                        command_buffer, nullptr, src_image, dst_image, nullptr,
                        dst_origin, region, 0, nullptr, nullptr, nullptr));
}
TEST_F(CommandBufferCopyImageTest, InvalidDstOrigin) {
  size_t src_origin[3] = {0, 0, 0};
  size_t dst_origin[3] = {dimension_length + 1, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyImageKHR(command_buffer, nullptr, src_image, dst_image,
                            src_origin, dst_origin, region, 0, nullptr, nullptr,
                            nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyImageKHR(command_buffer, nullptr, src_image, dst_image,
                            src_origin, nullptr, region, 0, nullptr, nullptr,
                            nullptr));
}

TEST_F(CommandBufferCopyImageTest, InvalidRegion) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length + 1, 1, 1};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          dst_image, origin, origin, region, 0,
                                          nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          dst_image, origin, origin, nullptr, 0,
                                          nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageTest, InvalidSyncPoints) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          dst_image, origin, origin, region, 1,
                                          nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyImageKHR(command_buffer, nullptr, src_image,
                                          dst_image, origin, origin, region, 0,
                                          &sync_point, nullptr, nullptr));
}
