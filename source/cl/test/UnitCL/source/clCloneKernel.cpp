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

// This file should not be built when targeting OpenCL 1.2.

#include <Common.h>

#include <cstring>

struct clCloneKernelTest : ucl::ContextTest {
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }

  cl_int buildProgramAndCreateSourceKernel() {
    const char *code = R"(
kernel void test(global int* out) {
  size_t id = get_global_id(0);
  out[id] = (int)id;
}
)";
    const size_t length = std::strlen(code);
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &code, &length, &error);
    if (error) {
      return error;
    }
    error =
        clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr);
    if (error) {
      return error;
    }
    source_kernel = clCreateKernel(program, "test", &error);
    return error;
  }

  virtual void TearDown() {
    if (clone_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(clone_kernel));
    }
    if (source_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(source_kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ucl::ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel source_kernel = nullptr;
  cl_kernel clone_kernel = nullptr;
};

TEST_F(clCloneKernelTest, Success) {
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(buildProgramAndCreateSourceKernel());
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, clone_kernel);
}

TEST_F(clCloneKernelTest, SuccessWithArgs) {
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(buildProgramAndCreateSourceKernel());
  UCL::cl::buffer buffer;
  ASSERT_SUCCESS(
      buffer.create(context, CL_MEM_READ_WRITE, sizeof(cl_int) * 64, nullptr));
  ASSERT_SUCCESS(clSetKernelArg(source_kernel, 0, sizeof(buffer),
                                static_cast<void *>(&buffer)));
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  ASSERT_SUCCESS(error);
  ASSERT_NE(nullptr, clone_kernel);
}

TEST_F(clCloneKernelTest, InvalidKernel) {
  ASSERT_EQ(nullptr, clCloneKernel(nullptr, nullptr));
  cl_int error;
  ASSERT_EQ(nullptr, clCloneKernel(nullptr, &error));
  ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL, error);
}

struct clCloneKernelRunTest : clCloneKernelTest {
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(clCloneKernelTest::SetUp());
    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(buildProgramAndCreateSourceKernel());
    cl_int error;
    command_queue = clCreateCommandQueue(context, device, 0, &error);
    ASSERT_SUCCESS(error);
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                            sizeof(cl_int) * work_size, nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  virtual void TearDown() {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (command_queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));
    }
    clCloneKernelTest::TearDown();
  }

  const size_t work_size = 64;
  cl_command_queue command_queue = nullptr;
  cl_mem buffer = nullptr;
};

TEST_F(clCloneKernelRunTest, Default) {
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  // Set an argument on clone_kernel, run it, then validate.
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clSetKernelArg(clone_kernel, 0, sizeof(buffer),
                                static_cast<void *>(&buffer)));
  cl_event ndRangeEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, clone_kernel, 1, nullptr,
                                        &work_size, nullptr, 0, nullptr,
                                        &ndRangeEvent));
  std::vector<cl_int> results(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     sizeof(cl_int) * work_size, results.data(),
                                     1, &ndRangeEvent, nullptr));
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, results[index]) << "at index: " << index;
  }
  // Check that source_kernel doens't have the clone_kernel argument set.
  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL_ARGS,
      clEnqueueNDRangeKernel(command_queue, source_kernel, 1, nullptr,
                             &work_size, nullptr, 0, nullptr, &ndRangeEvent));

  EXPECT_SUCCESS(clReleaseEvent(ndRangeEvent));
}

TEST_F(clCloneKernelRunTest, WithArgs) {
  // Set an argument on the source_kernel, run it, then validate.
  ASSERT_SUCCESS(clSetKernelArg(source_kernel, 0, sizeof(buffer),
                                static_cast<void *>(&buffer)));
  cl_event ndRangeSourceEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, source_kernel, 1,
                                        nullptr, &work_size, nullptr, 0,
                                        nullptr, &ndRangeSourceEvent));
  std::vector<cl_int> results(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     sizeof(cl_int) * work_size, results.data(),
                                     1, &ndRangeSourceEvent, nullptr));
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, results[index]) << "at index: " << index;
  }
  // Reset buffer to zero.
  cl_int pattern = 0;
  cl_event fillEvent;
  ASSERT_SUCCESS(
      clEnqueueFillBuffer(command_queue, buffer, &pattern, sizeof(pattern), 0,
                          sizeof(cl_int) * work_size, 0, nullptr, &fillEvent));
  // Clone source_kernel, run clone_kernel, then validate.
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  ASSERT_SUCCESS(error);
  cl_event ndRangeCloneEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, clone_kernel, 1, nullptr,
                                        &work_size, nullptr, 0, nullptr,
                                        &ndRangeCloneEvent));
  std::vector<cl_int> cloneResults(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, buffer, CL_TRUE, 0, sizeof(cl_int) * work_size,
      cloneResults.data(), 1, &ndRangeCloneEvent, nullptr));
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, results[index]) << "at index: " << index;
  }

  EXPECT_SUCCESS(clReleaseEvent(ndRangeSourceEvent));
  EXPECT_SUCCESS(clReleaseEvent(fillEvent));
  EXPECT_SUCCESS(clReleaseEvent(ndRangeCloneEvent));
}

