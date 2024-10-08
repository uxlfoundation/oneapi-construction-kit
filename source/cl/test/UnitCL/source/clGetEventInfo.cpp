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

#include <vector>

#include "Common.h"

class clGetEventInfoTest : public ucl::CommandQueueTest {
 protected:
  enum { WIDTH = 4, HEIGHT = 4, SIZE = WIDTH * HEIGHT };

  clGetEventInfoTest() : image_data(SIZE) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    hasImageSupport = getDeviceImageSupport();
    // NOTE: We setup the image format here so that it is initialised when we
    // perform checks to ascertain if an image format is supported.
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;
    image_format.image_channel_order = CL_RGBA;
    // NOTE: We also setup the image descriptor for the same reason as above.
    image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    image_desc.image_width = WIDTH;
    image_desc.image_height = HEIGHT;
    image_desc.image_depth = 1;
    image_desc.image_array_size = 1;
    image_desc.image_slice_pitch = 0;
    image_desc.image_row_pitch = 0;
    image_desc.num_mip_levels = 0;
    image_desc.num_samples = 0;
    image_desc.buffer = nullptr;
  }

  /// @brief Helper to set up image data. As it is set up time, fail hard on
  /// error.
  cl_int SetUpImage() {
    cl_int errcode = CL_SUCCESS;

    region[0] = image_desc.image_width;
    region[1] = image_desc.image_height;
    region[2] = 1;

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    const cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR;

    hasImageSupport = UCL::isImageFormatSupported(
        context, {flags}, image_desc.image_type, image_format);
    if (hasImageSupport) {
      memset(image_data, 255, SIZE * sizeof(cl_uchar4));

      image = clCreateImage(context, flags, &image_format, &image_desc,
                            image_data, &errcode);
    }

    return errcode;
  }

  /// @brief Does a check for required size and then queries for a given
  /// param_name.
  ///
  /// @gotcha This would never work if the return type was dynamically sized
  /// (char * for example).
  /// @tparam T
  /// @param event
  /// @param param_name
  /// @param expected
  template <typename T>
  cl_int GetEventInfoHelper(cl_event event, cl_event_info param_name,
                            T expected) {
    size_t size_needed;
    cl_int errcode =
        clGetEventInfo(event, param_name, 0, nullptr, &size_needed);
    if (CL_SUCCESS != errcode) {
      return errcode;
    }

    assert(sizeof(T) == size_needed);
    T result;
    errcode = clGetEventInfo(event, param_name, size_needed,
                             static_cast<void *>(&result), nullptr);
    EXPECT_SUCCESS(errcode);
    if (expected != result) {
      EXPECT_EQ(expected, result);
      errcode = CL_INVALID_VALUE;
    }
    return errcode;
  }

  /// @brief Helper to set up image data. As it is set up time, fail hard on
  /// error.
  cl_int TearDownImage() {
    if (hasImageSupport) {
      if (image) {
        return clReleaseMemObject(image);
      }
    }
    return CL_SUCCESS;
  }

  cl_mem image = nullptr;
  cl_image_format image_format = {};
  cl_image_desc image_desc = {};
  size_t region[3] = {};
  size_t origin[3] = {};
  UCL::AlignedBuffer<cl_uchar4> image_data;
  bool hasImageSupport = false;
};

TEST_F(clGetEventInfoTest, NullEvent) {
  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT, clGetEventInfo(nullptr, CL_EVENT_CONTEXT,
                                                     0, nullptr, nullptr));
}

