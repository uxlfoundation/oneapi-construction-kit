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

struct clGetImageInfoParamTest : ucl::ContextTest,
                                 testing::WithParamInterface<cl_image_format> {
  void SetUp() override {
    ContextTest::SetUp();
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    format = GetParam();
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    ContextTest::TearDown();
  }

  template <cl_mem_object_type>
  cl_int createImage();

  void testFormat() {
    size_t size;
    ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
    ASSERT_EQ(sizeof(cl_image_format), size);
    cl_image_format image_format;
    ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT,
                                  sizeof(cl_image_format), &image_format,
                                  nullptr));
    ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
    ASSERT_EQ(format.image_channel_data_type,
              image_format.image_channel_data_type);
  }

  cl_image_format format = {};
  cl_image_desc desc = {};
  cl_mem image = nullptr;
  cl_mem buffer = nullptr;
  /// @brief Factor to shrink initial image sizes by.
  ///
  /// Setting this to 4 or lower is guaranteed to cause memory usage issues
  /// with Qemu, setting it to 1 is known to cause sporadic memory allocation
  /// issues with address and thread sanitizers.
  ///
  /// Because the minimum legal values for some maximums in embedded profile
  /// are 2048 then setting this beyond 2048 may hinder testing.
  const size_t scale = 128u;
};

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE1D>() {
  desc.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc.image_width = getDeviceImage2dMaxWidth() / scale;
  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>() {
  cl_int error;
  const size_t width = getDeviceImageMaxBufferSize() / scale;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                          width * UCL::getPixelSize(format), nullptr, &error);
  if (CL_SUCCESS != error) {
    return error;
  }

  desc.image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
  desc.image_width = width;
  desc.buffer = buffer;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>() {
  desc.image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
  desc.image_width = getDeviceImage2dMaxWidth() / scale;
  desc.image_array_size = getDeviceImageMaxArraySize();
  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    desc.image_array_size /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE2D>() {
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = getDeviceImage2dMaxWidth() / scale;
  desc.image_height = getDeviceImage2dMaxHeight() / scale;
  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    desc.image_height /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>() {
  desc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc.image_width = getDeviceImage2dMaxWidth() / scale;
  desc.image_height = getDeviceImage2dMaxHeight() / scale;
  desc.image_array_size = getDeviceImageMaxArraySize() / scale;
  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    desc.image_height /= 2;
    desc.image_array_size /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

template <>
cl_int clGetImageInfoParamTest::createImage<CL_MEM_OBJECT_IMAGE3D>() {
  desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  desc.image_width = getDeviceImage3dMaxWidth() / scale;
  desc.image_height = getDeviceImage3dMaxHeight() / scale;
  desc.image_depth = getDeviceImage3dMaxDepth() / scale;
  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  // NOTE: If image creation fails, reduce image dimensions.
  while (CL_MEM_OBJECT_ALLOCATION_FAILURE == error ||
         CL_OUT_OF_RESOURCES == error) {
    desc.image_width /= 2;
    desc.image_height /= 2;
    desc.image_depth /= 2;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
  }
  return error;
}

TEST_P(clGetImageInfoParamTest, DefaultFormat1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(0, slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(0, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(0, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(0, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(nullptr, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples1D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

TEST_P(clGetImageInfoParamTest, DefaultFormat1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(0, slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(0, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(0, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(0, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(buffer, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples1DBuffer) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_BUFFER, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_BUFFER>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

TEST_P(clGetImageInfoParamTest, DefaultFormat1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(0, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(0, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(desc.image_array_size, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(nullptr, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples1DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE1D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE1D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

TEST_P(clGetImageInfoParamTest, DefaultFormat2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(0, slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(desc.image_height, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(0, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(0, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(nullptr, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples2D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

TEST_P(clGetImageInfoParamTest, DefaultFormat2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(desc.image_width * desc.image_height * UCL::getPixelSize(format),
            slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(desc.image_height, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(0, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(desc.image_array_size, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(nullptr, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples2DArray) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE2D_ARRAY, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE2D_ARRAY>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

TEST_P(clGetImageInfoParamTest, DefaultFormat3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_image_format), size);
  cl_image_format image_format;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format),
                                &image_format, nullptr));
  ASSERT_EQ(format.image_channel_order, image_format.image_channel_order);
  ASSERT_EQ(format.image_channel_data_type,
            image_format.image_channel_data_type);
}

TEST_P(clGetImageInfoParamTest, DefaultElementSize3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, sizeof(size_t),
                                nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t elementSize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE, size,
                                &elementSize, nullptr));
  ASSERT_EQ(UCL::getPixelSize(format), elementSize);
}

TEST_P(clGetImageInfoParamTest, DefaultRowPitch3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ROW_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t rowPitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_ROW_PITCH, size, &rowPitch, nullptr));
  ASSERT_EQ(desc.image_width * UCL::getPixelSize(format), rowPitch);
}

TEST_P(clGetImageInfoParamTest, DefaultSlicePitch3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t slicePitch;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_SLICE_PITCH, size, &slicePitch, nullptr));
  ASSERT_EQ(desc.image_width * desc.image_height * UCL::getPixelSize(format),
            slicePitch);
}

TEST_P(clGetImageInfoParamTest, DefaultWidth3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageWidth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_WIDTH, size, &imageWidth, nullptr));
  ASSERT_EQ(desc.image_width, imageWidth);
}

TEST_P(clGetImageInfoParamTest, DefaultHeight3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageHeight;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_HEIGHT, size, &imageHeight, nullptr));
  ASSERT_EQ(desc.image_height, imageHeight);
}

TEST_P(clGetImageInfoParamTest, DefaultDepth3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageDepth;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_DEPTH, size, &imageDepth, nullptr));
  ASSERT_EQ(desc.image_depth, imageDepth);
}

TEST_P(clGetImageInfoParamTest, DefaultArraySize3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t imageArraySize;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE, size,
                                &imageArraySize, nullptr));
  ASSERT_EQ(0, imageArraySize);
}

TEST_P(clGetImageInfoParamTest, DefaultBuffer3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem imageBuffer;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_BUFFER, size,
                                static_cast<void *>(&imageBuffer), nullptr));
  ASSERT_EQ(nullptr, imageBuffer);
}

TEST_P(clGetImageInfoParamTest, DefaultNumMipLevels3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumMipLevels;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS, size,
                                &imageNumMipLevels, nullptr));
  ASSERT_EQ(0, imageNumMipLevels);
}

TEST_P(clGetImageInfoParamTest, DefaultNumSamples3D) {
  if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                   CL_MEM_OBJECT_IMAGE3D, format)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(createImage<CL_MEM_OBJECT_IMAGE3D>());
  size_t size;
  ASSERT_SUCCESS(
      clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint imageNumSamples;
  ASSERT_SUCCESS(clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES, size,
                                &imageNumSamples, nullptr));
  ASSERT_EQ(0, imageNumSamples);
}

INSTANTIATE_TEST_CASE_P(
    SNORM_INT8, clGetImageInfoParamTest,
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

#if !defined(__clang_analyzer__)
// Only include the full set of parameters if not running clang analyzer (or
// clang-tidy), they'll all result in basically the same code but it takes a
// long time to analyze all of them.
INSTANTIATE_TEST_CASE_P(
    SNORM_INT16, clGetImageInfoParamTest,
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
    UNORM_INT8, clGetImageInfoParamTest,
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
    UNORM_INT16, clGetImageInfoParamTest,
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
    UNORM_SHORT_565, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_565},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_565}));

INSTANTIATE_TEST_CASE_P(
    UNORM_SHORT_555, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_555},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_555}));

INSTANTIATE_TEST_CASE_P(
    UNORM_INT_101010, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_INT_101010},
                      cl_image_format{CL_RGBx, CL_UNORM_INT_101010}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT8, clGetImageInfoParamTest,
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
    SIGNED_INT16, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT16},
                      cl_image_format{CL_Rx, CL_SIGNED_INT16},
                      cl_image_format{CL_A, CL_SIGNED_INT16},
                      cl_image_format{CL_RG, CL_SIGNED_INT16},
                      cl_image_format{CL_RGx, CL_SIGNED_INT16},
                      cl_image_format{CL_RA, CL_SIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT32, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT32},
                      cl_image_format{CL_Rx, CL_SIGNED_INT32},
                      cl_image_format{CL_A, CL_SIGNED_INT32},
                      cl_image_format{CL_RG, CL_SIGNED_INT32},
                      cl_image_format{CL_RGx, CL_SIGNED_INT32},
                      cl_image_format{CL_RA, CL_SIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT8, clGetImageInfoParamTest,
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
    UNSIGNED_INT16, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT16},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_A, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT32, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT32},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_A, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    HALF_FLOAT, clGetImageInfoParamTest,
    ::testing::Values(cl_image_format{CL_R, CL_HALF_FLOAT},
                      cl_image_format{CL_Rx, CL_HALF_FLOAT},
                      cl_image_format{CL_A, CL_HALF_FLOAT},
                      cl_image_format{CL_INTENSITY, CL_HALF_FLOAT},
                      cl_image_format{CL_LUMINANCE, CL_HALF_FLOAT},
                      cl_image_format{CL_RG, CL_HALF_FLOAT},
                      cl_image_format{CL_RGx, CL_HALF_FLOAT},
                      cl_image_format{CL_RA, CL_HALF_FLOAT},
                      cl_image_format{CL_RGBA, CL_HALF_FLOAT}));
