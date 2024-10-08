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

#include <random>

#include "Common.h"
#include "EventWaitList.h"

// Image values
const size_t image_width = 4;
const size_t image_height = 4;
const size_t image_channel_count = 4;
const cl_image_format image_format = {CL_RGBA, CL_SNORM_INT8};
const cl_image_desc image_desc = []() {
  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  image_desc.image_width = image_width;
  image_desc.image_height = image_height;
  image_desc.image_depth = 1;
  image_desc.image_array_size = 1;
  image_desc.image_row_pitch = 0;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;
  return image_desc;
}();
const size_t origin[3] = {0, 0, 0};
const size_t region[3] = {image_width, image_height, 1};
const cl_uchar image_data[image_width * image_height * image_channel_count] = {
    255, 0,   0,
    255,  // Red
    0,   255, 0,
    255,  // Green
    0,   0,   255,
    255,  // Blue
    255, 255, 0,
    255  // Yellow
};

class clEnqueueWriteImageTest : public ucl::CommandQueueTest,
                                TestWithEventWaitList {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     image_desc.image_type, image_format)) {
      GTEST_SKIP();
    }
    cl_int status;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &status);
    ASSERT_SUCCESS(status);
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    ASSERT_EQ_ERRCODE(
        err, clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, region,
                                 0, 0, image_data, num_events, events, event));
  }

  cl_mem image = nullptr;
};

TEST_F(clEnqueueWriteImageTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueWriteImage(nullptr, image, CL_TRUE, origin, region,
                                        0, 0, image_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteImageTest, InvalidContext) {
  cl_int error;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_command_queue other_queue =
      clCreateCommandQueue(other_context, device, 0, &error);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueWriteImage(other_queue, image, CL_TRUE, origin, region, 0, 0,
                          image_data, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseCommandQueue(other_queue));
  ASSERT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(clEnqueueWriteImageTest, InvalidMemObject) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueWriteImage(command_queue, nullptr, CL_TRUE, origin, region, 0, 0,
                          image_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteImageTest, InvalidValueNullPtr) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, region, 0, 0,
                          nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteImageTest, InvalidValueOrigin) {
  size_t bad_origin[3] = {image_width + 1, image_height + 1, 2};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteImage(command_queue, image, CL_TRUE, bad_origin, region, 0,
                          0, image_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteImageTest, InvalidValueRegion) {
  size_t bad_region[3] = {image_width + 1, image_height + 1, 2};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, bad_region, 0,
                          0, image_data, 0, nullptr, nullptr));
}

TEST_F(clEnqueueWriteImageTest, InvalidSrcOriginRegionRules) {
  // As this is a 2D image this origin should follow the incorrect rules
  size_t origin[3] = {0, 0, 1};
  size_t region[3] = {image_width, image_height, 1};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, region, 0, 0,
                          image_data, 0, nullptr, nullptr));
}

// These are problematic to test with a single device as such they have been
// left omitted for now
// Redmine #5116: Check CL_INVALID_IMAGE_SIZE if image dimensions (image width,
// height, specified or compute row and/or slice pitch) for image are not
// supported by device associated with queue. image creation is not possible so
// an image can't be enqueued.

// Redmine #5116: Check CL_INVALID_IMAGE_FORMAT if image format (image channel
// order and data type) for image are not supported by device associated with
// queue.  This is problematic to test with a single device, image creation is
// not possible so an image can't be enqueued.

// Redmine #5123: CL_MEM_OBJECT_ALLOCATION_FAILURE if there is a failure to
// allocate memory for data store associated with image.

// Redmine #5125: Check CL_INVALID_OPERATION if the device associated with
// command_queue does not support images (i.e. CL_DEVICE_IMAGE_SUPPORT
// specified the table of allowed values for param_name for clGetDeviceInfo is
// CL_FALSE).

// Redmine #5125: Check CL_INVALID_OPERATION if clEnqueueWriteImage is called
// on image which has been created with CL_MEM_HOST_WRITE_ONLY or
// CL_MEM_HOST_NO_ACCESS.
TEST_F(clEnqueueWriteImageTest, InvalidOperationHostMem) {
  cl_int status;
  cl_mem host_image =
      clCreateImage(context, CL_MEM_HOST_READ_ONLY, &image_format, &image_desc,
                    nullptr, &status);
  EXPECT_SUCCESS(status);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueWriteImage(command_queue, host_image, CL_TRUE, origin, region, 0,
                          0, image_data, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseMemObject(host_image));
  host_image = clCreateImage(context, CL_MEM_HOST_NO_ACCESS, &image_format,
                             &image_desc, nullptr, &status);
  EXPECT_SUCCESS(status);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueWriteImage(command_queue, host_image, CL_TRUE, origin, region, 0,
                          0, image_data, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseMemObject(host_image));
}