TEST_F(clGetEventInfoTest, NDRangeKernelEvent) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);
  const size_t buffer_size = 128;
  cl_mem in_mem = clCreateBuffer(context, 0, buffer_size, nullptr, &errcode);
  EXPECT_TRUE(in_mem);
  ASSERT_SUCCESS(errcode);
  cl_mem out_mem = clCreateBuffer(context, 0, buffer_size, nullptr, &errcode);
  EXPECT_TRUE(out_mem);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&out_mem));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&in_mem));

  const size_t global_size = buffer_size / sizeof(cl_int);
  cl_event event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_NDRANGE_KERNEL));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(out_mem));
  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetEventInfoTest, WriteBufferEvent) {
  const size_t buffer_size = 128;
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                 nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  UCL::vector<char> data(buffer_size, 0);
  cl_event event;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_FALSE, 0,
                                      buffer_size, data.data(), 0, nullptr,
                                      &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_WRITE_BUFFER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetEventInfoTest, ReadBufferEvent) {
  const size_t buffer_size = 128;
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                 nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  UCL::vector<char> data(buffer_size, 0);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, data.data(), 0, nullptr,
                                      nullptr));
  cl_event event = (cl_event)1;
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, data.data(), 0, nullptr,
                                     &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_READ_BUFFER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, cl_uint(1)));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetEventInfoTest, CopyBufferEvent) {
  const size_t buffer_size = 128;
  cl_int errcode;
  cl_mem buffer0 = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                  nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  cl_mem buffer1 = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                  nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  cl_event event;
  ASSERT_SUCCESS(clEnqueueCopyBuffer(command_queue, buffer0, buffer1, 0, 0,
                                     buffer_size, 0, nullptr, &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_COPY_BUFFER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(buffer1));
  ASSERT_SUCCESS(clReleaseMemObject(buffer0));
}

TEST_F(clGetEventInfoTest, UserEvent) {
  cl_int errcode;
  cl_event event = clCreateUserEvent(context, &errcode);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, nullptr));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_USER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_SUBMITTED));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clGetEventInfoTest, FillBufferEvent) {
  const size_t buffer_size = sizeof(cl_uint4) * 4;
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                 nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  cl_uint4 pattern;
  pattern.s[0] = 0;
  pattern.s[1] = 1;
  pattern.s[2] = 2;
  pattern.s[3] = 3;
  cl_event event;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, buffer, &pattern,
                                     sizeof(cl_uint4), 0, buffer_size, 0,
                                     nullptr, &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_FILL_BUFFER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clGetEventInfoTest, EnsureEventTransitionFromQueuedToComplete) {
  cl_int errorcode;
  cl_event userEvent = clCreateUserEvent(context, &errorcode);
  EXPECT_TRUE(userEvent);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent = nullptr;
  ASSERT_SUCCESS(
      clEnqueueMarkerWithWaitList(command_queue, 1, &userEvent, &markerEvent));

  cl_int status;
  ASSERT_SUCCESS(clGetEventInfo(markerEvent, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(status), &status, nullptr));
  ASSERT_GE_EXECSTATUS(status, CL_SUBMITTED);

  ASSERT_SUCCESS(clSetUserEventStatus(userEvent, CL_COMPLETE));

  ASSERT_SUCCESS(clWaitForEvents(1, &markerEvent));

  ASSERT_SUCCESS(clGetEventInfo(markerEvent, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(status), &status, nullptr));
  ASSERT_EQ_EXECSTATUS(CL_COMPLETE, status);

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseEvent(userEvent));
}

TEST_F(clGetEventInfoTest, EnqueueTaskEvent) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int status;
  const char *source =
      "void kernel foo(global int *out, int a, int b) { *out = a * b; }";
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);
  cl_mem out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int),
                              nullptr, &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&out)));
  cl_int a = 7;
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_int), &a));
  cl_int b = 6;
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_int), &b));

  cl_event event;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_TASK));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(out));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetEventInfoTest, MapBufferEvent) {
  cl_int errcode = CL_SUCCESS;
  const size_t size = 128;
  cl_mem in_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);

  cl_event event;
  void *mapped_data =
      clEnqueueMapBuffer(command_queue, in_mem, CL_FALSE, CL_MAP_READ, 0, size,
                         0, nullptr, &event, &errcode);
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(mapped_data);

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_MAP_BUFFER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));

  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
}