#endif

INSTANTIATE_TEST_CASE_P(
    FLOAT, clGetImageInfoParamTest,
    ::testing::Values(
        cl_image_format{CL_R, CL_FLOAT}, cl_image_format{CL_Rx, CL_FLOAT},
        cl_image_format{CL_A, CL_FLOAT},
        cl_image_format{CL_INTENSITY, CL_FLOAT},
        cl_image_format{CL_LUMINANCE, CL_FLOAT},
        cl_image_format{CL_RG, CL_FLOAT}, cl_image_format{CL_RGx, CL_FLOAT},
        cl_image_format{CL_RA, CL_FLOAT}, cl_image_format{CL_RGBA, CL_FLOAT}));

class clGetImageInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_FLOAT;
    // Note: This test redefines hasImageSupport from the parent class
    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     CL_MEM_OBJECT_IMAGE2D, format)) {
      GTEST_SKIP();
    }

    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = 128;
    desc.image_height = 128;
    desc.image_depth = 0;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
    EXPECT_TRUE(image);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    ContextTest::TearDown();
  }

  cl_mem image = nullptr;
};

TEST_F(clGetImageInfoTest, InvalidValueParamName) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetImageInfo(image, (cl_image_info)0xFFFFFFFF, 0, nullptr, &size));
}

