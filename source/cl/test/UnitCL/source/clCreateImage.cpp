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

class clCreateImageTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    ContextTest::TearDown();
  }

  cl_mem image = nullptr;
};

TEST_F(clCreateImageTest, InvalidContext) {
  const cl_image_format image_format = {};
  const cl_image_desc image_desc = {};
  cl_int errcode;
  EXPECT_EQ(nullptr, clCreateImage(nullptr, 0, &image_format, &image_desc,
                                   nullptr, &errcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errcode);
}

TEST_F(clCreateImageTest, InvalidValueFlags) {
  cl_mem_flags flags;
  memset(&flags, 0xff, sizeof(cl_mem_flags));
  const cl_image_format image_format = {};
  const cl_image_desc image_desc = {};
  cl_int errcode;
  image = clCreateImage(context, flags, &image_format, &image_desc, nullptr,
                        &errcode);
  EXPECT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateImageTest, InvalidValueFlagsReadOnlyWriteOnly) {
  const cl_mem_flags flags = CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY;
  const cl_image_format image_format = {};
  const cl_image_desc image_desc = {};
  cl_int errcode;
  image = clCreateImage(context, flags, &image_format, &image_desc, nullptr,
                        &errcode);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateImageTest, InvalidValueFlagsReadWriteReadOnly) {
  cl_mem_flags flags = CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY;
  const cl_image_format image_format = {};
  const cl_image_desc image_desc = {};
  cl_int errcode;
  flags = CL_MEM_READ_WRITE | CL_MEM_READ_ONLY;
  image = clCreateImage(context, flags, &image_format, &image_desc, nullptr,
                        &errcode);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateImageTest, InvalidImageFormatDescriptor) {
  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  image_desc.image_width = 1;
  image_desc.image_height = 1;
  image_desc.image_depth = 0;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = 0;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;

  cl_int errorcode;
  EXPECT_FALSE(
      clCreateImage(context, 0, nullptr, &image_desc, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, errorcode);
}

TEST_F(clCreateImageTest, InvalidImageDesc) {
  const cl_image_format format = {CL_RGBA, CL_FLOAT};
  cl_int errorcode;
  EXPECT_FALSE(
      clCreateImage(context, 0, &format, nullptr, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_DESCRIPTOR, errorcode);
}

static cl_mem imageSizeTest(cl_context context, cl_mem_object_type type,
                            size_t width, size_t height, size_t depth,
                            size_t arraySize, cl_mem buffer, cl_int *error) {
  cl_image_format format;
  format.image_channel_data_type = CL_FLOAT;
  format.image_channel_order = CL_RGBA;
  cl_image_desc desc = {};
  desc.image_type = type;
  desc.image_width = width;
  desc.image_height = height;
  desc.image_depth = depth;
  desc.image_array_size = arraySize;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = buffer;
  return clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                       error);
}

TEST_F(clCreateImageTest, InvalidImageSize2DWidth) {
  size_t maxWidth = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH,
                                 sizeof(size_t), &maxWidth, nullptr));
  ASSERT_NE(0u, maxWidth);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE2D, maxWidth + 1, 16,
                               1, 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidImageSize2DHeight) {
  size_t maxHeight = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT,
                                 sizeof(size_t), &maxHeight, nullptr));
  ASSERT_NE(0u, maxHeight);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE2D, 16,
                               maxHeight + 1, 1, 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidImageSize3DWidth) {
  size_t maxWidth = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH,
                                 sizeof(size_t), &maxWidth, nullptr));
  ASSERT_NE(0u, maxWidth);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE3D, maxWidth + 1, 16,
                               16, 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidImageSize3DHeight) {
  size_t maxHeight = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT,
                                 sizeof(size_t), &maxHeight, nullptr));
  ASSERT_NE(0u, maxHeight);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE3D, 16,
                               maxHeight + 1, 16, 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidImageSize3DDepth) {
  size_t maxDepth = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH,
                                 sizeof(size_t), &maxDepth, nullptr));
  ASSERT_NE(0u, maxDepth);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE3D, 16, 16,
                               maxDepth + 1, 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidImageSizeBufferSize) {
  const size_t maxBufferSize = getDeviceImageMaxBufferSize();
  ASSERT_NE(0u, maxBufferSize);
  cl_int error;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, maxBufferSize + 1,
                                 nullptr, &error);
  ASSERT_NE(nullptr, buffer);
  EXPECT_SUCCESS(error);
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE1D_BUFFER,
                               maxBufferSize + 1, 0, 0, 1, buffer, &error);
  EXPECT_EQ(nullptr, image);
  EXPECT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
  ASSERT_EQ(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_F(clCreateImageTest, Invalid1DBuffer) {
  cl_image_format format = {};
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;
  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
  desc.image_width = getDeviceImageMaxBufferSize();
  desc.buffer = nullptr;
  cl_int error;
  cl_mem image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc,
                               nullptr, &error);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_DESCRIPTOR, error);
  ASSERT_EQ(nullptr, image);
}

TEST_F(clCreateImageTest, InvalidImageSizeArraySize) {
  size_t maxArraySize = 0;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE,
                                 sizeof(size_t), &maxArraySize, nullptr));
  ASSERT_NE(0u, maxArraySize);
  cl_int error;
  cl_mem image = imageSizeTest(context, CL_MEM_OBJECT_IMAGE2D_ARRAY, 16, 16, 1,
                               maxArraySize + 1, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_IMAGE_SIZE, error);
}

TEST_F(clCreateImageTest, InvalidHostPtrNull) {
  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;
  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = 16;
  desc.image_height = 16;
  desc.image_depth = 1;
  desc.image_array_size = 1;
  // TODO: Do we need to set the row and pitch?
  desc.image_row_pitch = desc.image_width * sizeof(cl_float) * 4;
  desc.image_slice_pitch = desc.image_slice_pitch * desc.image_height;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;
  cl_int error;
  cl_mem image = clCreateImage(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               &format, &desc, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, error);
  image = clCreateImage(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE,
                        &format, &desc, nullptr, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, error);
}

TEST_F(clCreateImageTest, InvalidHostPtrFlags) {
  cl_float data[16 * 16 * 4];
  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;
  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = 16;
  desc.image_height = 16;
  desc.image_depth = 1;
  desc.image_array_size = 1;
  // TODO: Do we need to set the row and pitch?
  desc.image_row_pitch = desc.image_width * sizeof(cl_float) * 4;
  desc.image_slice_pitch = desc.image_slice_pitch * desc.image_height;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;
  cl_int error;
  cl_mem image =
      clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, data, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, error);
  image =
      clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, data, &error);
  ASSERT_EQ(nullptr, image);
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, error);
}

// TODO: TEST_F(clCreateImageTest, ImageFormatNotSupported) {}
// TODO: TEST_F(clCreateImageTest, MemObjectAllocationFailture) {}
// TODO: TEST_F(clCreateImageTest, InvalidOperation) {}
// TODO: TEST_F(clCreateImageTest, OutOfResources) {}
// TODO: TEST_F(clCreateImageTest, OutOfHostMemory) {}