TEST_F(clGetEventInfoTest, UnMapMemObjectTest) {
  cl_int errcode = CL_SUCCESS;
  const size_t size = 128;
  cl_mem in_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);

  cl_event event;
  void *mapped_data =
      clEnqueueMapBuffer(command_queue, in_mem, CL_FALSE, CL_MAP_READ, 0, size,
                         0, nullptr, &event, &errcode);
  ASSERT_SUCCESS(errcode);
  ASSERT_TRUE(mapped_data);
  ASSERT_SUCCESS(clWaitForEvents(1, &event));
  ASSERT_SUCCESS(clReleaseEvent(event));

  ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, in_mem, mapped_data, 0,
                                         nullptr, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_UNMAP_MEM_OBJECT));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
}

TEST_F(clGetEventInfoTest, ReadBufferRect) {
  cl_int errcode = CL_SUCCESS;
  const size_t size = 128;
  cl_mem in_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);

  enum { dimensions = 3 };
  const size_t buff_origin[dimensions] = {0, 0, 0};
  const size_t host_origin[dimensions] = {0, 0, 0};
  const size_t region[dimensions] = {1, 1, 1};
  char data = 0;

  cl_event event;
  ASSERT_SUCCESS(clEnqueueReadBufferRect(command_queue, in_mem, CL_TRUE,
                                         buff_origin, host_origin, region, 0, 0,
                                         0, 0, &data, 0, nullptr, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_READ_BUFFER_RECT));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
}

TEST_F(clGetEventInfoTest, WriteBufferRect) {
  cl_int errcode = CL_SUCCESS;
  const size_t size = 128;
  cl_mem in_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);

  enum { dimensions = 3 };
  const size_t buff_origin[dimensions] = {0, 0, 0};
  const size_t host_origin[dimensions] = {0, 0, 0};
  const size_t region[dimensions] = {1, 1, 1};
  const char data = 1;

  cl_event event;
  ASSERT_SUCCESS(clEnqueueWriteBufferRect(command_queue, in_mem, CL_TRUE,
                                          buff_origin, host_origin, region, 0,
                                          0, 0, 0, &data, 0, nullptr, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_WRITE_BUFFER_RECT));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
}

TEST_F(clGetEventInfoTest, CopyBufferRect) {
  cl_int errcode = CL_SUCCESS;
  const size_t size = 128;
  cl_mem in_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);
  cl_mem out_mem =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  ASSERT_SUCCESS(errcode);

  enum { dimensions = 3 };
  const size_t src_offset[dimensions] = {0, 0, 0};
  const size_t dst_offset[dimensions] = {0, 0, 0};
  const size_t region[dimensions] = {1, 1, 1};
  const size_t src_row_pitch = 1;
  const size_t src_slice_pitch = 1;
  const size_t dst_row_pitch = 1;
  const size_t dst_slice_pitch = 1;

  cl_event event;
  ASSERT_SUCCESS(clEnqueueCopyBufferRect(
      command_queue, in_mem, out_mem, &src_offset[0], &dst_offset[0],
      &region[0], src_row_pitch, src_slice_pitch, dst_row_pitch,
      dst_slice_pitch, 0, nullptr, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_COPY_BUFFER_RECT));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
  ASSERT_SUCCESS(clReleaseMemObject(out_mem));
}

static void CL_CALLBACK user_fun(void *args) { (void)args; }

TEST_F(clGetEventInfoTest, NativeKernel) {
  struct Args {
    int a;
    int b;
  } args;
  if (UCL::hasNativeKernelSupport(device)) {
    cl_event event;
    ASSERT_SUCCESS(clEnqueueNativeKernel(command_queue, &user_fun, &args,
                                         sizeof(Args), 0, nullptr, nullptr, 0,
                                         nullptr, &event));
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    cl_int status;

    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_NATIVE_KERNEL));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

    ASSERT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                  sizeof(cl_int), &status, nullptr));
    ASSERT_EQ_EXECSTATUS(CL_COMPLETE, status);
    ASSERT_SUCCESS(clReleaseEvent(event));
  } else {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_OPERATION,
        clEnqueueNativeKernel(command_queue, &user_fun, &args, sizeof(Args), 0,
                              nullptr, nullptr, 0, nullptr, nullptr));
  }
}