// Redmine #5117: Check CL_OUT_OF_RESOURCES if there is a failure to allocate
// resources required by the OpenCL implementation on the device.

// Redmine #5114: CL_OUT_OF_HOST_MEMORY if there is a failure to allocate
// resources required by the OpenCL implementation on the host.

// Redmine #5125: This should actually be an instanstiated parameterised
// test fixture
template <cl_mem_object_type image_type, size_t Width, size_t Height,
          size_t Depth, size_t ArraySize>
class clEnqueueWriteImageTestBase : public ucl::CommandQueueTest {
 public:
  enum {
    width = Width,
    height = Height,
    depth = Depth,
    array_size = ArraySize,
    num_pixels = width * height * depth * array_size
  };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    image_format.image_channel_order = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;
    image_desc.image_type = image_type;
    image_desc.image_width = width;
    image_desc.image_height = height;
    image_desc.image_depth = depth;
    image_desc.image_array_size = array_size;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
      cl_int status;
      buffer =
          clCreateBuffer(context, CL_MEM_READ_WRITE,
                         image_desc.image_width * num_pixels, nullptr, &status);
      image_desc.buffer = buffer;
      ASSERT_SUCCESS(status);
    } else {
      image_desc.buffer = nullptr;
    }
    cl_int status;
    image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                          &image_desc, nullptr, &status);
    ASSERT_SUCCESS(status);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  void TestDefaultBody() {
    size_t region_y = height;
    size_t region_z = depth;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY) {
      region_y = array_size;
      region_z = 1;
    } else if (image_desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
      region_z = array_size;
    }
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, region_y, region_z};
    cl_uchar4 image_data[num_pixels];
    memset(image_data, 42, sizeof(cl_uchar4) * num_pixels);
    EXPECT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_TRUE, origin,
                                       region, 0, 0, image_data, 0, nullptr,
                                       nullptr));
  }

  void TestVerifyBody() {
    size_t region_y = height;
    size_t region_z = depth;
    if (image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY) {
      region_y = array_size;
      region_z = 1;
    } else if (image_desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
      region_z = array_size;
    }
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, region_y, region_z};
    cl_uchar4 input_data[num_pixels];
    for (size_t x = 0; x < width; ++x) {
      for (size_t y = 0; y < region_y; ++y) {
        for (size_t z = 0; z < region_z; ++z) {
          const size_t i = x + (width * y) + (width * region_y * z);
          // Appease compiler warnings about subscript above array bounds.
          assert(i < num_pixels);
          if (i < num_pixels) {
            input_data[i].s[0] = static_cast<cl_uchar>(x);
            input_data[i].s[1] = static_cast<cl_uchar>(y);
            input_data[i].s[2] = static_cast<cl_uchar>(z);
            input_data[i].s[3] = 42;
          }
        }
      }
    }
    ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_TRUE, origin,
                                       region, 0, 0, input_data, 0, nullptr,
                                       nullptr));
    cl_uchar4 output_data[num_pixels];
    memset(output_data, 0, sizeof(cl_uchar4) * num_pixels);
    ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                      region, 0, 0, output_data, 0, nullptr,
                                      nullptr));
    for (size_t pixel = 0; pixel < num_pixels; ++pixel) {
      for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(input_data[pixel].s[i], output_data[pixel].s[i]);
      }
    }
  }

  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  cl_mem buffer = nullptr;
  cl_mem image = nullptr;
};

class clEnqueueWriteImage1DTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE1D, 4, 1, 1, 1> {};
class clEnqueueWriteImage1DArrayTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE1D_ARRAY, 4, 1, 1,
                                         4> {};
class clEnqueueWriteImage1DBufferTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE1D_BUFFER, 4, 1, 1,
                                         1> {};
class clEnqueueWriteImage2DTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE2D, 4, 4, 1, 1> {};
class clEnqueueWriteImage2DArrayTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE2D_ARRAY, 4, 4, 1,
                                         4> {};
