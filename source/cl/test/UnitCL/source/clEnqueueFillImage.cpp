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

namespace {
constexpr size_t QUARTER_DIMENSION_LENGTH = 4;
constexpr size_t HALF_DIMENSION_LENGTH =
    QUARTER_DIMENSION_LENGTH + QUARTER_DIMENSION_LENGTH;
constexpr size_t DIMENSION_LENGTH =
    HALF_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH;
constexpr size_t TOTAL_LENGTH =
    DIMENSION_LENGTH * DIMENSION_LENGTH * DIMENSION_LENGTH;

static const cl_mem_object_type image_types[] = {
    CL_MEM_OBJECT_IMAGE1D,       CL_MEM_OBJECT_IMAGE2D,
    CL_MEM_OBJECT_IMAGE3D,       CL_MEM_OBJECT_IMAGE1D_BUFFER,
    CL_MEM_OBJECT_IMAGE1D_ARRAY, CL_MEM_OBJECT_IMAGE2D_ARRAY};
}  // namespace

struct clEnqueueFillImageTest
    : ucl::CommandQueueTest,
      testing::WithParamInterface<cl_mem_object_type> {
  clEnqueueFillImageTest() : original(TOTAL_LENGTH) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    is1D = (CL_MEM_OBJECT_IMAGE1D == GetParam()) ||
           (CL_MEM_OBJECT_IMAGE1D_BUFFER == GetParam());
    is1DArray = (CL_MEM_OBJECT_IMAGE1D_ARRAY == GetParam());
    is2D = (CL_MEM_OBJECT_IMAGE2D == GetParam());
    is2DArray = (CL_MEM_OBJECT_IMAGE2D_ARRAY == GetParam());

    for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
      for (unsigned int y = 0; y < DIMENSION_LENGTH; y++) {
        for (unsigned int z = 0; z < DIMENSION_LENGTH; z++) {
          const unsigned int index =
              x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
          const cl_uint4 element = {{index, index + 1, index + 2, index + 3}};
          original[index] = element;
        }
      }
    }
    cl_int status;
    if (CL_MEM_OBJECT_IMAGE1D_BUFFER == GetParam()) {
      buffer = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR,
                              DIMENSION_LENGTH * sizeof(cl_uint4), original,
                              &status);
      EXPECT_TRUE(buffer);
      ASSERT_SUCCESS(status);
    }

    const cl_image_desc description = [this]() {
      cl_image_desc image_desc;
      image_desc.image_type = GetParam();
      image_desc.image_width = DIMENSION_LENGTH;
      image_desc.image_height = DIMENSION_LENGTH;
      image_desc.image_depth = DIMENSION_LENGTH;
      image_desc.image_array_size = 1;
      image_desc.image_row_pitch = 0;
      image_desc.image_slice_pitch = 0;
      image_desc.num_mip_levels = 0;
      image_desc.num_samples = 0;
      image_desc.buffer =
          (CL_MEM_OBJECT_IMAGE1D_BUFFER == GetParam()) ? buffer : nullptr;
      return image_desc;
    }();

    image_flags =
        CL_MEM_OBJECT_IMAGE1D_BUFFER == GetParam() ? 0 : CL_MEM_COPY_HOST_PTR;

    void *host_ptr = (CL_MEM_OBJECT_IMAGE1D_BUFFER == GetParam())
                         ? nullptr
                         : original.data();

    image = clCreateImage(context, image_flags, &image_format, &description,
                          host_ptr, &status);
    EXPECT_TRUE(image);
    ASSERT_SUCCESS(status);
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  cl_mem_flags image_flags = 0;
  cl_mem_object_type image_type = 0;
  cl_image_format image_format = {CL_RGBA, CL_SIGNED_INT32};
  cl_mem image = nullptr;
  cl_mem buffer = nullptr;
  cl_event event = nullptr;
  bool is1D = false;
  bool is1DArray = false;
  bool is2D = false;
  bool is2DArray = false;
  UCL::AlignedBuffer<cl_uint4> original;
};

INSTANTIATE_TEST_CASE_P(CheckAPI, clEnqueueFillImageTest,
                        ::testing::ValuesIn(image_types));

