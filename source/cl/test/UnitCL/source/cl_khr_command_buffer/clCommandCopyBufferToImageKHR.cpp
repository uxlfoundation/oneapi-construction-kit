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
#include "cl_khr_command_buffer.h"

struct CommandBufferCopyBufferToImageTest : cl_khr_command_buffer_Test {
  CommandBufferCopyBufferToImageTest() : test_data(image_elements) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests assume images are supported by the device
    if (!UCL::hasImageSupport(device)) {
      GTEST_SKIP();
    }

    const cl_image_format image_format = {CL_RGBA, CL_FLOAT};
    const cl_mem_flags image_flags = CL_MEM_READ_WRITE;
    const cl_mem_object_type image_type = CL_MEM_OBJECT_IMAGE2D;
    if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                     image_format)) {
      GTEST_SKIP();
    }

    for (size_t index = 0; index < image_elements; index++) {
      for (size_t element = 0; element < 4; element++) {
        test_data[index].s[element] =
            (cl_float)(index + 42) / (cl_float)(element + 3);
      }
    }

    cl_int err = !CL_SUCCESS;
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            sizeof(cl_float4) * image_elements,
                            test_data.data(), &err);
    ASSERT_SUCCESS(err);

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

    image = clCreateImage(context, image_flags, &image_format, &image_desc,
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

  cl_mem buffer = nullptr;
  cl_mem image = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  UCL::vector<cl_float4> test_data;
};

TEST_F(CommandBufferCopyBufferToImageTest, Default) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_SUCCESS(clCommandCopyBufferToImageKHR(command_buffer, nullptr, buffer,
                                               image, 0, origin, region, 0,
                                               nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  UCL::vector<cl_float4> out(image_elements);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, out.data(), 0, nullptr,
                                    nullptr));
  for (size_t i = 0; i < image_elements; i++) {
    const ucl::Float4 compare(out[i]);
    const ucl::Float4 reference(test_data[i]);

    ASSERT_EQ(compare, reference) << " Failed at index: " << i;
  }
}

TEST_F(CommandBufferCopyBufferToImageTest, Sync) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandCopyBufferToImageKHR(
      command_buffer, nullptr, buffer, image, 0, origin, region, 0, nullptr,
      &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferToImageKHR(
      command_buffer, nullptr, buffer, image, 0, origin, region, 0, nullptr,
      &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferToImageKHR(command_buffer, nullptr, buffer,
                                               image, 0, origin, region, 2,
                                               sync_points, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidCommandBuffer) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandCopyBufferToImageKHR(nullptr, nullptr, buffer, image, 0, origin,
                                    region, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidContext) {
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
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, other_buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidMemObject) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, nullptr, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, nullptr, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidSrcOffset) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  const size_t buffer_size = image_elements * sizeof(cl_float4);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, buffer_size + 1,
                        origin, region, 0, nullptr, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 1, origin,
                        region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidDstOrigin) {
  size_t origin[] = {1, 0, 0};
  const size_t region[] = {image_width, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));

  origin[0] = 0;
  origin[1] = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, nullptr,
                        region, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidDstRegion) {
  const size_t origin[] = {0, 0, 0};
  size_t region[] = {image_width + 1, image_height, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
  region[0] = image_width;
  region[1] = image_height + 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));
  region[1] = image_height;
  region[2] = 2;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        nullptr, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandBufferCopyBufferToImageTest, InvalidSyncPoints) {
  const size_t origin[] = {0, 0, 0};
  const size_t region[] = {image_width, image_height, 1};

  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 1, nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandCopyBufferToImageKHR(
                        command_buffer, nullptr, buffer, image, 0, origin,
                        region, 0, &sync_point, nullptr, nullptr));
}