TEST_F(clGetImageInfoTest, InvalidValueParamValueSize) {
  cl_image_format format;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetImageInfo(image, CL_IMAGE_FORMAT, sizeof(cl_image_format) - 1,
                     &format, nullptr));
  size_t sizeValue;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_ELEMENT_SIZE,
                                   sizeof(size_t) - 1, &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_ROW_PITCH,
                                   sizeof(size_t) - 1, &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_SLICE_PITCH,
                                   sizeof(size_t) - 1, &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_WIDTH, sizeof(size_t) - 1,
                                   &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_HEIGHT, sizeof(size_t) - 1,
                                   &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_DEPTH, sizeof(size_t) - 1,
                                   &sizeValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_ARRAY_SIZE,
                                   sizeof(size_t) - 1, &sizeValue, nullptr));
  cl_mem bufferValue;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_BUFFER, sizeof(cl_mem) - 1,
                                   static_cast<void *>(&bufferValue), nullptr));
  cl_uint numValue;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_NUM_MIP_LEVELS,
                                   sizeof(cl_uint) - 1, &numValue, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetImageInfo(image, CL_IMAGE_NUM_SAMPLES,
                                   sizeof(cl_uint) - 1, &numValue, nullptr));
}

TEST_F(clGetImageInfoTest, InvalidMemObject) {
  cl_image_format format;
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clGetImageInfo(nullptr, CL_IMAGE_FORMAT,
                                   sizeof(cl_image_format), &format, nullptr));
}

// TODO: TEST_F(clGetImageInfoTest, OutOfResources) {}
// TODO: TEST_F(clGetImageInfoTest, OutOfHostResources) {}
