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

#include <algorithm>
#include <cmath>
#include <string>

#include "Common.h"
#include "EventWaitList.h"

class cl3DImageWriteExtensionTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!(getDeviceImageSupport() && getDeviceCompilerAvailable() &&
          isDeviceExtensionSupported("cl_khr_3d_image_writes"))) {
      GTEST_SKIP();
    }

    const char *source =
        "#pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable\n"
        "__constant sampler_t sampler = \n"
        "  CLK_NORMALIZED_COORDS_FALSE  \n"
        "  | CLK_ADDRESS_CLAMP_TO_EDGE  \n"
        "  | CLK_FILTER_NEAREST;  \n"
        " \n"
        "__kernel void imagesf(__read_only image3d_t A, \n"
        "__read_only image3d_t B, __write_only image3d_t C \n"
        ") {  \n"
        "  int4 i3 = (int4)(get_global_id(0), get_global_id(1),\n"
        "  get_global_id(2), 0); \n"
        "  float4 a = read_imagef(A, sampler, i3);  \n"
        "  float4 b = read_imagef(B, sampler, i3);  \n"
        "  write_imagef(C, i3, a + b);  \n"
        "}\n"
        "__kernel void imagesi(__read_only image3d_t A, \n"
        " __read_only image3d_t B, __write_only image3d_t C \n"
        ") {  \n"
        "  int4 i3 = (int4)(get_global_id(0), get_global_id(1),\n"
        "  get_global_id(2), 0); \n"
        "  int4 a = read_imagei(A, sampler, i3);  \n"
        "  int4 b = read_imagei(B, sampler, i3);  \n"
        "  write_imagei(C, i3, a + b);  \n"
        "}\n"
        "__kernel void imagesui(__read_only image3d_t A, \n"
        "__read_only image3d_t B, __write_only image3d_t C \n"
        ") {  \n"
        "  int4 i3 = (int4)(get_global_id(0), get_global_id(1),\n"
        "  get_global_id(2), 0); \n"
        "  uint4 a = read_imageui(A, sampler, i3);  \n"
        "  uint4 b = read_imageui(B, sampler, i3);  \n"
        "  write_imageui(C, i3, a + b);  \n"
        "}\n"
        // TODO: CA-713 Re-enable when half image functions are ready
        //"#ifdef cl_khr_fp16\n"
        "#if 0\n"
        "#pragma OPENCL EXTENSION cl_khr_fp16 : enable\n"
        "__kernel void imagesh(\n__read_only image3d_t A, \n"
        "__read_only image3d_t B, __write_only image3d_t C \n"
        ") {  \n"
        "  int4 i3 = (int4)(get_global_id(0), get_global_id(1),\n"
        "  get_global_id(2), 0); \n"
        "  uint4 a = read_imageh(A, sampler, i3);  \n"
        "  uint4 b = read_imageh(B, sampler, i3);  \n"
        "  write_imageh(C, i3, a + b);  \n"
        "}\n"
        "#endif\n";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (imgC) {
      EXPECT_SUCCESS(clReleaseMemObject(imgC));
    }
    if (imgB) {
      EXPECT_SUCCESS(clReleaseMemObject(imgB));
    }
    if (imgA) {
      EXPECT_SUCCESS(clReleaseMemObject(imgA));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;

  cl_mem imgA = nullptr;
  cl_mem imgB = nullptr;
  cl_mem imgC = nullptr;

  cl_channel_type IMAGE_CHANNEL_TYPE = CL_FLOAT;
  cl_channel_order IMAGE_CHANNEL_ORDER = CL_RGBA;
  int IMAGE_CHANNEL_SIZE = 4;
  const int IMAGE_NUM_CHANNELS = 4;
  const int IMAGE_NUM_ELEMENTS = 4;
  const int IMAGE_ELEMENT_SIZE = IMAGE_CHANNEL_SIZE * IMAGE_NUM_CHANNELS;
  const int IMAGE_DATA_SIZE = IMAGE_NUM_CHANNELS * IMAGE_NUM_ELEMENTS;

  template <typename T>
  void test_body(const char *kernel_name) {
    using data_t = T;
    auto prepare = [](int N, std::vector<data_t> &A, std::vector<data_t> &B) {
      auto C = std::vector<data_t>(N);
      for (int i = 0; i < N; ++i) {
        A[i] = (data_t)1;
        B[i] = (data_t)2;
      }
      return C;
    };

    auto A = std::vector<data_t>(IMAGE_DATA_SIZE);
    auto B = std::vector<data_t>(IMAGE_DATA_SIZE);
    auto C = prepare(IMAGE_DATA_SIZE, A, B);

    cl_int errorcode;

    const cl_image_format format = {IMAGE_CHANNEL_ORDER, IMAGE_CHANNEL_TYPE};
    cl_image_desc descriptor;
    descriptor.image_height = 0;
    descriptor.image_depth = 0;
    descriptor.image_row_pitch = 0;
    descriptor.image_slice_pitch = 0;
    descriptor.image_array_size = 1;
    descriptor.num_mip_levels = 0;
    descriptor.num_samples = 0;
    descriptor.buffer = nullptr;

    descriptor.image_type = CL_MEM_OBJECT_IMAGE3D;
    descriptor.image_width = IMAGE_NUM_ELEMENTS / 4;
    descriptor.image_height = IMAGE_NUM_ELEMENTS / descriptor.image_width;
    descriptor.image_depth =
        IMAGE_NUM_ELEMENTS / descriptor.image_width / descriptor.image_height;

    imgA = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         &format, &descriptor, (void *)(A.data()), &errorcode);
    ASSERT_SUCCESS(errorcode);
    imgB = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         &format, &descriptor, (void *)(B.data()), &errorcode);
    ASSERT_SUCCESS(errorcode);
    imgC = clCreateImage(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                         &format, &descriptor, (void *)(C.data()), &errorcode);
    ASSERT_SUCCESS(errorcode);

    cl_kernel kernel = clCreateKernel(program, kernel_name, &errorcode);
    ASSERT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);
    errorcode = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&imgA);
    EXPECT_SUCCESS(errorcode);
    errorcode = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&imgB);
    EXPECT_SUCCESS(errorcode);
    errorcode = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&imgC);
    EXPECT_SUCCESS(errorcode);
    size_t globalWorkSize[3] = {
        descriptor.image_width,
        std::max((size_t)1, descriptor.image_height),
        std::max((size_t)1, descriptor.image_depth),
    };

    errorcode =
        clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                               globalWorkSize, nullptr, 0, nullptr, nullptr);
    EXPECT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clReleaseKernel(kernel));

    errorcode = clFinish(command_queue);
    ASSERT_SUCCESS(errorcode);

    size_t origin[3] = {0, 0, 0};
    errorcode =
        clEnqueueReadImage(command_queue, imgC, CL_TRUE, origin, globalWorkSize,
                           0, 0, C.data(), 0, nullptr, nullptr);
    ASSERT_SUCCESS(errorcode);

    for (int i = 0; i < IMAGE_DATA_SIZE; ++i) {
      const data_t expected_value = A[i] + B[i];
      ASSERT_EQ(expected_value, C[i])
          << "Index " << i << " is " << C[i] << " when " << expected_value
          << " was expected!";
    }
  }
};

TEST_F(cl3DImageWriteExtensionTest, Float) {
  IMAGE_CHANNEL_TYPE = CL_FLOAT;
  IMAGE_CHANNEL_SIZE = 4;
  test_body<float>("imagesf");
}

TEST_F(cl3DImageWriteExtensionTest, Int32) {
  IMAGE_CHANNEL_TYPE = CL_SIGNED_INT32;
  IMAGE_CHANNEL_SIZE = 4;
  test_body<int>("imagesi");
}

TEST_F(cl3DImageWriteExtensionTest, UnsignedInt32) {
  IMAGE_CHANNEL_TYPE = CL_UNSIGNED_INT32;
  IMAGE_CHANNEL_SIZE = 4;
  test_body<unsigned int>("imagesui");
}

TEST_F(cl3DImageWriteExtensionTest, DISABLED_Half) {
  IMAGE_CHANNEL_TYPE = CL_UNSIGNED_INT32;
  IMAGE_CHANNEL_SIZE = 4;
  test_body<unsigned int>("imagesh");
}
