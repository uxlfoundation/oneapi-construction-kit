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

class clEnqueueMapImageTestBase : public ucl::CommandQueueTest {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    createImage();
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  virtual void createImage() = 0;

  // Used for 1D Array, 2DArray and 3D negative testing to avoid duplication
  void InvalidSlicePitchTestBody() {
    size_t image_row_pitch = 0;
    cl_int error;
    void *ptr = clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ,
                                  origin, region, &image_row_pitch, nullptr, 0,
                                  nullptr, nullptr, &error);
    ASSERT_EQ(nullptr, ptr);
    ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  }

  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  cl_mem image = nullptr;
  size_t origin[3] = {};
  size_t region[3] = {};
};

struct clEnqueueMapImageTestsParams {
  cl_mem_object_type object_type;
  bool is_aligned;
  bool is_pitched;
};

static std::ostream &operator<<(std::ostream &out,
                                const clEnqueueMapImageTestsParams &params) {
  out << "clEnqueueMapImageTestsParams{.object_type{";
  switch (params.object_type) {
#define CASE(TYPE) \
  case TYPE:       \
    out << #TYPE;  \
    break;
    CASE(CL_MEM_OBJECT_BUFFER)
    CASE(CL_MEM_OBJECT_IMAGE2D)
    CASE(CL_MEM_OBJECT_IMAGE3D)
#ifdef CL_VERSION_1_2
    CASE(CL_MEM_OBJECT_IMAGE2D_ARRAY)
    CASE(CL_MEM_OBJECT_IMAGE1D)
    CASE(CL_MEM_OBJECT_IMAGE1D_ARRAY)
    CASE(CL_MEM_OBJECT_IMAGE1D_BUFFER)
#endif
#ifdef CL_VERSION_2_0
    CASE(CL_MEM_OBJECT_PIPE)
#endif
#undef CASE
    default:
      out << params.object_type;
      break;
  }
  out << "}, .is_aligned{" << (params.is_aligned ? "true" : "false")
      << "}, .is_pitched{" << (params.is_pitched ? "true" : "false") << "}}";
  return out;
}