TEST_P(clEnqueueFillImageTest, FillFull) {
  if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                   image_format)) {
    GTEST_SKIP();
  }
  const size_t origin[] = {0, 0, 0};

  const size_t region[] = {
      DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : DIMENSION_LENGTH};

  const cl_uint4 fillColor = {{42, (cl_uint)(-1), 0x80000000, 0x7FFFFFFF}};

  ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, &fillColor, origin,
                                    region, 0, nullptr, &event));
  ASSERT_TRUE(event);

  cl_int status = !CL_SUCCESS;
  size_t imageRowPitch = 0;
  size_t imageSlicePitch = 0;
  cl_uint4 *const mappedImage = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, origin, region,
      &imageRowPitch, &imageSlicePitch, 1, &event, nullptr, &status));
  EXPECT_TRUE(mappedImage);
  ASSERT_SUCCESS(status);

  ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageRowPitch);

  if (is1D || is2D) {
    ASSERT_EQ(0u, imageSlicePitch);
  } else if (is1DArray) {
    ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageSlicePitch);
  } else {
    ASSERT_EQ(DIMENSION_LENGTH * DIMENSION_LENGTH * sizeof(cl_uint4),
              imageSlicePitch);
  }

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0, yMax = (is1D || is1DArray) ? 1u : DIMENSION_LENGTH;
         y < yMax; y++) {
      for (unsigned int z = 0, zMax = (is1D || is2D || is1DArray || is2DArray)
                                          ? 1u
                                          : DIMENSION_LENGTH;
           z < zMax; z++) {
        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const ucl::UInt4 result(mappedImage[index]);
        const ucl::UInt4 compare(fillColor);
        ASSERT_EQ(compare, result) << "Coordinates (" << x << ", " << y << ", "
                                   << z << ") linearized to (" << index << ")";
      }
    }
  }
}

TEST_P(clEnqueueFillImageTest, FillStart) {
  if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                   image_format)) {
    GTEST_SKIP();
  }
  const size_t origin[] = {0, 0, 0};

  const size_t region[] = {
      HALF_DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : HALF_DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : HALF_DIMENSION_LENGTH};

  const cl_uint4 fillColor = {{42, (cl_uint)(-1), 0x80000000, 0x7FFFFFFF}};

  ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, &fillColor, origin,
                                    region, 0, nullptr, &event));
  ASSERT_TRUE(event);

  const size_t mapOrigin[] = {0, 0, 0};

  const size_t mapRegion[] = {
      DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : DIMENSION_LENGTH};

  cl_int status = !CL_SUCCESS;
  size_t imageRowPitch = 0;
  size_t imageSlicePitch = 0;
  cl_uint4 *const mappedImage = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, mapOrigin, mapRegion,
      &imageRowPitch, &imageSlicePitch, 1, &event, nullptr, &status));
  EXPECT_TRUE(mappedImage);
  ASSERT_SUCCESS(status);

  ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageRowPitch);

  if (is1D || is2D) {
    ASSERT_EQ(0u, imageSlicePitch);
  } else if (is1DArray) {
    ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageSlicePitch);
  } else {
    ASSERT_EQ(DIMENSION_LENGTH * DIMENSION_LENGTH * sizeof(cl_uint4),
              imageSlicePitch);
  }

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0, yMax = (is1D || is1DArray) ? 1u : DIMENSION_LENGTH;
         y < yMax; y++) {
      for (unsigned int z = 0, zMax = (is1D || is2D || is1DArray || is2DArray)
                                          ? 1u
                                          : DIMENSION_LENGTH;
           z < zMax; z++) {
        const bool inRegion = (x < HALF_DIMENSION_LENGTH) &&
                              (y < HALF_DIMENSION_LENGTH) &&
                              (z < HALF_DIMENSION_LENGTH);

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const ucl::UInt4 result(mappedImage[index]);
        const ucl::UInt4 compare((inRegion) ? fillColor : original[index]);
        ASSERT_EQ(result, compare) << "Coordinates (" << x << ", " << y << ", "
                                   << z << ") linearlized to (" << index << ")";
      }
    }
  }
}