TEST_F(clCloneKernelRunTest, WithChangedArgs) {
  // Set an argument on the source_kernel, run it, then validate.
  ASSERT_SUCCESS(clSetKernelArg(source_kernel, 0, sizeof(buffer),
                                static_cast<void *>(&buffer)));
  cl_event ndRangeSourceEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, source_kernel, 1,
                                        nullptr, &work_size, nullptr, 0,
                                        nullptr, &ndRangeSourceEvent));
  std::vector<cl_int> sourceResults(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, buffer, CL_TRUE, 0, sizeof(cl_int) * work_size,
      sourceResults.data(), 1, &ndRangeSourceEvent, nullptr));
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, sourceResults[index]) << "at index: " << index;
  }
  // Create another buffer.
  UCL::cl::buffer otherBuffer;
  ASSERT_SUCCESS(otherBuffer.create(context, CL_MEM_READ_WRITE,
                                    sizeof(cl_int) * work_size, nullptr));
  // Clone source_kernel, set otherBuffer as argument, run it.
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(
      clSetKernelArg(clone_kernel, 0, sizeof(otherBuffer), &otherBuffer));
  cl_event ndRangeCloneEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, clone_kernel, 1, nullptr,
                                        &work_size, nullptr, 0, nullptr,
                                        &ndRangeCloneEvent));

  std::vector<cl_int> cloneResults(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, otherBuffer, CL_TRUE, 0, sizeof(cl_int) * work_size,
      cloneResults.data(), 1, &ndRangeCloneEvent, nullptr));
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, cloneResults[index]) << "at index: " << index;
  }

  EXPECT_SUCCESS(clReleaseEvent(ndRangeSourceEvent));
  EXPECT_SUCCESS(clReleaseEvent(ndRangeCloneEvent));
}

TEST_F(clCloneKernelRunTest, ParallelWithChangedArgs) {
  // Set an argument on the source_kernel.
  ASSERT_SUCCESS(clSetKernelArg(source_kernel, 0, sizeof(buffer),
                                static_cast<void *>(&buffer)));
  // Clone source_kernel.
  cl_int error;
  clone_kernel = clCloneKernel(source_kernel, &error);
  ASSERT_SUCCESS(error);
  // Create another buffer, and set as argument on the clone_kernel.
  UCL::cl::buffer otherBuffer;
  ASSERT_SUCCESS(otherBuffer.create(context, CL_MEM_READ_WRITE,
                                    sizeof(cl_int) * work_size, nullptr));
  ASSERT_SUCCESS(
      clSetKernelArg(clone_kernel, 0, sizeof(otherBuffer), &otherBuffer));
  // Run both kernels.
  cl_event ndRangeSourceEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, source_kernel, 1,
                                        nullptr, &work_size, nullptr, 0,
                                        nullptr, &ndRangeSourceEvent));
  cl_event ndRangeCloneEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, clone_kernel, 1, nullptr,
                                        &work_size, nullptr, 0, nullptr,
                                        &ndRangeCloneEvent));
  // Read results.
  std::vector<cl_int> sourceResults(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, buffer, CL_TRUE, 0, sizeof(cl_int) * work_size,
      sourceResults.data(), 1, &ndRangeSourceEvent, nullptr));
  std::vector<cl_int> cloneResults(work_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, otherBuffer, CL_TRUE, 0, sizeof(cl_int) * work_size,
      cloneResults.data(), 1, &ndRangeCloneEvent, nullptr));
  // Validate results.
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, sourceResults[index]) << "at index: " << index;
  }
  for (size_t index = 0; index < work_size; index++) {
    ASSERT_EQ(index, cloneResults[index]) << "at index: " << index;
  }

  EXPECT_SUCCESS(clReleaseEvent(ndRangeSourceEvent));
  EXPECT_SUCCESS(clReleaseEvent(ndRangeCloneEvent));
}