struct clEnqueueMapImageTests
    : ucl::CommandQueueTest,
      testing::WithParamInterface<clEnqueueMapImageTestsParams> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!(getDeviceImageSupport() && getDeviceCompilerAvailable())) {
      GTEST_SKIP();
    }
    object_type = GetParam().object_type;
    is_aligned = GetParam().is_aligned;
    is_pitched = GetParam().is_pitched;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE}, object_type,
                                     {CL_RGBA, CL_FLOAT})) {
      GTEST_SKIP();
    }

    const char *source = R"(
      kernel void img_copy1d(read_only image1d_t src_image,
                     write_only image1d_t dst_image) {
        int coord;
        coord = get_global_id(0);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      kernel void img_copy1d_array(read_only image1d_array_t src_image,
                     write_only image1d_array_t dst_image) {
        int2 coord;
        coord.x = get_global_id(0);
        coord.y = get_global_id(1);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      kernel void img_copy1d_buffer(read_only image1d_buffer_t src_image,
                     write_only image1d_buffer_t dst_image) {
        int coord;
        coord = get_global_id(0);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      kernel void img_copy2d(read_only image2d_t src_image,
                             write_only image2d_t dst_image) {
        int2 coord;
        coord.x = get_global_id(0);
        coord.y = get_global_id(1);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      kernel void img_copy2d_array(read_only image2d_array_t src_image,
                             write_only image2d_array_t dst_image) {
        int4 coord = (int4) (get_global_id(0), get_global_id(1), get_global_id(2), 0);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      kernel void img_copy3d(read_only image3d_t src_image,
                             write_only image3d_t dst_image) {
        int4 coord = (int4) (get_global_id(0), get_global_id(1), get_global_id(2), 0);
        float4 color = read_imagef(src_image, coord);
        write_imagef(dst_image, coord, color);
      }
      )";
    const size_t length = strlen(source);
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &source, &length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, program);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, "", nullptr, nullptr));
    cl_build_status buildStatus = CL_BUILD_NONE;
    ASSERT_SUCCESS(
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                              sizeof(cl_build_status), &buildStatus, nullptr));

    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_FLOAT;

    image_desc.image_type = object_type;
    image_desc.image_width = 1;
    image_desc.image_height = 1;
    image_desc.image_depth = 1;
    image_desc.image_array_size = 1;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = image_desc.image_height;
    region[2] = image_desc.image_depth;
    expected_row_pitch = 0;
    expected_slice_pitch = 0;

    switch (object_type) {
      case CL_MEM_OBJECT_IMAGE1D:
        region[0] = image_desc.image_width = 16;
        numPixels = image_desc.image_width;
        kernel = clCreateKernel(program, "img_copy1d", &error);
        expected_row_pitch = sizeof(cl_float4) * image_desc.image_width;
        break;
      case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        region[0] = image_desc.image_width = 16;
        region[1] = image_desc.image_array_size = 8;
        expected_row_pitch = sizeof(cl_float4) * image_desc.image_width;
        expected_slice_pitch = expected_row_pitch;
        if (is_pitched) {
          image_desc.image_slice_pitch =
              sizeof(cl_float4) * (image_desc.image_width + 1);
          expected_slice_pitch = image_desc.image_slice_pitch;
        }
        numPixels = expected_slice_pitch * image_desc.image_array_size /
                    sizeof(cl_float4);
        kernel = clCreateKernel(program, "img_copy1d_array", &error);
        break;
      case CL_MEM_OBJECT_IMAGE1D_BUFFER:
        region[0] = image_desc.image_width = 16;
        image_desc.buffer = clCreateBuffer(
            context, CL_MEM_READ_ONLY,
            image_desc.image_width * sizeof(cl_float4), nullptr, &error);
        ASSERT_SUCCESS(error);
        numPixels = image_desc.image_width;
        kernel = clCreateKernel(program, "img_copy1d_buffer", &error);
        break;
      case CL_MEM_OBJECT_IMAGE2D:
        region[0] = image_desc.image_width = 16;
        region[1] = image_desc.image_height = 16;
        kernel = clCreateKernel(program, "img_copy2d", &error);
        expected_row_pitch = sizeof(cl_float4) * (image_desc.image_width);
        if (is_pitched) {
          image_desc.image_row_pitch =
              sizeof(cl_float4) * (image_desc.image_width + 1);
          expected_row_pitch = image_desc.image_row_pitch;
        }
        numPixels =
            expected_row_pitch * image_desc.image_height / sizeof(cl_float4);
        break;
      case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        region[0] = image_desc.image_width = 12;
        region[1] = image_desc.image_height = 12;
        region[2] = image_desc.image_array_size = 4;
        expected_row_pitch = sizeof(cl_float4) * (image_desc.image_width);
        expected_slice_pitch = expected_row_pitch * image_desc.image_height;
        if (is_pitched) {
          image_desc.image_row_pitch = sizeof(cl_float4) * 15;
          image_desc.image_slice_pitch =
              image_desc.image_row_pitch * (image_desc.image_height + 1);
          expected_row_pitch = image_desc.image_row_pitch;
          expected_slice_pitch = image_desc.image_slice_pitch;
        }
        numPixels = expected_slice_pitch * image_desc.image_array_size /
                    sizeof(cl_float4);
        kernel = clCreateKernel(program, "img_copy2d_array", &error);
        break;
      case CL_MEM_OBJECT_IMAGE3D:
        region[0] = image_desc.image_width = 8;
        region[1] = image_desc.image_height = 8;
        region[2] = image_desc.image_depth = 8;
        expected_row_pitch = sizeof(cl_float4) * (image_desc.image_width);
        expected_slice_pitch = expected_row_pitch * image_desc.image_height;
        if (is_pitched) {
          image_desc.image_row_pitch =
              sizeof(cl_float4) * (image_desc.image_width + 1);
          image_desc.image_slice_pitch =
              image_desc.image_row_pitch * (image_desc.image_height + 1);
          expected_row_pitch = image_desc.image_row_pitch;
          expected_slice_pitch = image_desc.image_slice_pitch;
        }
        numPixels =
            expected_slice_pitch * image_desc.image_depth / sizeof(cl_float4);
        kernel = clCreateKernel(program, "img_copy3d", &error);
        break;
      default:
        UCL_ABORT("unknown object type %d", (int)object_type);
    }
  }

  void TearDown() override {
    if (image_desc.buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(image_desc.buffer));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  // Used for 1D Array, 2DArray and 3D negative testing to avoid duplication
  void InvalidSlicePitchTestBody() {
    size_t image_row_pitch = 0;

    cl_int error;
    void *ptr = clEnqueueMapImage(command_queue, src_image, CL_TRUE,
                                  CL_MAP_READ, origin, region, &image_row_pitch,
                                  nullptr, 0, nullptr, nullptr, &error);
    ASSERT_EQ(nullptr, ptr);
    ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  cl_mem src_image = nullptr;
  cl_mem dst_image = nullptr;
  cl_mem out_buffer = nullptr;
  size_t origin[3] = {};
  size_t region[3] = {};
  size_t numPixels = 0;

  cl_mem_object_type object_type = {};

  bool is_aligned = false;
  bool is_pitched = false;

  size_t expected_row_pitch = 0;
  size_t expected_slice_pitch = 0;
};

TEST_P(clEnqueueMapImageTests, MapImage) {
  cl_int error;

  // We choose the default alignment for this to be 4K. This is rather
  // arbitrary, but is used to try and force it down different paths wrt the
  // host_ptr
  const size_t good_alignment = 4096;
  UCL::vector<float, good_alignment> srcPixels((numPixels * 4) + 1);
  UCL::vector<float, good_alignment> dstPixels((numPixels * 4) + 1);

  float *src_pixels = srcPixels.data();
  float *dst_pixels = dstPixels.data();
  ASSERT_NE(nullptr, src_pixels);
  ASSERT_NE(nullptr, dst_pixels);

  // Move the pointer on by a single float. This will make it less well
  // aligned and potentially force down a different path
  if (!is_aligned) {
    src_pixels++;
    dst_pixels++;
  }

  for (unsigned int pixel = 0; pixel < numPixels; pixel++) {
    for (size_t element = 0; element < 4; element++) {
      // clang-tidy fails to understand the ASSERT_NE above and so thinks
      // that src_pixels may be null here, NOLINT to suppress that.
      src_pixels[pixel * 4 + element] = (float)element;  // NOLINT
    }
  }

  src_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                            &image_format, &image_desc, src_pixels, &error);
  ASSERT_SUCCESS(error);
  dst_image = clCreateImage(context, CL_MEM_WRITE_ONLY, &image_format,
                            &image_desc, nullptr, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, src_image);
  ASSERT_NE(nullptr, dst_image);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(src_image),
                                static_cast<void *>(&src_image)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(dst_image),
                                static_cast<void *>(&dst_image)));

  size_t image_row_pitch = 0;
  size_t image_slice_pitch = 0;

  void *ptr = clEnqueueMapImage(
      command_queue, src_image, CL_TRUE, CL_MAP_READ, origin, region,
      &image_row_pitch, &image_slice_pitch, 0, nullptr, nullptr, &error);
  EXPECT_NE(nullptr, ptr);
  ASSERT_SUCCESS(error) << region[0] << ", " << region[1] << ", " << region[2];
  ASSERT_EQ(image_row_pitch, expected_row_pitch);
  ASSERT_EQ(image_slice_pitch, expected_slice_pitch);

  size_t localWorkSize[3] = {1, 1, 1};
  cl_event workEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, origin,
                                        region, localWorkSize, 0, nullptr,
                                        &workEvent));
  ASSERT_NE(nullptr, workEvent);
  cl_event readEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_FALSE, origin,
                                    region, 0, 0, dst_pixels, 1, &workEvent,
                                    &readEvent));
  ASSERT_NE(nullptr, readEvent);
  ASSERT_SUCCESS(clFinish(command_queue));
  const size_t num_slices = image_desc.image_type == CL_MEM_OBJECT_IMAGE3D
                                ? image_desc.image_depth
                                : image_desc.image_array_size;

  const size_t row_pitch_in_pixels = expected_row_pitch / sizeof(cl_float4);
  const size_t slice_pitch_in_pixels = expected_slice_pitch / sizeof(cl_float4);

  for (size_t slice = 0; slice < num_slices; slice++) {
    for (size_t row = 0; row < image_desc.image_height; row++) {
      for (size_t col = 0; col < image_desc.image_width; col++) {
        const size_t src_pixel_index =
            (slice * slice_pitch_in_pixels) + (row * row_pitch_in_pixels) + col;
        const size_t dst_pixel_index =
            (slice * (image_desc.image_width * image_desc.image_height)) +
            (row * image_desc.image_height) + col;
        for (size_t element = 0; element < 4; element++) {
          ASSERT_EQ(src_pixels[(src_pixel_index * 4) + element],
                    dst_pixels[(dst_pixel_index * 4) + element])
              << "At pixel : " << dst_pixel_index << '\n'
              << src_pixels[(src_pixel_index * 4) + element] << " vs "
              << dst_pixels[(dst_pixel_index * 4) + element] << '\n'
              << "Total : " << numPixels;
        }
      }
    }
  }

  clReleaseEvent(workEvent);
  clReleaseEvent(readEvent);

  if (src_image) {
    EXPECT_SUCCESS(clReleaseMemObject(src_image));
  }
  if (src_image) {
    EXPECT_SUCCESS(clReleaseMemObject(dst_image));
  }
}