class clEnqueueWriteImage3DTest
    : public clEnqueueWriteImageTestBase<CL_MEM_OBJECT_IMAGE3D, 4, 4, 4, 1> {};

TEST_F(clEnqueueWriteImage1DTest, Default) { TestDefaultBody(); }
TEST_F(clEnqueueWriteImage1DArrayTest, Default) { TestDefaultBody(); }
TEST_F(clEnqueueWriteImage1DBufferTest, Default) { TestDefaultBody(); }

TEST_F(clEnqueueWriteImage2DTest, Default) { TestDefaultBody(); }
TEST_F(clEnqueueWriteImage2DArrayTest, Default) { TestDefaultBody(); }

TEST_F(clEnqueueWriteImage3DTest, Default) { TestDefaultBody(); }

TEST_F(clEnqueueWriteImage1DTest, DefaultVerifyWholeImage) { TestVerifyBody(); }
TEST_F(clEnqueueWriteImage1DArrayTest, DefaultVerifyWholeImage) {
  TestVerifyBody();
}
TEST_F(clEnqueueWriteImage1DBufferTest, DefaultVerifyWholeImage) {
  TestVerifyBody();
}

TEST_F(clEnqueueWriteImage2DTest, DefaultVerifyWholeImage) { TestVerifyBody(); }
TEST_F(clEnqueueWriteImage2DArrayTest, DefaultVerifyWholeImage) {
  TestVerifyBody();
}

TEST_F(clEnqueueWriteImage3DTest, DefaultVerifyWholeImage) { TestVerifyBody(); }

struct clEnqueueWriteImageVerify
    : ucl::CommandQueueTest,
      testing::WithParamInterface<cl_image_format> {
  clEnqueueWriteImageVerify() : format(GetParam()) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    // This image is created in the test body to allow more informative test
    // name to be given but to also avoid the difficulties of creating a
    // correctly parameterised test with all combinations for all image types
    // + image objs. As such the image can be released in TearDown for all the
    // tests without creating it in SetUp.
  }

  void TearDown() override {
    if (image) {
      EXPECT_SUCCESS(clReleaseMemObject(image));
    }
    CommandQueueTest::TearDown();
  }

  const cl_image_format &format;
  cl_mem image = nullptr;
};

static void generate_write_data(UCL::vector<uint8_t> &data) {
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

TEST_P(clEnqueueWriteImageVerify, Image1D) {
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE1D;
  desc.image_width = 16;
  desc.image_height = 1;
  desc.image_depth = 1;
  desc.image_array_size = 1;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  const size_t pixel_size = UCL::getPixelSize(format);
  const size_t pixel_count = pixel_size * desc.image_width;
  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, 1, 1};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
}
TEST_P(clEnqueueWriteImageVerify, Image1DArray) {
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
  desc.image_width = 16;
  desc.image_height = 1;
  desc.image_depth = 1;
  desc.image_array_size = 8;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  const size_t pixel_size = UCL::getPixelSize(format);
  const size_t pixel_count =
      pixel_size * desc.image_width * desc.image_array_size;
  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_array_size, 1};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
}
TEST_P(clEnqueueWriteImageVerify, Image1DBuffer) {
  const size_t pixel_size = UCL::getPixelSize(format);

  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
  desc.image_width = 16;
  desc.image_height = 1;
  desc.image_depth = 1;
  desc.image_array_size = 1;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;

  const size_t pixel_count = pixel_size * desc.image_width;
  cl_int error;
  desc.buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, pixel_count, nullptr, &error);
  ASSERT_SUCCESS(error);

  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, 1, 1};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
  EXPECT_SUCCESS(clReleaseMemObject(desc.buffer));
}

TEST_P(clEnqueueWriteImageVerify, Image2D) {
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = 8;
  desc.image_height = 8;
  desc.image_depth = 1;
  desc.image_array_size = 1;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  const size_t pixel_size = UCL::getPixelSize(format);
  const size_t pixel_count = pixel_size * desc.image_width * desc.image_height;
  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, 1};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
}

TEST_P(clEnqueueWriteImageVerify, Image2DArray) {
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc.image_width = 3;
  desc.image_height = 3;
  desc.image_depth = 1;
  desc.image_array_size = 3;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  const size_t pixel_size = UCL::getPixelSize(format);
  const size_t pixel_count =
      pixel_size * desc.image_width * desc.image_height * desc.image_array_size;
  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height,
                      desc.image_array_size};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
}