TEST_P(clEnqueueFillImageTest, FillEnd) {
  if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                   image_format)) {
    GTEST_SKIP();
  }
  const size_t origin[] = {
      HALF_DIMENSION_LENGTH, (is1D || is1DArray) ? 0u : HALF_DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 0u : HALF_DIMENSION_LENGTH};

  const size_t region[] = {
      HALF_DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : HALF_DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : HALF_DIMENSION_LENGTH};

  const cl_uint4 fillColor = {{42, (cl_uint)(-1), 0x80000000, 0x7FFFFFFF}};

  ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, &fillColor, origin,
                                    region, 0, nullptr, &event));
  ASSERT_TRUE(event);

  const size_t mapOrigin[] = {0, 0, 0};

  const size_t mapRegion[] = {
      DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : DIMENSION_LENGTH};

  cl_int status = !CL_SUCCESS;
  size_t imageRowPitch = 0;
  size_t imageSlicePitch = 0;
  cl_uint4 *const mappedImage = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, mapOrigin, mapRegion,
      &imageRowPitch, &imageSlicePitch, 1, &event, nullptr, &status));
  EXPECT_TRUE(mappedImage);
  ASSERT_SUCCESS(status);

  ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageRowPitch);

  if (is1D || is2D) {
    ASSERT_EQ(0u, imageSlicePitch);
  } else if (is1DArray) {
    ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageSlicePitch);
  } else {
    ASSERT_EQ(DIMENSION_LENGTH * DIMENSION_LENGTH * sizeof(cl_uint4),
              imageSlicePitch);
  }

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0, yMax = (is1D || is1DArray) ? 1u : DIMENSION_LENGTH;
         y < yMax; y++) {
      for (unsigned int z = 0, zMax = (is1D || is2D || is1DArray || is2DArray)
                                          ? 1u
                                          : DIMENSION_LENGTH;
           z < zMax; z++) {
        const bool inRegion =
            (HALF_DIMENSION_LENGTH <= x) && (x < DIMENSION_LENGTH) &&
            ((is1D || is1DArray) ||
             ((HALF_DIMENSION_LENGTH <= y) && (y < DIMENSION_LENGTH))) &&
            ((is1D || is2D || is1DArray || is2DArray) ||
             ((HALF_DIMENSION_LENGTH <= z) && (z < DIMENSION_LENGTH)));

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const ucl::UInt4 result(mappedImage[index]);
        const ucl::UInt4 expect((inRegion) ? fillColor : original[index]);
        ASSERT_EQ(expect, result) << "Coordinates (" << x << ", " << y << ", "
                                  << z << ") linearlized to (" << index << ")";
      }
    }
  }
}

TEST_P(clEnqueueFillImageTest, FillMiddle) {
  if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                   image_format)) {
    GTEST_SKIP();
  }
  const size_t origin[] = {
      QUARTER_DIMENSION_LENGTH,
      (is1D || is1DArray) ? 0u : QUARTER_DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 0u : QUARTER_DIMENSION_LENGTH};

  const size_t region[] = {
      HALF_DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : HALF_DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : HALF_DIMENSION_LENGTH};

  const cl_uint4 fillColor = {{42, (cl_uint)(-1), 0x80000000, 0x7FFFFFFF}};

  ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, &fillColor, origin,
                                    region, 0, nullptr, &event));
  ASSERT_TRUE(event);

  const size_t mapOrigin[] = {0, 0, 0};

  const size_t mapRegion[] = {
      DIMENSION_LENGTH, (is1D || is1DArray) ? 1u : DIMENSION_LENGTH,
      (is1D || is1DArray || is2D || is2DArray) ? 1u : DIMENSION_LENGTH};

  cl_int status = !CL_SUCCESS;
  size_t imageRowPitch = 0;
  size_t imageSlicePitch = 0;
  cl_uint4 *const mappedImage = static_cast<cl_uint4 *>(clEnqueueMapImage(
      command_queue, image, CL_TRUE, CL_MAP_READ, mapOrigin, mapRegion,
      &imageRowPitch, &imageSlicePitch, 1, &event, nullptr, &status));
  EXPECT_TRUE(mappedImage);
  ASSERT_SUCCESS(status);

  ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageRowPitch);

  if (is1D || is2D) {
    ASSERT_EQ(0u, imageSlicePitch);
  } else if (is1DArray) {
    ASSERT_EQ(DIMENSION_LENGTH * sizeof(cl_uint4), imageSlicePitch);
  } else {
    ASSERT_EQ(DIMENSION_LENGTH * DIMENSION_LENGTH * sizeof(cl_uint4),
              imageSlicePitch);
  }

  for (unsigned int x = 0; x < DIMENSION_LENGTH; x++) {
    for (unsigned int y = 0, yMax = (is1D || is1DArray) ? 1u : DIMENSION_LENGTH;
         y < yMax; y++) {
      for (unsigned int z = 0, zMax = (is1D || is2D || is1DArray || is2DArray)
                                          ? 1u
                                          : DIMENSION_LENGTH;
           z < zMax; z++) {
        const bool inRegion =
            (QUARTER_DIMENSION_LENGTH <= x) &&
            (x < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH) &&
            ((is1D || is1DArray) ||
             ((QUARTER_DIMENSION_LENGTH <= y) &&
              (y < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH))) &&
            ((is1D || is2D || is1DArray || is2DArray) ||
             ((QUARTER_DIMENSION_LENGTH <= z) &&
              (z < QUARTER_DIMENSION_LENGTH + HALF_DIMENSION_LENGTH)));

        const unsigned int index =
            x + (DIMENSION_LENGTH * (y + DIMENSION_LENGTH * z));
        const ucl::UInt4 result(mappedImage[index]);
        const ucl::UInt4 expect((inRegion) ? fillColor : original[index]);
        ASSERT_EQ(expect, result) << "Coordinates (" << x << ", " << y << ", "
                                  << z << ") linearlized to (" << index << ")";
      }
    }
  }
}