INSTANTIATE_TEST_CASE_P(
    MemObjTypeTest, clEnqueueMapImageTests,
    ::testing::Values(
        clEnqueueMapImageTestsParams{cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D},
                                     false, true},
        clEnqueueMapImageTestsParams{
            cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D_ARRAY}, false, true},
        clEnqueueMapImageTestsParams{cl_mem_object_type{CL_MEM_OBJECT_IMAGE3D},
                                     false, true},
        clEnqueueMapImageTestsParams{cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D},
                                     true, false},
        clEnqueueMapImageTestsParams{
            cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D_ARRAY}, true, false},
        // TODO: Test this for CL_MEM_OBJECT_IMAGE1D_BUFFER
        // cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D_BUFFER},
        clEnqueueMapImageTestsParams{cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D},
                                     true, false},
        clEnqueueMapImageTestsParams{
            cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D_ARRAY}, true, false},
        clEnqueueMapImageTestsParams{cl_mem_object_type{CL_MEM_OBJECT_IMAGE3D},
                                     true, false}));

// ########################## NEGATIVE TESTING ################################

class clEnqueueMapImageNegativeTest1d : public clEnqueueMapImageTestBase {
 public:
  void createImage() final {
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    image_desc.image_type = CL_MEM_OBJECT_IMAGE1D;
    image_desc.image_width = 1;
    image_desc.image_height = 0;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 0;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = 1;
    region[2] = 1;
  }
};