TEST_P(clEnqueueWriteImageVerify, Image3D) {
  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  desc.image_width = 3;
  desc.image_height = 3;
  desc.image_depth = 3;
  desc.image_array_size = 1;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  cl_int error;
  image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                        &error);
  ASSERT_SUCCESS(error);

  const size_t pixel_size = UCL::getPixelSize(format);
  const size_t pixel_count =
      pixel_size * desc.image_width * desc.image_height * desc.image_depth;
  UCL::vector<uint8_t> src_data(pixel_count);
  generate_write_data(src_data);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};
  cl_event write_event;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, image, CL_FALSE, origin,
                                     region, 0, 0, src_data.data(), 0, nullptr,
                                     &write_event));
  UCL::vector<uint8_t> dst_data(pixel_count);
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_TRUE, origin,
                                    region, 0, 0, dst_data.data(), 1,
                                    &write_event, nullptr));
  ASSERT_SUCCESS(clReleaseEvent(write_event));
  for (size_t i = 0; i < pixel_count; i++) {
    EXPECT_EQ(src_data[i], dst_data[i]);
  }
}

INSTANTIATE_TEST_CASE_P(
    SNORM_INT8, clEnqueueWriteImageVerify,
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
    SNORM_INT16, clEnqueueWriteImageVerify,
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
    UNORM_INT8, clEnqueueWriteImageVerify,
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
    UNORM_INT16, clEnqueueWriteImageVerify,
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
    UNORM_SHORT_565, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_565},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_565}));

INSTANTIATE_TEST_CASE_P(
    UNORM_SHORT_555, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_SHORT_555},
                      cl_image_format{CL_RGBx, CL_UNORM_SHORT_555}));

INSTANTIATE_TEST_CASE_P(
    UNORM_INT_101010, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_RGB, CL_UNORM_INT_101010},
                      cl_image_format{CL_RGBx, CL_UNORM_INT_101010}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT8, clEnqueueWriteImageVerify,
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
    SIGNED_INT16, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT16},
                      cl_image_format{CL_Rx, CL_SIGNED_INT16},
                      cl_image_format{CL_A, CL_SIGNED_INT16},
                      cl_image_format{CL_RG, CL_SIGNED_INT16},
                      cl_image_format{CL_RGx, CL_SIGNED_INT16},
                      cl_image_format{CL_RA, CL_SIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    SIGNED_INT32, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_R, CL_SIGNED_INT32},
                      cl_image_format{CL_Rx, CL_SIGNED_INT32},
                      cl_image_format{CL_A, CL_SIGNED_INT32},
                      cl_image_format{CL_RG, CL_SIGNED_INT32},
                      cl_image_format{CL_RGx, CL_SIGNED_INT32},
                      cl_image_format{CL_RA, CL_SIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_SIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT8, clEnqueueWriteImageVerify,
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
    UNSIGNED_INT16, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT16},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_A, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT16},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT16}));

INSTANTIATE_TEST_CASE_P(
    UNSIGNED_INT32, clEnqueueWriteImageVerify,
    ::testing::Values(cl_image_format{CL_R, CL_UNSIGNED_INT32},
                      cl_image_format{CL_Rx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_A, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RG, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGx, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RA, CL_UNSIGNED_INT32},
                      cl_image_format{CL_RGBA, CL_UNSIGNED_INT32}));

INSTANTIATE_TEST_CASE_P(
    HALF_FLOAT, clEnqueueWriteImageVerify,
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
    FLOAT, clEnqueueWriteImageVerify,
    ::testing::Values(
        cl_image_format{CL_R, CL_FLOAT}, cl_image_format{CL_Rx, CL_FLOAT},
        cl_image_format{CL_A, CL_FLOAT},
        cl_image_format{CL_INTENSITY, CL_FLOAT},
        cl_image_format{CL_LUMINANCE, CL_FLOAT},
        cl_image_format{CL_RG, CL_FLOAT}, cl_image_format{CL_RGx, CL_FLOAT},
        cl_image_format{CL_RA, CL_FLOAT}, cl_image_format{CL_RGBA, CL_FLOAT}));

GENERATE_EVENT_WAIT_LIST_TESTS_BLOCKING(clEnqueueWriteImageTest)