TEST_F(clGetEventInfoTest, Marker) {
  cl_event event;
  ASSERT_SUCCESS(clEnqueueMarker(command_queue, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_MARKER));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clGetEventInfoTest, MigrateMemObjects) {
  cl_int errcode = CL_SUCCESS;
  const size_t buffer_size = 128;
  cl_event event;
  cl_mem in_mem = clCreateBuffer(context, 0, buffer_size, nullptr, &errcode);
  ASSERT_TRUE(in_mem);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clEnqueueMigrateMemObjects(command_queue, 1, &in_mem, 0, 0,
                                            nullptr, &event));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(
      GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_MIGRATE_MEM_OBJECTS));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                    CL_COMPLETE));
  ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clGetEventInfoTest, BarrierWithWaitList) {
  cl_int status;
  cl_mem in_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float),
                                 nullptr, &status);
  ASSERT_TRUE(in_mem);
  ASSERT_SUCCESS(status);

  cl_float pattern = 0.0f;
  cl_event fill_event;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, in_mem, &pattern,
                                     sizeof(cl_float), 0, sizeof(cl_float), 0,
                                     nullptr, &fill_event));
  ASSERT_TRUE(fill_event);
  cl_event barrier_event;
  ASSERT_SUCCESS(clEnqueueBarrierWithWaitList(command_queue, 1, &fill_event,
                                              &barrier_event));
  ASSERT_TRUE(barrier_event);
  ASSERT_SUCCESS(clWaitForEvents(1, &barrier_event));
  ASSERT_SUCCESS(
      GetEventInfoHelper(barrier_event, CL_EVENT_COMMAND_QUEUE, command_queue));
  ASSERT_SUCCESS(GetEventInfoHelper(barrier_event, CL_EVENT_CONTEXT, context));
  ASSERT_SUCCESS(GetEventInfoHelper(barrier_event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_BARRIER));
  ASSERT_SUCCESS(GetEventInfoHelper(
      barrier_event, CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));
  ASSERT_SUCCESS(
      GetEventInfoHelper(barrier_event, CL_EVENT_REFERENCE_COUNT, 1));

  ASSERT_SUCCESS(clReleaseMemObject(in_mem));
  ASSERT_SUCCESS(clReleaseEvent(fill_event));
  ASSERT_SUCCESS(clReleaseEvent(barrier_event));
}