class clEnqueueMapImageNegativeTest1dBuffer : public clEnqueueMapImageTestBase {
 public:
  void TearDown() {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    clEnqueueMapImageTestBase::TearDown();
  }

  void createImage() final {
    cl_int error;
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uchar4),
                            nullptr, &error);
    ASSERT_SUCCESS(error);

    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    image_desc.image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
    image_desc.image_width = 1;
    image_desc.image_height = 0;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 0;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = buffer;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = 1;
    region[2] = 1;
  }

  cl_mem buffer = nullptr;
};

class clEnqueueMapImageNegativeTest1dArray : public clEnqueueMapImageTestBase {
 public:
  void createImage() final {
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    image_desc.image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
    image_desc.image_width = 1;
    image_desc.image_height = 0;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 1;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = 1;
    region[2] = 1;
  }
};

class clEnqueueMapImageNegativeTest2d : public clEnqueueMapImageTestBase {
 public:
  void createImage() final {
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

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

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = image_desc.image_height;
    region[2] = 1;
  }
};

class clEnqueueMapImageNegativeTest2dArray : public clEnqueueMapImageTestBase {
 public:
  void createImage() final {
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    image_desc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
    image_desc.image_width = 1;
    image_desc.image_height = 1;
    image_desc.image_depth = 0;
    image_desc.image_array_size = 0;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = image_desc.image_height;
    region[2] = 1;
  }
};

