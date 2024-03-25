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

struct CommandBufferFillImageTest : cl_khr_command_buffer_Test {
  CommandBufferFillImageTest() : test_data(dimension_length) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests assume images are supported by the device
    if (!UCL::hasImageSupport(device)) {
      GTEST_SKIP();
    }

    image_format = {CL_RGBA, CL_SIGNED_INT32};
    image_type = CL_MEM_OBJECT_IMAGE1D;
    image_flags = CL_MEM_COPY_HOST_PTR;

    if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                     image_format)) {
      GTEST_SKIP();
    }

    for (size_t x = 0; x < dimension_length; x++) {
      const cl_uint index = x;
      const cl_uint4 element = {{index, index + 1, index + 2, index + 3}};
      test_data[index] = element;
    }
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
    image = clCreateImage(context, image_flags, &image_format, &image_desc,
                          test_data.data(), &err);
    ASSERT_SUCCESS(err);

    command_buffer = clCreateCommandBufferKHR(1, &command_queue, nullptr, &err);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }

    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
  cl_mem image = nullptr;
  UCL::vector<cl_uint4> test_data;

  cl_image_format image_format;
  cl_image_desc image_desc;
  cl_mem_object_type image_type;
  cl_mem_flags image_flags;

  static constexpr cl_uint4 fill_color = {
      {42, (cl_uint)(-1), 0x80000000, 0x7FFFFFFF}};
  static constexpr size_t half_dimension = 8;
  static constexpr size_t dimension_length = half_dimension + half_dimension;
};

TEST_F(CommandBufferFillImageTest, Sync) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {dimension_length, 1, 1};

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 0, nullptr,
                                       &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 0, nullptr,
                                       &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 2,
                                       sync_points, nullptr, nullptr));
}

TEST_F(CommandBufferFillImageTest, FillFull) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {dimension_length, 1, 1};

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 0, nullptr,
                                       nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  cl_int err = !CL_SUCCESS;
  size_t image_row_pitch = 0;
  size_t image_slice_pitch = 0;
  cl_uint4 *const mapped_image = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, origin, region,
      &image_row_pitch, &image_slice_pitch, 0, nullptr, nullptr, &err));
  EXPECT_TRUE(mapped_image);
  ASSERT_SUCCESS(err);

  ASSERT_EQ(dimension_length * sizeof(cl_uint4), image_row_pitch);
  ASSERT_EQ(0u, image_slice_pitch);

  for (size_t i = 0; i < dimension_length; i++) {
    const ucl::UInt4 result(mapped_image[i]);
    const ucl::UInt4 compare(fill_color);
    ASSERT_EQ(compare, result) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferFillImageTest, FillStart) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {half_dimension, 1, 1};

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 0, nullptr,
                                       nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  const size_t map_origin[] = {0, 0, 0};
  const size_t map_region[] = {dimension_length, 1, 1};

  cl_int err = !CL_SUCCESS;
  size_t image_row_pitch = 0;
  size_t image_slice_pitch = 0;
  cl_uint4 *const mapped_image = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, map_origin, map_region,
      &image_row_pitch, &image_slice_pitch, 0, nullptr, nullptr, &err));
  ASSERT_TRUE(mapped_image);
  ASSERT_SUCCESS(err);

  ASSERT_EQ(dimension_length * sizeof(cl_uint4), image_row_pitch);
  ASSERT_EQ(0u, image_slice_pitch);

  for (size_t i = 0; i < dimension_length; i++) {
    const bool in_region = i < half_dimension;

    const ucl::UInt4 result(mapped_image[i]);
    const ucl::UInt4 compare((in_region) ? fill_color : test_data[i]);
    ASSERT_EQ(result, compare) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferFillImageTest, FillEnd) {
  const size_t origin[] = {half_dimension, 0, 0};
  const size_t region[] = {half_dimension, 1, 1};

  ASSERT_SUCCESS(clCommandFillImageKHR(command_buffer, nullptr, image,
                                       &fill_color, origin, region, 0, nullptr,
                                       nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  const size_t map_origin[] = {0, 0, 0};
  const size_t map_region[] = {dimension_length, 1, 1};

  cl_int err = !CL_SUCCESS;
  size_t image_row_pitch = 0;
  size_t image_slice_pitch = 0;
  cl_uint4 *const mapped_image = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, map_origin, map_region,
      &image_row_pitch, &image_slice_pitch, 0, nullptr, nullptr, &err));
  ASSERT_TRUE(mapped_image);
  ASSERT_SUCCESS(err);

  ASSERT_EQ(dimension_length * sizeof(cl_uint4), image_row_pitch);
  ASSERT_EQ(0u, image_slice_pitch);

  for (size_t i = 0; i < dimension_length; i++) {
    const bool in_region = i >= half_dimension;

    const ucl::UInt4 result(mapped_image[i]);
    const ucl::UInt4 expect((in_region) ? fill_color : test_data[i]);

    ASSERT_EQ(expect, result) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferFillImageTest, InvalidCommandBuffer) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandFillImageKHR(nullptr, nullptr, image, &fill_color, origin,
                            region, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferFillImageTest, InvalidMemObject) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandFillImageKHR(command_buffer, nullptr, nullptr, &fill_color,
                            origin, region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferFillImageTest, InvalidContext) {
  cl_int errcode;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(other_context);
  EXPECT_SUCCESS(errcode);

  cl_mem other_image = clCreateImage(other_context, image_flags, &image_format,
                                     &image_desc, test_data.data(), &errcode);
  EXPECT_TRUE(other_image);
  EXPECT_SUCCESS(errcode);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clCommandFillImageKHR(command_buffer, nullptr, other_image, &fill_color,
                            origin, region, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_image));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}
TEST_F(CommandBufferFillImageTest, NullConfig) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillImageKHR(command_buffer, nullptr, image, nullptr, origin,
                            region, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color,
                            nullptr, region, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            nullptr, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferFillImageTest, OutOfBounds) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length + 1, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            region, 0, nullptr, nullptr, nullptr));
  region[0]--;
  origin[0] = 2;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferFillImageTest, InvalidSyncPoints) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {dimension_length, 1, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            region, 1, nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandFillImageKHR(command_buffer, nullptr, image, &fill_color, origin,
                            region, 0, &sync_point, nullptr, nullptr));
}
