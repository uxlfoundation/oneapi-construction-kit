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

struct CommandBufferCopyImageToBufferTest : cl_khr_command_buffer_Test {
  CommandBufferCopyImageToBufferTest()
      : test_data(sizeof(cl_int4) * image_elements) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests assume images are supported by the device
    if (!UCL::hasImageSupport(device)) {
      GTEST_SKIP();
    }

    const cl_image_format image_format = {CL_RGBA, CL_SIGNED_INT32};
    const cl_mem_flags image_flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
    const cl_mem_object_type image_type = CL_MEM_OBJECT_IMAGE2D;
    if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                     image_format)) {
      GTEST_SKIP();
    }

    cl_image_desc image_desc;
    image_desc.image_type = image_type;
    image_desc.image_width = image_width;
    image_desc.image_height = image_height;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 1;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    auto &generator = ucl::Environment::instance->GetInputGenerator();
    generator.GenerateIntData(test_data);

    cl_int err = !CL_SUCCESS;
    image = clCreateImage(context, image_flags, &image_format, &image_desc,
                          test_data.data(), &err);
    ASSERT_SUCCESS(err);

    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, test_data.size(),
                            nullptr, &err);
    ASSERT_SUCCESS(err);

    command_buffer = clCreateCommandBufferKHR(1, &command_queue, nullptr, &err);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }

    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  static constexpr size_t image_width = 32;
  static constexpr size_t image_height = 32;
  static constexpr size_t image_elements = image_width * image_height;

  cl_mem image = nullptr;
  cl_mem buffer = nullptr;

  cl_command_buffer_khr command_buffer = nullptr;
  std::vector<uint8_t> test_data;
};

TEST_F(CommandBufferCopyImageToBufferTest, Default) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_SUCCESS(clCommandCopyImageToBufferKHR(command_buffer, nullptr, image,
                                               buffer, origin, region, 0, 0,
                                               nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  const size_t size = test_data.size();
  std::vector<uint8_t> out(size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0, size,
                                     out.data(), 0, nullptr, nullptr));

  ASSERT_EQ(test_data, out);
}

TEST_F(CommandBufferCopyImageToBufferTest, Sync) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandCopyImageToBufferKHR(
      command_buffer, nullptr, image, buffer, origin, region, 0, 0, nullptr,
      &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyImageToBufferKHR(
      command_buffer, nullptr, image, buffer, origin, region, 0, 0, nullptr,
      &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyImageToBufferKHR(command_buffer, nullptr, image,
                                               buffer, origin, region, 0, 2,
                                               sync_points, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidCommandBuffer) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandCopyImageToBufferKHR(nullptr, nullptr, image, buffer, origin,
                                    region, 0, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidContext) {
  cl_int error;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_NE(nullptr, other_context);

  cl_mem other_buffer = clCreateBuffer(
      other_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
      sizeof(cl_float4) * image_elements, test_data.data(), &error);
  EXPECT_SUCCESS(error);
  EXPECT_NE(nullptr, other_buffer);

  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, other_buffer, origin,
                        region, 0, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidMemObject) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, nullptr, origin, region,
                        0, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, nullptr, buffer, origin,
                        region, 0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidSrcOrigin) {
  const size_t origin[] = {image_width + 1, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        0, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, nullptr, region,
                        0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidSrcOriginPlusSrcRegion) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width + 1, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidDstOffset) {
  const size_t origin[3] = {0, 0, 0};
  const size_t region[3] = {image_width, image_height, 1};
  const size_t offset = image_elements + 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        offset, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidDstOffsetPlusDstCb) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  const size_t offset = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        offset, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidNullRegion) {
  const size_t origin[] = {0, 0, 0};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, nullptr,
                        0, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidOriginRegionRules) {
  const size_t origin[] = {0, 1, 0};
  const size_t region[] = {image_width, image_height, 1};
  const size_t offset = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        offset, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyImageToBufferTest, InvalidSyncPoints) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        0, 1, nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyImageToBufferKHR(
                        command_buffer, nullptr, image, buffer, origin, region,
                        0, 0, &sync_point, nullptr, nullptr));
}