class clEnqueueMapImageNegativeTest3d : public clEnqueueMapImageTestBase {
 public:
  void createImage() final {
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;

    image_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
    image_desc.image_width = 2;
    image_desc.image_height = 2;
    image_desc.image_depth = 2;
    image_desc.image_array_size = 0;
    image_desc.image_row_pitch = 0;
    image_desc.image_slice_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;

    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int error;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &error);
    ASSERT_SUCCESS(error);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = image_desc.image_width;
    region[1] = image_desc.image_height;
    region[2] = image_desc.image_depth;
  }
};

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidCommandQueue) {
  cl_int error;
  size_t image_row_pitch;
  void *ptr =
      clEnqueueMapImage(nullptr, image, CL_TRUE, CL_MAP_READ, origin, region,
                        &image_row_pitch, nullptr, 0, nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, error);
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidContext) {
  cl_int error;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_command_queue other_queue =
      clCreateCommandQueue(other_context, device, 0, &error);
  EXPECT_SUCCESS(error);

  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(other_queue, image, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  EXPECT_EQ(nullptr, ptr);
  EXPECT_EQ_ERRCODE(CL_INVALID_CONTEXT, error);

  EXPECT_SUCCESS(clReleaseCommandQueue(other_queue));
  ASSERT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidMemObject) {
  cl_int error;
  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(command_queue, nullptr, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, error);
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidValueOutOfBounds) {
  cl_int error;
  origin[0] = 4;
  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidValueOrigin) {
  cl_int error;
  origin[0] = 2;
  origin[1] = 2;
  origin[2] = 0;
  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidValueRegion) {
  cl_int error;
  region[0] = 0;
  region[1] = 0;
  region[2] = 0;
  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
}

TEST_F(clEnqueueMapImageNegativeTest2d, InvalidValueImageRowPitchNull) {
  cl_int error;
  void *ptr =
      clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ, origin,
                        region, nullptr, nullptr, 0, nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
}

// Redmine #5116: Check CL_INVALID_IMAGE_SIZE
// Redmine #5116: Check CL_INVALID_IMAGE_FORMAT_DESCRIPTOR

using clEnqueueMapNoImages = ucl::CommandQueueTest;

TEST_F(clEnqueueMapNoImages, InvalidOperationNoImageSupport) {
  if (getDeviceImageSupport()) {
    GTEST_SKIP();
  }
  cl_mem image = nullptr;
  size_t origin[3] = {2, 2, 0};
  size_t region[3] = {8, 8, 1};
  cl_int error = CL_SUCCESS;
  size_t image_row_pitch;
  void *ptr = clEnqueueMapImage(command_queue, image, CL_TRUE, CL_MAP_READ,
                                origin, region, &image_row_pitch, nullptr, 0,
                                nullptr, nullptr, &error);
  ASSERT_EQ(nullptr, ptr);

  // OpenCL should either complain and say it can't support images at all, or
  // complain that the memory object that was created was dud (because we
  // could not create a image anyway because our device doesn't support it)
  ASSERT_TRUE(CL_INVALID_OPERATION == error || CL_INVALID_MEM_OBJECT == error);
}

// Redmine #5125: Check CL_INVALID_OPERATION
// Redmine #5117: CL_OUT_OF_RESOURCES
// Redmine #5114: Check CL_OUT_OF_HOST_MEMORY

class clEnqueueMapImageTest : public clEnqueueMapImageNegativeTest2d {};

// https://cvs.khronos.org/bugzilla/show_bug.cgi?id=7390
TEST_F(clEnqueueMapImageTest, ZeroMapFlagsImplicitReadWrite) {
  cl_int error;
  size_t image_row_pitch;
  void *ptr =
      clEnqueueMapImage(command_queue, image, CL_TRUE, 0, origin, region,
                        &image_row_pitch, nullptr, 0, nullptr, nullptr, &error);
  ASSERT_NE(nullptr, ptr);
  ASSERT_SUCCESS(error);
  cl_event unmap_event;
  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, image, ptr, 0, nullptr,
                                         &unmap_event));
  ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
  ASSERT_SUCCESS(clReleaseEvent(unmap_event));
}

// CL_INVALID_VALUE if image is a 3D image, 1D or 2D image array object and
// image_slice_pitch is NULL.
TEST_F(clEnqueueMapImageNegativeTest1dArray, InvalidSlicePitch) {
  InvalidSlicePitchTestBody();
}

TEST_F(clEnqueueMapImageNegativeTest2dArray, InvalidSlicePitch) {
  InvalidSlicePitchTestBody();
}

TEST_F(clEnqueueMapImageNegativeTest3d, InvalidSlicePitch) {
  InvalidSlicePitchTestBody();
}
