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

#include <limits>

#include "Common.h"

class clReleaseEventTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    event = clCreateUserEvent(context, &errorcode);
    EXPECT_TRUE(event);
    ASSERT_SUCCESS(errorcode);
  }

  cl_event event = nullptr;
};

TEST_F(clReleaseEventTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_EVENT, clReleaseEvent(nullptr));
  ASSERT_SUCCESS(clReleaseEvent(event));
}

class clReleaseEventWithQueueTest : public ucl::ContextTest {
  /*
   This regression test is a modified version of a test that checks queues with
   profiling of events from the CTS. We don't care about most of the kernel
   setup and execution, but only the destruction of the queue and event and the
   ordering involved. The CTS destructs the queue and events in a different
   order than we had previously expected, which exposed an invalid data
   dependency in the event on the queue. This resulted in the event accessing a
   queue through a dangling pointer and subsequent SIGSEGV Thus: all the
   boilerplate is in SetUp() and the actual meat of the test is simply the
   destruction of the bits that exposed this issue
   */
 protected:
  void SetUp() override {
    const char *src[] = {
        "__kernel void array_copy("
        "        __global unsigned char *src, __global unsigned char *dst) {"
        "  size_t i = get_global_id(0);"
        "  dst[i] = src[i];"
        "}\n",
    };
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    ASSERT_TRUE(context);
    ASSERT_TRUE(platform);
    cl_int err;
    program = clCreateProgramWithSource(context, sizeof src / sizeof src[0],
                                        src, nullptr, &err);
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));
    kernel = clCreateKernel(program, "array_copy", &err);
    ASSERT_SUCCESS(err);
    auto clCreateCommandQueueWithPropertiesKHR =
        (clCreateCommandQueueWithPropertiesKHR_fn)
            clGetExtensionFunctionAddressForPlatform(
                platform, "clCreateCommandQueueWithPropertiesKHR");
    EXPECT_TRUE(clCreateCommandQueueWithPropertiesKHR);
    queue = clCreateCommandQueueWithPropertiesKHR(context, device, queue_props,
                                                  &err);
    const size_t n = std::numeric_limits<cl_uchar>::max();
    std::vector<cl_uchar> input(n), output(n);
    for (size_t i = 0; i < n; ++i) {
      input[i] = i;
    }
    streams[0] =
        clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, n,
                       input.data(), &err);
    streams[1] = clCreateBuffer(
        context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, n, nullptr, &err);
    err = clSetKernelArg(kernel, 0, sizeof(streams[0]),
                         static_cast<void *>(&streams[0]));
    EXPECT_SUCCESS(err);

    err = clSetKernelArg(kernel, 1, sizeof(streams[1]),
                         static_cast<void *>(&streams[1]));
    EXPECT_SUCCESS(err);

    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &n, nullptr, 0,
                                 nullptr, &event);
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(event);

    err = clWaitForEvents(1, &event);
    EXPECT_SUCCESS(err);

    err = clEnqueueReadBuffer(queue, streams[1], CL_TRUE, 0, n, output.data(),
                              0, nullptr, nullptr);
    for (size_t i = 0; i < n; ++i) {
      ASSERT_EQ((cl_uchar)i, output[i]);
    }
  }

  void TearDown() override {
    for (auto &stream : streams) {
      if (stream) {
        EXPECT_SUCCESS(clReleaseMemObject(stream));
      }
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_kernel kernel = nullptr;
  cl_program program = nullptr;
  cl_mem streams[2] = {nullptr};
  cl_command_queue queue = nullptr;
  cl_event event = nullptr;
  cl_queue_properties_khr queue_props[3] = {CL_QUEUE_PROPERTIES,
                                            CL_QUEUE_PROFILING_ENABLE, 0};
};

// Destroying the queue or event should be order independent and shouldn't cause
// crashes like that one time...
TEST_F(clReleaseEventWithQueueTest, QueueGoFirst) {
  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clReleaseEventWithQueueTest, EventGoFirst) {
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
  ASSERT_SUCCESS(clReleaseEvent(event));
}
