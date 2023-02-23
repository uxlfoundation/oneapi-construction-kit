// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"
#include "EventWaitList.h"

enum {
  IMAGE_WIDTH = 32,
  IMAGE_HEIGHT = 32,
  BUFFER_ELEMENT_COUNT = IMAGE_WIDTH * IMAGE_HEIGHT,
};

class clEnqueueCopyBufferToImageTest : public ucl::CommandQueueTest,
                                       public TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
    data.resize(BUFFER_ELEMENT_COUNT);
    cl_int error;
    for (int index = 0; index < BUFFER_ELEMENT_COUNT; index++) {
      for (int element = 0; element < 4; element++) {
        data[index].s[element] = (float)(index + 42) / (float)(element + 3);
      }
    }
    bufferSize = sizeof(cl_float4) * data.size();
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                            bufferSize, data.data(), &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, buffer);
    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_FLOAT;
    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = IMAGE_WIDTH;
    desc.image_height = IMAGE_HEIGHT;
    desc.image_depth = 0;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;
    if (!UCL::isImageFormatSupported(context, {CL_MEM_READ_WRITE},
                                     desc.image_type, format)) {
      GTEST_SKIP();
    }
    image = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, nullptr,
                          &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, image);
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

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
    ASSERT_EQ_ERRCODE(
        err, clEnqueueCopyBufferToImage(command_queue, buffer, image, 0, origin,
                                        region, num_events, events, event));
  }

  UCL::vector<cl_float4> data;
  size_t bufferSize = 0;
  cl_mem buffer = nullptr;
  cl_mem image = nullptr;
  cl_event event = nullptr;
};

TEST_F(clEnqueueCopyBufferToImageTest, Default) {
  cl_int error;
  cl_event beginEvent = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  cl_event copyEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueCopyBufferToImage(command_queue, buffer, image, 0,
                                            origin, region, 1, &beginEvent,
                                            &copyEvent));
  UCL::vector<cl_float4> result(BUFFER_ELEMENT_COUNT);
  cl_event readEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, image, CL_FALSE, origin,
                                    region, 0, 0, result.data(), 1, &copyEvent,
                                    &readEvent));
  ASSERT_SUCCESS(clSetUserEventStatus(beginEvent, CL_COMPLETE));
  ASSERT_SUCCESS(clWaitForEvents(1, &readEvent));
  ASSERT_SUCCESS(clReleaseEvent(beginEvent));
  ASSERT_SUCCESS(clReleaseEvent(copyEvent));
  ASSERT_SUCCESS(clReleaseEvent(readEvent));

  for (size_t index = 0; index < BUFFER_ELEMENT_COUNT; index++) {
    ASSERT_EQ(data[index].s[0], result[index].s[0]);
    ASSERT_EQ(data[index].s[1], result[index].s[1]);
    ASSERT_EQ(data[index].s[2], result[index].s[2]);
    ASSERT_EQ(data[index].s[3], result[index].s[3]);
  }
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidCommandQueue) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueCopyBufferToImage(nullptr, buffer, image, 0, origin, region, 0,
                                 nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidContext) {
  cl_int error;
  cl_context otherContext =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_NE(nullptr, otherContext);
  cl_command_queue otherCommandQueue =
      clCreateCommandQueue(otherContext, device, 0, &error);
  EXPECT_SUCCESS(error);
  EXPECT_NE(nullptr, otherCommandQueue);

  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueCopyBufferToImage(otherCommandQueue, buffer, image, 0, origin,
                                 region, 0, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseCommandQueue(otherCommandQueue));
  EXPECT_SUCCESS(clReleaseContext(otherContext));
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidMemObject) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueCopyBufferToImage(command_queue, nullptr, image, 0, origin,
                                 region, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clEnqueueCopyBufferToImage(command_queue, buffer, nullptr, 0, origin,
                                 region, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidValueSrcOffset) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferToImage(command_queue, buffer, image, bufferSize + 1,
                                 origin, region, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 1,
                                          origin, region, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidValueDstOrigin) {
  size_t origin[3] = {1, 0, 0};
  size_t region[3] = {IMAGE_WIDTH, IMAGE_HEIGHT, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 0,
                                          origin, region, 0, nullptr, nullptr));
  origin[0] = 0;
  origin[1] = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 0,
                                          origin, region, 0, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferToImage(command_queue, buffer, image, 0, nullptr,
                                 region, 0, nullptr, nullptr));
}

TEST_F(clEnqueueCopyBufferToImageTest, InvalidValueDstRegion) {
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {IMAGE_WIDTH + 1, IMAGE_HEIGHT, 1};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 0,
                                          origin, region, 0, nullptr, nullptr));
  region[0] = IMAGE_WIDTH;
  region[1] = IMAGE_HEIGHT + 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 0,
                                          origin, region, 0, nullptr, nullptr));
  region[1] = IMAGE_HEIGHT;
  region[2] = 2;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCopyBufferToImage(
                                          command_queue, buffer, image, 0,
                                          origin, region, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clEnqueueCopyBufferToImage(command_queue, buffer, image, 0, origin,
                                 nullptr, 0, nullptr, nullptr));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueCopyBufferToImageTest)

// TODO(Redmine #6816): Missing tests