TEST_F(clGetEventInfoTest, ReadImage) {
  if (hasImageSupport) {
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());

    UCL::vector<cl_uchar4> result(SIZE);
    ASSERT_EQ(CL_SUCCESS,
              clEnqueueReadImage(command_queue, image, CL_TRUE, origin, region,
                                 0, 0, result.data(), 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_READ_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));
    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, WriteImage) {
  if (hasImageSupport) {
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());

    UCL::vector<cl_uchar4> write_data(SIZE);
    ASSERT_EQ(CL_SUCCESS,
              clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, region,
                                  0, 0, write_data.data(), 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_WRITE_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));
    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, CopyImage) {
  if (hasImageSupport &&
      UCL::isImageFormatSupported(context, {CL_MEM_WRITE_ONLY},
                                  image_desc.image_type, image_format)) {
    cl_int errcode = CL_SUCCESS;
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());
    cl_mem dest_image = clCreateImage(context, CL_MEM_WRITE_ONLY, &image_format,
                                      &image_desc, nullptr, &errcode);
    ASSERT_SUCCESS(errcode);
    ASSERT_TRUE(dest_image);

    ASSERT_EQ(CL_SUCCESS,
              clEnqueueCopyImage(command_queue, image, dest_image, origin,
                                 origin, region, 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_COPY_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));
    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(clReleaseMemObject(dest_image));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, CopyBufferToImage) {
  if (hasImageSupport) {
    cl_int errcode = CL_SUCCESS;
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());
    cl_mem buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                   SIZE * sizeof(cl_uchar4), nullptr, &errcode);
    ASSERT_SUCCESS(errcode);
    ASSERT_TRUE(buffer);

    ASSERT_EQ(CL_SUCCESS,
              clEnqueueCopyBufferToImage(command_queue, buffer, image, 0,
                                         origin, region, 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_COPY_BUFFER_TO_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, CopyImageToBuffer) {
  if (hasImageSupport) {
    cl_int errcode = CL_SUCCESS;
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   SIZE * sizeof(cl_uchar4), nullptr, &errcode);
    ASSERT_SUCCESS(errcode);
    ASSERT_TRUE(buffer);

    ASSERT_EQ(CL_SUCCESS,
              clEnqueueCopyImageToBuffer(command_queue, image, buffer, origin,
                                         region, 0, 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_COPY_IMAGE_TO_BUFFER));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));

    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, MapImage) {
  if (hasImageSupport) {
    cl_int errcode = CL_SUCCESS;
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());
    size_t image_row_pitch = 0;
    size_t image_slice_pitch = 0;
    void *map_ptr = clEnqueueMapImage(
        command_queue, image, CL_TRUE, CL_MAP_WRITE, origin, region,
        &image_row_pitch, &image_slice_pitch, 0, nullptr, &event, &errcode);
    ASSERT_SUCCESS(errcode);

    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE, CL_COMMAND_MAP_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));
    cl_event unmap_event;
    ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, image, map_ptr, 1,
                                           &event, &unmap_event));
    ASSERT_SUCCESS(clWaitForEvents(1, &unmap_event));
    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(clReleaseEvent(unmap_event));
    ASSERT_SUCCESS(TearDownImage());
  }
}

TEST_F(clGetEventInfoTest, FillImage) {
  if (hasImageSupport) {
    cl_event event;
    ASSERT_SUCCESS(SetUpImage());
    const cl_uint blue[4] = {0, 0, 255, 255};
    ASSERT_SUCCESS(clEnqueueFillImage(command_queue, image, blue, origin,
                                      region, 0, nullptr, &event));
    ASSERT_TRUE(event);
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
    ASSERT_SUCCESS(
        GetEventInfoHelper(event, CL_EVENT_COMMAND_QUEUE, command_queue));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_CONTEXT, context));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_FILL_IMAGE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));
    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_REFERENCE_COUNT, 1));
    ASSERT_SUCCESS(clReleaseEvent(event));
    ASSERT_SUCCESS(TearDownImage());
  }
}
/* Redmine #5146: Test each of the following cases once the accompanying
   function has been implemented
CL_COMMAND_ACQUIRE_GL_OBJECTS
CL_COMMAND_RELEASE_GL_OBJECTS
CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR
  (if cl_khr_gl_event is enabled)
CL_COMMAND_ACQUIRE_D3D10_OBJECTS_KHR
  (if cl_khr_d3d10_sharing is enabled)
CL_COMMAND_RELEASE_D3D10_OBJECTS_KHR
  (if cl_khr_d3d10_sharing is enabled)
CL_COMMAND_ACQUIRE_DX9_MEDIA_SURFACES_KHR
  (if cl_khr_dx9_media_sharing is enabled)
CL_COMMAND_RELEASE_DX9_MEDIA_SURFACES_KHR
  (if cl_khr_dx9_media_sharing is enabled)
CL_COMMAND_ACQUIRE_D3D11_OBJECTS_KHR
  (if  cl_khr_d3d11_sharing is enabled)
CL_COMMAND_RELEASE_D3D11_OBJECTS_KHR
  (if  cl_khr_d3d11_sharing is enabled)
*/
