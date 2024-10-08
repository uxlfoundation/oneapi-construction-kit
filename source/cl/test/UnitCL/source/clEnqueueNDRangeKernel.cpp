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
#include <atomic>
#include <cmath>
#include <limits>
#include <thread>

#include "Common.h"
#include "EventWaitList.h"

class clEnqueueNDRangeKernelTest : public ucl::CommandQueueTest,
                                   TestWithEventWaitList {
 protected:
  enum { SIZE = 128 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    memset(buffer, 42, sizeof(buffer));

    const char *source =
        "void kernel foo(global int * a, global int * b) {\n"
        "    if (a) *a = *b;\n"
        "}"
        "void kernel foo_pod(global int * a, global int * b, int pod_val) {\n"
        "    if (a) *a = *b + pod_val;\n"
        "}";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);

    inMem = clCreateBuffer(context, 0, SIZE, nullptr, &errorcode);
    EXPECT_TRUE(inMem);
    ASSERT_SUCCESS(errorcode);

    // Write data to the in buffer
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, CL_TRUE, 0, SIZE,
                                        buffer, 0, nullptr, nullptr));

    outMem = clCreateBuffer(context, 0, SIZE, nullptr, &errorcode);
    EXPECT_TRUE(outMem);
    ASSERT_SUCCESS(errorcode);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outMem));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inMem));
  }

  void TearDown() override {
    if (outMem) {
      EXPECT_SUCCESS(clReleaseMemObject(outMem));
    }
    if (inMem) {
      EXPECT_SUCCESS(clReleaseMemObject(inMem));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    const size_t global_size = SIZE / sizeof(cl_int);
    EXPECT_EQ_ERRCODE(err, clEnqueueNDRangeKernel(
                               command_queue, kernel, 1, nullptr, &global_size,
                               nullptr, num_events, events, event));
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem inMem = nullptr;
  cl_mem outMem = nullptr;
  char buffer[SIZE] = {};
};

TEST_F(clEnqueueNDRangeKernelTest, OneDimension) {
  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, TwoDimensions) {
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, global_size / 2};
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 2, nullptr,
                                        sizes, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, ThreeDimensions) {
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidQueue) {
  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueNDRangeKernel(nullptr, kernel, 1, nullptr, &global_size, nullptr,
                             0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidKernel) {
  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clEnqueueNDRangeKernel(command_queue, nullptr, 1, nullptr, &global_size,
                             nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, QueueHasOtherContext) {
  cl_int errorcode;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(errorcode);

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(context);
  EXPECT_SUCCESS(errorcode);

  const size_t global_size = SIZE / sizeof(cl_int);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size, nullptr,
                             0, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
  EXPECT_SUCCESS(clReleaseContext(context));
}

TEST_F(clEnqueueNDRangeKernelTest, KernelArgsNotSet) {
  cl_int errorcode;
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errorcode);

  const size_t global_size = SIZE / sizeof(cl_int);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_KERNEL_ARGS,
      clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &global_size,
                             nullptr, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidWorkDimSmall) {
  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_DIMENSION,
      clEnqueueNDRangeKernel(command_queue, kernel, 0, nullptr, &global_size,
                             nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidWorkDimBig) {
  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_DIMENSION,
      clEnqueueNDRangeKernel(command_queue, kernel, 4, nullptr, &global_size,
                             nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidLocalWorkSize1D) {
  size_t max_work_group_size;
  size_t max_work_item_sizes[3];
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof(size_t), &max_work_group_size,
                                 nullptr));
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                 sizeof(size_t) * 3, &max_work_item_sizes,
                                 nullptr));

  // Check the assumption that we can add 1 to max_work_group_size without
  // overflowing size_t.
  ASSERT_LE(max_work_group_size, std::numeric_limits<size_t>::max() - 1u);

  // Use this size for both global and local size.  The value is legal w.r.t
  // CL_DEVICE_MAX_WORK_ITEM_SIZES, but potentially not with
  // CL_DEVICE_MAX_WORK_GROUP_SIZE.
  const size_t size =
      std::min(max_work_item_sizes[0], max_work_group_size + 1u);

  // Although the above checks ensure that CL_INVALID_WORK_ITEM_SIZE is never
  // the expected return code below, the first max local size may be less than
  // the total max.  In this case it is not possible to trigger
  // CL_INVALID_WORK_GROUP_SIZE with a single dimension.
  const cl_int expected =
      (size <= max_work_group_size) ? CL_SUCCESS : CL_INVALID_WORK_GROUP_SIZE;

  // Note that even if we expect CL_SUCCESS, it is still safe to enqueue the
  // large range because the kernel doesn't use get_global_id to index into
  // buffers.
  ASSERT_EQ_ERRCODE(
      expected, clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &size,
                                       &size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidLocalWorkSizeBigCube) {
  size_t max_work_group_size;
  size_t max_work_item_sizes[3];
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof(size_t), &max_work_group_size,
                                 nullptr));
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                 sizeof(size_t) * 3, &max_work_item_sizes,
                                 nullptr));

  // Use this size for both global and local size.  Each dimension is legal
  // (both w.r.t CL_DEVICE_MAX_WORK_GROUP_SIZE and
  // CL_DEVICE_MAX_WORK_ITEM_SIZES), but the total is not constrained.
  const size_t size[3] = {
      std::min(max_work_item_sizes[0], max_work_group_size),
      std::min(max_work_item_sizes[1], max_work_group_size),
      std::min(max_work_item_sizes[2], max_work_group_size)};

  // Figure out the total work group size that the above represents, while
  // checking the assumption that we can multiply together the above size_t
  // values and still have the result be a size_t without overflowing.
  ASSERT_GT(size[0], 0u);
  ASSERT_GT(size[1], 0u);
  ASSERT_GT(size[2], 0u);
  // The following two statements trigger divide by zero warnings in clang-tidy
  // because it does not understand gtest assertions so mark them NOLINT to
  // disable the check.
  ASSERT_LE(size[0], std::numeric_limits<size_t>::max() / size[1]);  // NOLINT
  ASSERT_LE(size[0] * size[1],
            std::numeric_limits<size_t>::max() / size[2]);  // NOLINT
  const size_t total_size = size[0] * size[1] * size[2];

  // Although the above checks ensure that CL_INVALID_WORK_ITEM_SIZE is never
  // the expected return code below, there are two circumstances under which it
  // is not possible to trigger CL_INVALID_WORK_GROUP_SIZE while using
  // otherwise legal values:
  // (a) If the max work group size is 1, then 1^3 is also 1, which is valid.
  // (b) The product of the max local sizes may be less than the total max.
  const cl_int expected = (total_size <= max_work_group_size)
                              ? CL_SUCCESS
                              : CL_INVALID_WORK_GROUP_SIZE;

  // Note that even if we expect CL_SUCCESS, it is still safe to enqueue the
  // large range because the kernel doesn't use get_global_id to index into
  // buffers.
  ASSERT_EQ_ERRCODE(
      expected, clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr, size,
                                       size, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, InvalidLocalWorkItemSize) {
  size_t max_work_group_size;
  size_t max_work_item_sizes[3];
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof(size_t), &max_work_group_size,
                                 nullptr));
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                 sizeof(size_t) * 3, &max_work_item_sizes,
                                 nullptr));

  // For a local work item dimension of N test local work group sizes of:
  //   - {N+1, 1, 1}
  //   - {1, N+1, 1}
  //   - {1, 1, N+1}.
  for (size_t i = 0; i < 3; i++) {
    // Check the assumption that we can add 1 to max_work_item_size[i] without
    // overflowing size_t.
    ASSERT_LE(max_work_item_sizes[i], std::numeric_limits<size_t>::max() - 1u);

    const size_t size[3] = {(i == 0) ? max_work_item_sizes[0] + 1u : 1u,
                            (i == 1) ? max_work_item_sizes[1] + 1u : 1u,
                            (i == 2) ? max_work_item_sizes[2] + 1u : 1u};

    // It's a bit awkward, but if max_work_group_size is N, and
    // max_work_item_sizes is {N, N, N} then for a local work group size of
    // {N+1, 1, 1} then either CL_INVALID_WORK_GROUP_SIZE or
    // CL_DEVICE_MAX_WORK_ITEM_SIZES are valid return codes.
    const cl_int err = clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                              size, size, 0, nullptr, nullptr);
    EXPECT_TRUE((err == CL_INVALID_WORK_ITEM_SIZE) ||
                (err == CL_INVALID_WORK_GROUP_SIZE));
  }
}

TEST_F(clEnqueueNDRangeKernelTest, ChangeKernelArguments) {
  const size_t global_size = SIZE / sizeof(cl_int);
  cl_int err;

  // Prepare the alternate buffer data
  char newBuffer[SIZE];
  newBuffer[0] = 21;  // Only the first element is actually used

  cl_mem newInMem = clCreateBuffer(context, 0, SIZE, nullptr, &err);
  EXPECT_TRUE(newInMem);
  ASSERT_SUCCESS(err);

  // Write the new data to the new in buffer
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, newInMem, CL_TRUE, 0, SIZE,
                                      newBuffer, 0, nullptr, nullptr));

  // Use a user event to block the call until we've changed the arguments
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_TRUE(user_event);
  ASSERT_SUCCESS(err);

  // Enqueue with the default arguments
  cl_event enqueue_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 1, &user_event,
                                        &enqueue_event));

  // Change the input kernel argument.
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&newInMem));

  // Unblock the enqueueNDRange call
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Read the data in the outMem buffer
  char readBuffer[SIZE];
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, CL_TRUE, 0, SIZE,
                                     (void *)readBuffer, 1, &enqueue_event,
                                     nullptr));

  // The out buffer should contain the data of the original buffer and not the
  // data of the argument set after the enqueueNDRangeKernel call. Only the
  // first element is set in the kernel.
  ASSERT_EQ(buffer[0], readBuffer[0]);

  EXPECT_SUCCESS(clReleaseMemObject(newInMem));
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseEvent(enqueue_event));
}

// Similar to ChangeKernelArguments, we then immediately enqueue after changing
// with a different pod value and we check that the right result occurs in both
// cases
TEST_F(clEnqueueNDRangeKernelTest, ChangeKernelArgumentsPod) {
  const size_t global_size = SIZE / sizeof(cl_int);
  cl_int err;

  cl_kernel kernel_pod;
  kernel_pod = clCreateKernel(program, "foo_pod", &err);
  EXPECT_TRUE(kernel_pod);
  ASSERT_SUCCESS(err);

  // Prepare the alternate buffer data
  cl_int value1 = 0;
  cl_int value2 = 100;

  UCL::cl::buffer newOutMem;
  ASSERT_SUCCESS(newOutMem.create(context, 0, SIZE, nullptr));

  // Set the pod input kernel argument.
  ASSERT_SUCCESS(
      clSetKernelArg(kernel_pod, 0, sizeof(cl_mem), (void *)&outMem));
  ASSERT_SUCCESS(clSetKernelArg(kernel_pod, 1, sizeof(cl_mem), (void *)&inMem));
  ASSERT_SUCCESS(
      clSetKernelArg(kernel_pod, 2, sizeof(uint32_t), (void *)&value1));

  // Enqueue with the default arguments
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel_pod, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  // Change the output kernel argument.
  ASSERT_SUCCESS(
      clSetKernelArg(kernel_pod, 0, sizeof(cl_mem), (void *)&newOutMem));
  ASSERT_SUCCESS(
      clSetKernelArg(kernel_pod, 2, sizeof(uint32_t), (void *)&value2));

  // Enqueue with the default arguments
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel_pod, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  // Read the data in the outMem buffer
  cl_int readBuffer[SIZE];
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, CL_TRUE, 0, SIZE,
                                     (void *)readBuffer, 0, nullptr, nullptr));

  const cl_int buf0 = reinterpret_cast<cl_int *>(buffer)[0];

  // The out buffer should contain the data of the original buffer and not the
  // data of the arguments set after the enqueueNDRangeKernel call. Only the
  // first element is set in the kernel.
  ASSERT_EQ(buf0, readBuffer[0]);

  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, newOutMem, CL_TRUE, 0, SIZE,
                                     (void *)readBuffer, 0, nullptr, nullptr));

  // The second out buffer should contain the data of the original buffer + 100,
  // due to the change of the third argument
  const cl_int expectedResult = buf0 + 100;
  ASSERT_EQ(expectedResult, readBuffer[0]);

  EXPECT_SUCCESS(clReleaseKernel(kernel_pod));
}

// CL_INVALID_GLOBAL_OFFSET if the value specified in global_work_size + the
// corresponding values in global_work_offset for any dimensions is greater than
// the "sizeof(size_t)" (means max value of size_t) for the device on which the
// kernel execution will be enqueued.
TEST_F(clEnqueueNDRangeKernelTest, InvalidGlobalOffset) {
  const size_t global_work_offset[3] = {std::numeric_limits<size_t>::max(),
                                        std::numeric_limits<size_t>::max(),
                                        std::numeric_limits<size_t>::max()};
  const size_t global_size[3] = {std::numeric_limits<size_t>::max(),
                                 std::numeric_limits<size_t>::max(),
                                 std::numeric_limits<size_t>::max()};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_GLOBAL_OFFSET,
      clEnqueueNDRangeKernel(command_queue, kernel, 1, global_work_offset,
                             global_size, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelTest, NullBuffer) {
  // Overwrite the first kernel argument with NULL, this is legal OpenCL, the
  // expected behaviour is that NULL is passed to the kernel itself.  This
  // test, however, mostly exists to make sure that the API code doesn't assume
  // that the argument will always be non-null for cl_mem's.
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), nullptr));

  const size_t global_size = SIZE / sizeof(cl_int);
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueNDRangeKernelTest)

// This test exists to prove that no data-race on LLVM global variables exists
// between calls to `clBuildProgram`, `clCreateKernel`, and
// `clEnqueueNDRangeKernel`.  It did not expose any additional issues over the
// slightly simpler clCreateKernelTest::ConcurrentBuildAndCreate test at the
// time of writing, but including it anyway as it targets a different area of
// the code base (clEnqueueNDRangeKernel).
TEST_F(clEnqueueNDRangeKernelTest, ConcurrentCreateAndEnqueue) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k() {}";
  const size_t range = 1;

  auto worker = [this, &src, &range]() {
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, nullptr);
    std::vector<cl_event> kernel_events;
    kernel_events.reserve(16);

    for (int i = 0; i < 16; i++) {
      cl_program program =
          clCreateProgramWithSource(context, 1, &src, nullptr, nullptr);
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
      cl_kernel kernel = clCreateKernel(program, "k", nullptr);
      cl_event kernel_event;
      clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &range, nullptr, 0,
                             nullptr, &kernel_event);
      kernel_events.push_back(kernel_event);
      clReleaseKernel(kernel);
      clReleaseProgram(program);
    }

    clFinish(queue);
    for (cl_event e : kernel_events) {
      ASSERT_TRUE(UCL::hasCommandExecutionCompleted(e));
      ASSERT_SUCCESS(clReleaseEvent(e));
    }
    clReleaseCommandQueue(queue);
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}

// This is the same as ConcurrentCreateAndEnqueue above, except that each
// worker thread has it's own cl_context, which allows much more LLVM
// concurrency (on separate LLVMContexts).  This exposed additional issues
// involving LLVM global state.
TEST_F(clEnqueueNDRangeKernelTest, ConcurrentContextCreateAndEnqueue) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k() {}";
  const size_t range = 1;

  auto worker = [&, range]() {
    cl_context ctx =
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, nullptr);

    std::vector<cl_event> kernel_complete_events;
    kernel_complete_events.reserve(32);
    for (int i = 0; i < 32; i++) {
      cl_program program =
          clCreateProgramWithSource(ctx, 1, &src, nullptr, nullptr);
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
      cl_kernel kernel = clCreateKernel(program, "k", nullptr);
      cl_event kernel_event;
      ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &range,
                                            nullptr, 0, nullptr,
                                            &kernel_event));
      kernel_complete_events.push_back(kernel_event);
      clReleaseKernel(kernel);
      clReleaseProgram(program);
    }

    clFinish(queue);
    for (cl_event event : kernel_complete_events) {
      ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));
      ASSERT_SUCCESS(clReleaseEvent(event));
    }

    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}

// This test is similar to clEnqueueNDRangeKernelTest::
// ConcurrentContextCreateAndEnqueue, but each kernel produces an output that
// is checked.  I.e. this test could theoretically fail outside of a thread
// sanitizer.  Each version of the kernel gets a different output value set as
// a build option define, this is because LLVM/Clang option handling code has
// some global state, so this test is to try and ensure that despite the global
// state each thread (and LLVMContext) will always preserve its own required
// options in the presence of parallel builds.
TEST_F(clEnqueueNDRangeKernelTest, ConcurrentBuildDefines) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k(global int *out) { *out = (VAL); }";
  const size_t range = 1;

  // This isn't a very precise way to record failure, we won't know which
  // thread failed etc, but that can be investigated in a debugger.  The value
  // of this variable is monotonic.  It gets initialised to true, and then
  // either never gets written to again, or gets false written to it.
  std::atomic<bool> success{true};

  auto worker = [&, range]() {
    cl_context ctx =
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, nullptr);

    for (int i = 0; i < 16; i++) {
      cl_program program =
          clCreateProgramWithSource(ctx, 1, &src, nullptr, nullptr);
      const std::string define = std::string("-DVAL=") + std::to_string(i);
      clBuildProgram(program, 0, nullptr, define.c_str(), nullptr, nullptr);
      cl_mem buf = clCreateBuffer(ctx, 0, sizeof(cl_int), nullptr, nullptr);
      cl_kernel kernel = clCreateKernel(program, "k", nullptr);
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&buf));
      clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &range, nullptr, 0,
                             nullptr, nullptr);
      cl_int result = -1;
      clEnqueueReadBuffer(queue, buf, CL_TRUE, 0, sizeof(cl_int), &result, 0,
                          nullptr, nullptr);
      if (result != i) {
        // Note: If any of the above OpenCL functions failed then 'result' will
        // not get the value set, so in fact they are all imprecisely checked.
        success = false;
      }
      clReleaseMemObject(buf);
      clReleaseKernel(kernel);
      clReleaseProgram(program);
    }

    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  EXPECT_TRUE(success);
}

// This test is notionally similar to clEnqueueNDRangeKernelTest
// ConcurrentBuildDefines, but is closer in structure to
// ConcurrentContextCreateAndEnqueue.  It attempts to set -Werror in some
// builds but not all, so if if a #warn and racy option parsing could result in
// the wrong build failing. At the time of writing it did not manage to trigger
// such behaviour, as so much of our Option parsing code is our own rather than
// using Clang's, but it did cause segfaults on AArch64 with 100% reliability.
TEST_F(clEnqueueNDRangeKernelTest, ConcurrentBuildOptions) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "#warning Oh no!\nkernel void k() {}";
  const char *option_error = "-Werror -cl-opt-disable";
  const char *option_noop = "-cl-opt-disable";

  // This isn't a very precise way to record failure, we won't know which
  // thread failed etc, but that can be investigated in a debugger.  The value
  // of this variable is monotonic.  It gets initialised to true, and then
  // either never gets written to again, or gets false written to it.
  std::atomic<bool> success{true};

  auto worker = [&]() {
    cl_context ctx =
        clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);

    for (int i = 0; i < 32; i++) {
      const bool expect_error = 0 == (i % 2);
      const char *opt = expect_error ? option_error : option_noop;

      cl_program program =
          clCreateProgramWithSource(ctx, 1, &src, nullptr, nullptr);
      const cl_int err =
          clBuildProgram(program, 0, nullptr, opt, nullptr, nullptr);
      if ((expect_error && CL_SUCCESS == err) ||
          (!expect_error && CL_SUCCESS != err)) {
        success = false;
      }
      clReleaseProgram(program);
    }

    clReleaseContext(ctx);
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  EXPECT_TRUE(success);
}

// Completing a command might internally remove or reuse the signaling primitive
// following commands depend on. Should this happen, then a deadlock can occur
// if newly enqueued commands internally reuse the signaling primitive but
// also depend on the earlier commands now waiting on them to complete.
TEST_F(clEnqueueNDRangeKernelTest, NoDeadlockDueToInternalEventCaching) {
  auto possible_deadlock_callback = [](cl_event, cl_int, void *user_data) CL_LAMBDA_CALLBACK {
    // Event should be from the predecessing command.
    cl_event predecessing_command_event = *(static_cast<cl_event *>(user_data));
    cl_int status = CL_QUEUED;

    // Checking the event of the predecessing command requires admin work inside
    // the OpenCL runtime which might reset internal signaling so following
    // commands might deadlock waiting for that signal.
    while ((CL_COMPLETE != status) && (0 <= status /* no error */)) {
      (void)clGetEventInfo(predecessing_command_event,
                           CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status),
                           &status, nullptr);
    }
  };

  // Collect all command events for later release.
  std::vector<cl_event> events;
  cl_event event = nullptr;

  const size_t global_size = 1;

  // First command and member of the first command group.
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, events.size(),
                                        nullptr, &event));
  events.push_back(event);
  event = nullptr;

  // Second command and member of the first command group.
  // Command completion should lead to a reset of its command group signal
  // semaphore.
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, events.size(),
                                        events.data(), &event));
  cl_event callback_wait_event = event;
  events.push_back(event);
  event = nullptr;

  // Third command and member of the second command group.
  // Anchor for the event callback described below.
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, events.size(),
                                        events.data(), &event));
  events.push_back(event);

  // Once the above ND range kernel command is complete, check the status of the
  // event of the previous command which may trigger an internal cleanup and
  // reset of the previous command group signaling primitive. If command group
  // dependencies are implemented to directly access signaling primitives of
  // other command groups, then a deadlock can occur due to waiting on a signal
  // of a command group itself waiting on the earlier command group's signal.
  ASSERT_EQ(CL_SUCCESS,
            clSetEventCallback(event, CL_COMPLETE, possible_deadlock_callback,
                               static_cast<void *>(&callback_wait_event)));
  event = nullptr;

  // Fourth command and member of the third command group.
  // The command group should not wait on a reused and reset signal primitive
  // of an earlier command group otherwise a deadlock should occur.
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, events.size(),
                                        events.data(), &event));
  events.push_back(event);
  event = nullptr;

  // Dispatch all commands and their associated command groups.
  ASSERT_EQ(CL_SUCCESS, clFinish(command_queue));
  for (cl_event event : events) {
    ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));
    ASSERT_SUCCESS(clReleaseEvent(event));
  }
}

#ifdef CL_VERSION_3_0
TEST_F(clEnqueueNDRangeKernelTest, ZeroNDRange) {
  auto check_ndrange = [&](int dimension, const size_t *ndrange) {
    char pattern = 0;
    ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, outMem, &pattern,
                                       sizeof(pattern), 0, SIZE, 0, nullptr,
                                       nullptr));

    ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, dimension,
                                          nullptr, ndrange, nullptr, 0, nullptr,
                                          nullptr));

    // Check that the output buffer has not changed.
    cl_int error;
    auto *data = static_cast<char *>(
        clEnqueueMapBuffer(command_queue, outMem, CL_TRUE, CL_MAP_READ, 0, SIZE,
                           0, nullptr, nullptr, &error));
    ASSERT_SUCCESS(error);
    for (int i = 0; i < SIZE; ++i) {
      ASSERT_NE(data[i], 42);
    }
    ASSERT_SUCCESS(clEnqueueUnmapMemObject(command_queue, outMem, data, 0,
                                           nullptr, nullptr));
  };

  // One dimensional.
  const size_t ndrange1 = 0;
  EXPECT_NO_FATAL_FAILURE(check_ndrange(1, &ndrange1));

  // Two dimensional.
  const std::vector<std::array<size_t, 2>> ndrange2 = {{0, 0}, {1, 0}, {0, 1}};
  for (const auto &ndrange : ndrange2) {
    EXPECT_NO_FATAL_FAILURE(check_ndrange(2, ndrange.data()));
  }

  // Three dimensional.
  const std::vector<std::array<size_t, 3>> ndrange3 = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
      {0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
  for (const auto &ndrange : ndrange3) {
    EXPECT_NO_FATAL_FAILURE(check_ndrange(3, ndrange.data()));
  }
}
#endif

class clEnqueueNDRangeKernelByValStructTest : public ucl::CommandQueueTest {
 protected:
  enum { NUM = 64 };

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem inMem = nullptr;
  cl_mem outMem = nullptr;
};

TEST_F(clEnqueueNDRangeKernelByValStructTest, Default) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "      typedef struct _my_struct\n"
      "      {\n"
      "        int foo;\n"
      "        int bar;\n"
      "        int gee;\n"
      "      } my_struct;\n"
      "\n"
      "      void kernel byval_kernel(__global int * in, my_struct my_str) {\n"
      "        const int idx = get_global_id(0);\n"
      "        in[idx] = (idx * my_str.foo) + (my_str.bar * my_str.gee);\n"
      "      }\n";

  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  kernel = clCreateKernel(program, "byval_kernel", &errorcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errorcode);

  outMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &errorcode);
  EXPECT_TRUE(outMem);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outMem));

  typedef struct _my_struct {
    int foo;
    int bar;
    int gee;
  } my_struct;

  my_struct ms = {2, 1, 2};

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(my_struct), &ms));

  const size_t global_size = NUM;
  cl_event event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        &event));

  UCL::vector<int> res(NUM);

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, CL_TRUE, 0,
                                     NUM * sizeof(cl_int), res.data(), 1,
                                     &event, nullptr));

  for (int i = 0, e = NUM; i < e; ++i) {
    ASSERT_EQ(res[i], (i * 2) + 2);
  }

  EXPECT_SUCCESS(clReleaseMemObject(outMem));
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

/* Redmine #5144: CL_INVALID_PROGRAM_EXECUTABLE if there is no successfully
   built program executable available for device associated with command_queue.
   Can only be hit when we have multiple devices (as we would have to build for
   one device only, get a kernel of that device, then try and run it with the
   other devices command queue). */

/* Redmine #5116: Check CL_INVALID_IMAGE_SIZE if an image object is specified as
  an argument value and the image dimensions (image width, height, specified or
  compute row and/or slice pitch) are not supported by device associated with
  queue. CL_INVALID_IMAGE_FORMAT if an image object is specified as an argument
  value and the image format (image channel order and data type) is not
  supported by device associated with queue. Only when we support images. */

// Redmine #5144: test local work sizes

/*
    CL_MISALIGNED_SUB_BUFFER_OFFSET if a sub-buffer object is specified as the
   value for an argument that is a buffer object and the offset specified when
   the sub-buffer object is created is not aligned to
   CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue.

    CL_OUT_OF_RESOURCES if there is a failure to queue the execution instance of
   kernel on the command-queue because of insufficient resources needed to
   execute the kernel. For example, the explicitly specified local_work_size
   causes a failure to execute the kernel because of insufficient resources such
   as registers or local memory. Another example would be the number of
   read-only image args used in kernel exceed the CL_DEVICE_MAX_READ_IMAGE_ARGS
   value for device or the number of write-only image args used in kernel exceed
   the CL_DEVICE_MAX_WRITE_IMAGE_ARGS value for device or the number of samplers
   used in kernel exceed CL_DEVICE_MAX_SAMPLERS for device.
    CL_MEM_OBJECT_ALLOCATION_FAILURE if there is a failure to allocate memory
   for data store associated with image or buffer objects specified as arguments
   to kernel.
    CL_OUT_OF_RESOURCES if there is a failure to allocate resources required by
   the OpenCL implementation on the device.
    CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required
   by the OpenCL implementation on the host.
*/

class clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest
    : public ucl::CommandQueueTest {
 protected:
  enum { SIZE = 128 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    max_work_group_size = getDeviceMaxWorkGroupSize();
    max_work_item_sizes = getDeviceMaxWorkItemSizes();
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  void SetUpProgram(const char *source) {
    cl_int errorcode = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);
  }

  bool isValidWorkGroupSize(
      const std::array<size_t, 3> &work_group_size) const {
    return work_group_size[0] <= max_work_item_sizes[0] &&
           work_group_size[1] <= max_work_item_sizes[1] &&
           work_group_size[2] <= max_work_item_sizes[2] &&
           (work_group_size[0] * work_group_size[1] * work_group_size[2] <=
            max_work_group_size);
  }

  size_t max_work_group_size = 0;
  std::vector<size_t> max_work_item_sizes = {};

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       ThreeDimensionsNoAttribute) {
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const char *source = "kernel void foo() {int a = 42;}";
  SetUpProgram(source);
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest, ThreeDimensions) {
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const size_t local_size[3] = {1, 1, 1};
  const char *source =
      "kernel void __attribute__((reqd_work_group_size(1, 1, 1)))"
      "foo() {int a = 42;}";
  SetUpProgram(source);
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, &local_size[0], 0, nullptr,
                                        nullptr));
}

// CL_INVALID_WORK_GROUP_SIZE if local_work_size is nullptr and the
// __attribute__ (( reqd_work_group_size(X, Y, Z))) qualifier
// is used to declare the work-group size for kernel in the program source.
TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       LocalWorkSizeNotSpecifiedThreeDimensions) {
  if (!isValidWorkGroupSize({7, 8, 9})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const char *source =
      "kernel void __attribute__((reqd_work_group_size(7, 8, 9)))"
      "foo() {int a = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr, sizes, nullptr,
                             0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       LocalWorkSizeNotSpecifiedTwoDimensions) {
  if (!isValidWorkGroupSize({7, 8, 9})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, global_size / 2};
  const char *source =
      "kernel void __attribute__((reqd_work_group_size(7, 8, 9)))"
      "foo() {int a = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 2, nullptr, sizes, nullptr,
                             0, nullptr, nullptr));
}

// CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of
// work-items specified by global_work_size is not evenly divisable by size of
// work-group given by local_work_size or does not match the work-group size
// specified for kernel using the _ _attribute__ ((reqd_work_group_size(X, Y,
// Z))) qualifier in program source.
TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       ReqdWorkGroupNotMatchingLocal) {
  if (!isValidWorkGroupSize({7, 11, 13})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const size_t local_size[3] = {1, 1, 1};
  const char *source =
      "kernel void __attribute__((reqd_work_group_size(7, 11, 13)))"
      "foo() {int a = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr, sizes,
                             &local_size[0], 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       TwoKernelsThreeDimensions) {
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const size_t local_size[3] = {1, 1, 1};
  const char *source =
      "kernel void not_the_one() {int a = 42;}"
      "kernel void __attribute__((reqd_work_group_size(1, 1, 1)))"
      "foo() {int b = 42;}";
  SetUpProgram(source);
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, &local_size[0], 0, nullptr,
                                        nullptr));
}

// CL_INVALID_WORK_GROUP_SIZE if local_work_size is nullptr and the
// __attribute__ (( reqd_work_group_size(X, Y, Z))) qualifier
// is used to declare the work-group size for kernel in the program source.
TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       TwoKernelsLocalWorkSizeNotSpecified) {
  if (!isValidWorkGroupSize({7, 8, 9})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const char *source =
      "kernel void not_the_one() {int a = 42;}"
      "kernel void __attribute__((reqd_work_group_size(7, 8, 9)))"
      "foo() {int b = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr, sizes, nullptr,
                             0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       TwoKernelsLocalWorkSizeNotSpecifiedTwoDimensions) {
  if (!isValidWorkGroupSize({7, 8, 9})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, global_size / 2};
  const char *source =
      "kernel void not_the_one() {int a = 42;}"
      "kernel void __attribute__((reqd_work_group_size(7, 8, 9)))"
      "foo() {int b = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 2, nullptr, sizes, nullptr,
                             0, nullptr, nullptr));
}

// CL_INVALID_WORK_GROUP_SIZE if local_work_size is specified and number of
// work-items
// specified by global_work_size is not evenly divisable by size of work-group
// given by
// local_work_size or does not match the work-group size specified for kernel
// using the _
// _attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier in program source.
TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       TwoKernelsReqdWorkGroupNotMatchingLocal) {
  if (!isValidWorkGroupSize({7, 11, 13})) {
    GTEST_SKIP();
  }
  const size_t global_size = SIZE / sizeof(cl_int);
  const size_t sizes[] = {2, 2, global_size / 4};
  const size_t local_size[3] = {1, 1, 1};
  const char *source =
      "kernel void not_the_one() {int a = 42;}"
      "kernel void __attribute__((reqd_work_group_size(7, 11, 13)))"
      "foo() {int b = 42;}";
  SetUpProgram(source);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_WORK_GROUP_SIZE,
      clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr, sizes,
                             &local_size[0], 0, nullptr, nullptr));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest, ReqdWorkGroupSize) {
  const std::array<size_t, 3> local_size{13, 3, 5};
  if (!isValidWorkGroupSize(local_size)) {
    GTEST_SKIP();
  }
  const size_t size = local_size[0] * local_size[1] * local_size[2];
  const size_t sizes[3] = {size, size, size};

  const char *source =
      "kernel void foo(__global uint* out)"
      "    __attribute__((reqd_work_group_size(13, 3, 5))) {\n"
      "  const size_t xId = get_global_id(0);\n"
      "  const size_t yId = get_global_id(1);\n"
      "  const size_t zId = get_global_id(2);\n"
      "  const size_t id = xId + (get_global_size(0) * yId) +"
      "    (get_global_size(0) * get_global_size(1) * zId);\n"
      "  out[(id * 3) + 0] = get_local_size(0);\n"
      "  out[(id * 3) + 1] = get_local_size(1);\n"
      "  out[(id * 3) + 2] = get_local_size(2);\n"
      "}";

  SetUpProgram(source);

  if (!UCL::hasLocalWorkSizeSupport(device, 3, local_size.data())) {
    return;
  }

  cl_int errorcode;

  // Storing 3 values per work item.
  const size_t mem_size = 3 * sizeof(cl_uint) * size * size * size;
  cl_mem mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, local_size.data(), 0, nullptr,
                                        &kernel_event));

  cl_uint *const infos = reinterpret_cast<cl_uint *>(
      clEnqueueMapBuffer(command_queue, mem, false, CL_MAP_READ, 0, mem_size, 0,
                         nullptr, nullptr, &errorcode));
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  ASSERT_SUCCESS(clReleaseEvent(kernel_event));

  for (size_t i = 0, e = mem_size / sizeof(cl_uint); i < e; i += 3) {
    ASSERT_EQ(local_size[0], infos[i + 0]);
    ASSERT_EQ(local_size[1], infos[i + 1]);
    ASSERT_EQ(local_size[2], infos[i + 2]);
  }

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, mem, infos, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));

  ASSERT_SUCCESS(clReleaseMemObject(mem));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       ReqdWorkGroupSizeLocalSizeNull) {
  const std::array<size_t, 3> local_size{13, 3, 5};
  if (!isValidWorkGroupSize(local_size)) {
    GTEST_SKIP();
  }
  const size_t size = local_size[0] * local_size[1] * local_size[2];
  const size_t sizes[3] = {size, size, size};

  const char *source =
      "kernel void foo(__global uint* out)"
      "    __attribute__((reqd_work_group_size(13, 3, 5))) {\n"
      "  const size_t xId = get_global_id(0);\n"
      "  const size_t yId = get_global_id(1);\n"
      "  const size_t zId = get_global_id(2);\n"
      "  const size_t id = xId + (get_global_size(0) * yId) +"
      "    (get_global_size(0) * get_global_size(1) * zId);\n"
      "  out[(id * 3) + 0] = get_local_size(0);\n"
      "  out[(id * 3) + 1] = get_local_size(1);\n"
      "  out[(id * 3) + 2] = get_local_size(2);\n"
      "}";

  SetUpProgram(source);

  // local_size isn't passed to clEnqueueNDRangeKernel, but the kernel still
  // has a reqd_work_group_size, so check that it is supported.
  if (!UCL::hasLocalWorkSizeSupport(device, 3, local_size.data())) {
    return;
  }

  cl_int errorcode;

  // Storing 3 values per work item.
  const size_t mem_size = 3 * sizeof(cl_uint) * size * size * size;
  cl_mem mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, nullptr, 0, nullptr,
                                        &kernel_event));

  cl_uint *const infos = reinterpret_cast<cl_uint *>(
      clEnqueueMapBuffer(command_queue, mem, false, CL_MAP_READ, 0, mem_size, 0,
                         nullptr, nullptr, &errorcode));
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  EXPECT_SUCCESS(clReleaseEvent(kernel_event));

  for (size_t i = 0, e = mem_size / sizeof(cl_uint); i < e; i += 3) {
    ASSERT_EQ(local_size[0], infos[i + 0]);
    ASSERT_EQ(local_size[1], infos[i + 1]);
    ASSERT_EQ(local_size[2], infos[i + 2]);
  }

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, mem, infos, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));

  ASSERT_SUCCESS(clReleaseMemObject(mem));
}

TEST_F(clEnqueueNDRangeKernelWithReqdWorkGroupSizeTest,
       ReqdWorkGroupSizeWithBarriers) {
  const std::array<size_t, 3> local_size{13, 3, 5};
  if (!isValidWorkGroupSize(local_size)) {
    GTEST_SKIP();
  }
  const size_t size = local_size[0] * local_size[1] * local_size[2];
  const size_t sizes[3] = {size, size, size};

  const char *source =
      "kernel void foo(__global uint* out)"
      "    __attribute__((reqd_work_group_size(13, 3, 5))) {\n"
      "  local int tmp[3];\n"
      "  const size_t xId = get_global_id(0);\n"
      "  const size_t yId = get_global_id(1);\n"
      "  const size_t zId = get_global_id(2);\n"
      "  const size_t id = xId + (get_global_size(0) * yId) +"
      "    (get_global_size(0) * get_global_size(1) * zId);\n"
      "  if ((get_local_id(0) + get_local_id(1) + get_local_id(2)) == 0) {\n"
      "    tmp[0] = get_local_size(0);\n"
      "    tmp[1] = get_local_size(1);\n"
      "    tmp[2] = get_local_size(2);\n"
      "  }\n"
      "  barrier(CLK_LOCAL_MEM_FENCE);\n"
      "  out[(id * 3) + 0] = tmp[0];\n"
      "  out[(id * 3) + 1] = tmp[1];\n"
      "  out[(id * 3) + 2] = tmp[2];\n"
      "}";

  SetUpProgram(source);

  if (!UCL::hasLocalWorkSizeSupport(device, 3, local_size.data())) {
    return;
  }

  cl_int errorcode;

  // Storing 3 values per work item.
  const size_t mem_size = 3 * sizeof(cl_uint) * size * size * size;
  cl_mem mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                        sizes, local_size.data(), 0, nullptr,
                                        &kernel_event));

  cl_uint *const infos = reinterpret_cast<cl_uint *>(
      clEnqueueMapBuffer(command_queue, mem, false, CL_MAP_READ, 0, mem_size, 0,
                         nullptr, nullptr, &errorcode));
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  ASSERT_SUCCESS(clReleaseEvent(kernel_event));

  for (size_t i = 0, e = mem_size / sizeof(cl_uint); i < e; i += 3) {
    ASSERT_EQ(local_size[0], infos[i + 0]);
    ASSERT_EQ(local_size[1], infos[i + 1]);
    ASSERT_EQ(local_size[2], infos[i + 2]);
  }

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, mem, infos, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));

  ASSERT_SUCCESS(clReleaseMemObject(mem));
}

struct NDRangeValue {
  cl_uint work_dim;
  size_t *global_work_offset;
  size_t *global_work_size;
  size_t *local_work_size;

  NDRangeValue(const NDRangeValue &other)
      : work_dim(other.work_dim),
        global_work_offset(other.global_work_offset ? new size_t[other.work_dim]
                                                    : nullptr),
        global_work_size(other.global_work_size ? new size_t[other.work_dim]
                                                : nullptr),
        local_work_size(other.local_work_size ? new size_t[other.work_dim]
                                              : nullptr) {
    const size_t length = sizeof(size_t) * work_dim;
    if (other.global_work_offset) {
      memcpy(global_work_offset, other.global_work_offset, length);
    }

    if (other.global_work_size) {
      memcpy(global_work_size, other.global_work_size, length);
    }

    if (other.local_work_size) {
      memcpy(local_work_size, other.local_work_size, length);
    }
  }

  NDRangeValue &operator=(const NDRangeValue &other) {
    if (this == &other) {
      return *this;
    }

    work_dim = other.work_dim;
    global_work_offset =
        (other.global_work_offset) ? new size_t[other.work_dim] : nullptr;
    global_work_size =
        (other.global_work_size) ? new size_t[other.work_dim] : nullptr;
    local_work_size =
        (other.local_work_size) ? new size_t[other.work_dim] : nullptr;

    const size_t length = sizeof(size_t) * work_dim;
    if (other.global_work_offset) {
      memcpy(global_work_offset, other.global_work_offset, length);
    }

    if (other.global_work_size) {
      memcpy(global_work_size, other.global_work_size, length);
    }

    if (other.local_work_size) {
      memcpy(local_work_size, other.local_work_size, length);
    }

    return *this;
  }

  NDRangeValue(const cl_uint _work_dim, const size_t *const _global_work_offset,
               const size_t *const _global_work_size,
               const size_t *const _local_work_size)
      : work_dim(_work_dim),
        global_work_offset(_global_work_offset ? new size_t[_work_dim]
                                               : nullptr),
        global_work_size(_global_work_size ? new size_t[_work_dim] : nullptr),
        local_work_size(_local_work_size ? new size_t[_work_dim] : nullptr) {
    const size_t length = sizeof(size_t) * work_dim;
    if (_global_work_offset) {
      memcpy(global_work_offset, _global_work_offset, length);
    }

    if (_global_work_size) {
      memcpy(global_work_size, _global_work_size, length);
    }

    if (_local_work_size) {
      memcpy(local_work_size, _local_work_size, length);
    }
  }

  ~NDRangeValue() {
    if (global_work_offset) {
      delete[] global_work_offset;
    }

    if (global_work_size) {
      delete[] global_work_size;
    }

    if (local_work_size) {
      delete[] local_work_size;
    }
  }
};

static std::ostream &operator<<(std::ostream &out,
                                const NDRangeValue &nd_range) {
  out << "NDRangeValue{";
  out << ".work dimensions{" << nd_range.work_dim << "}";
  out << ".global work offset{";
  if (!nd_range.global_work_offset) {
    out << "{0, 0, 0}";
  } else {
    out << "{";
    for (cl_uint i = 0u; i < 3u; ++i) {
      out << (nd_range.work_dim > i ? nd_range.global_work_offset[i] : 0)
          << (i < 2 ? "," : "");
      if (i < 2u) {
        out << " ";
      }
    }
    out << "}";
  }
  out << ", .global work size{";
  if (!nd_range.global_work_size) {
    out << "{0, 0, 0}";
  } else {
    out << "{";
    for (cl_uint i = 0u; i < 3u; ++i) {
      out << (nd_range.work_dim > i ? nd_range.global_work_size[i] : 0)
          << (i < 2 ? "," : "");
      if (i < 2u) {
        out << " ";
      }
    }
    out << "}";
  }

  out << "  local work size{";
  if (!nd_range.local_work_size) {
    out << "{0, 0, 0}";
  } else {
    out << "{";
    for (cl_uint i = 0u; i < 3u; ++i) {
      out << (nd_range.work_dim > i ? nd_range.local_work_size[i] : 0)
          << (i < 2 ? "," : "");
      if (i < 2u) {
        out << " ";
      }
    }
    out << "}";
  }
  out << "}";
  return out;
}

struct clEnqueueNDRangeKernelWorkItemTest
    : ucl::CommandQueueTest,
      testing::WithParamInterface<NDRangeValue> {
#pragma pack(4)
  struct PerItemKernelInfo {
    // has to match the one in the kernel source
    cl_ulong4 global_size;
    cl_ulong4 global_id;
    cl_ulong4 local_size;
    cl_ulong4 local_id;
    cl_ulong4 num_groups;
    cl_ulong4 group_id;
    cl_ulong4 global_offset;
    cl_uint work_dim;
  };

  enum { NUM_DIMENSIONS = 3, DEFAULT_DIMENSION_LENGTH = 128 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    cl_ulong max_mem = getDeviceMaxMemAllocSize();

    // NOTE: To avoid allocating to much memory on devices sharing resources
    // with other applications (such as parallel testing), be conservative
    // about the buffer max_mem size.
    max_mem /= 8;

    const cl_ulong items = max_mem / sizeof(PerItemKernelInfo);
    const size_t possible_dimension_length = static_cast<size_t>(
        std::floor(std::pow(static_cast<double>(items), 1.0 / NUM_DIMENSIONS)));
    dimension_length = std::min(possible_dimension_length,
                                static_cast<size_t>(DEFAULT_DIMENSION_LENGTH));
    mem_size = sizeof(PerItemKernelInfo);
    for (int i = 0; i < NUM_DIMENSIONS; ++i) {
      mem_size *= dimension_length;
    }

    cl_int errorcode;
    mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
    // NOTE: If buffer creation fails, reduce the size.
    while (CL_MEM_OBJECT_ALLOCATION_FAILURE == errorcode ||
           CL_OUT_OF_RESOURCES == errorcode) {
      dimension_length /= 2;
      mem_size /= 8;
      mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
    }
    ASSERT_TRUE(mem);
    ASSERT_SUCCESS(errorcode);

    const char *source =
        "struct __attribute__ ((packed)) PerItemKernelInfo {\n"
        "  ulong4 global_size;\n"
        "  ulong4 global_id;\n"
        "  ulong4 local_size;\n"
        "  ulong4 local_id;\n"
        "  ulong4 num_groups;\n"
        "  ulong4 group_id;\n"
        "  ulong4 global_offset;\n"
        "  uint work_dim;\n"
        "};\n"
        "void kernel foo(global struct PerItemKernelInfo * info) {\n"
        "  size_t xId = get_global_id(0);\n"
        "  size_t yId = get_global_id(1);\n"
        "  size_t zId = get_global_id(2);\n"
        "  size_t id = xId + (get_global_size(0) * yId) +\n"
        "               (get_global_size(0) * get_global_size(1) * zId);\n"
        "  info[id].global_size = (ulong4)(get_global_size(0),\n"
        "                                  get_global_size(1),\n"
        "                                  get_global_size(2),\n"
        "                                  get_global_size(3));\n"
        "  info[id].global_id = (ulong4)(get_global_id(0),\n"
        "                                get_global_id(1),\n"
        "                                get_global_id(2),\n"
        "                                get_global_id(3));\n"
        "  info[id].local_size = (ulong4)(get_local_size(0),\n"
        "                                 get_local_size(1),\n"
        "                                 get_local_size(2),\n"
        "                                 get_local_size(3));\n"
        "  info[id].local_id = (ulong4)(get_local_id(0),\n"
        "                               get_local_id(1),\n"
        "                               get_local_id(2),\n"
        "                               get_local_id(3));\n"
        "  info[id].num_groups = (ulong4)(get_num_groups(0),\n"
        "                                 get_num_groups(1),\n"
        "                                 get_num_groups(2),\n"
        "                                 get_num_groups(3));\n"
        "  info[id].group_id = (ulong4)(get_group_id(0),\n"
        "                               get_group_id(1),\n"
        "                               get_group_id(2), get_group_id(3));\n"
        "  info[id].global_offset = (ulong4)(get_global_offset(0),\n"
        "                                    get_global_offset(1),\n"
        "                                    get_global_offset(2),\n"
        "                                    get_global_offset(3));\n"
        "  info[id].work_dim = get_work_dim();\n"
        "}\n";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem));
  }

  void TearDown() override {
    if (mem) {
      EXPECT_SUCCESS(clReleaseMemObject(mem));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  size_t dimension_length = 0;
  size_t mem_size = 0;
  cl_mem mem = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_P(clEnqueueNDRangeKernelWorkItemTest, Default) {
  const NDRangeValue val = GetParam();
  cl_event fillEvent, ndRangeEvent;

  if (nullptr == val.global_work_size) {
    // This exists to convince static analysis that this path is unreachable.
    FAIL() << "No global work size specified";
  }

  // Adjust work sizes to ensure we are within the bounds of the memory object,
  // whose size depends on the maximal available memory.
  for (int i = 0; i < NUM_DIMENSIONS; ++i) {
    val.global_work_size[i] =
        std::min(val.global_work_size[i], dimension_length);
  }

  if (!UCL::hasLocalWorkSizeSupport(device, NUM_DIMENSIONS,
                                    val.local_work_size)) {
    return;
  }

  const char pattern = 0;

  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, mem, &pattern,
                                     sizeof(pattern), 0, mem_size, 0, nullptr,
                                     &fillEvent));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      command_queue, kernel, NUM_DIMENSIONS, val.global_work_offset,
      val.global_work_size, val.local_work_size, 1, &fillEvent, &ndRangeEvent));

  cl_int errorcode = !CL_SUCCESS;
  PerItemKernelInfo *const infos = reinterpret_cast<PerItemKernelInfo *>(
      clEnqueueMapBuffer(command_queue, mem, true, CL_MAP_READ, 0, mem_size, 1,
                         &ndRangeEvent, nullptr, &errorcode));
  ASSERT_TRUE(infos);
  ASSERT_SUCCESS(errorcode);

  for (cl_uint x = 0; x < val.global_work_size[0]; x++) {
    for (cl_uint y = 0; y < val.global_work_size[1]; y++) {
      for (cl_uint z = 0; z < val.global_work_size[2]; z++) {
        // Copy the PerItemKernelInfo, as the version in the buffer is not
        // guaranteed to match the C++ alignment requirements of all the
        // struct's members (i.e. cl_ulong4).
        PerItemKernelInfo info;
        memcpy(&info,
               &infos[x + (y * val.global_work_size[0]) +
                      (z * val.global_work_size[0] * val.global_work_size[1])],
               sizeof(PerItemKernelInfo));

        if (val.global_work_offset && ((x < val.global_work_offset[0]) ||
                                       (y < val.global_work_offset[1]) ||
                                       (z < val.global_work_offset[2]))) {
          ASSERT_EQ(0u, info.global_size.s[0]);
          ASSERT_EQ(0u, info.global_size.s[1]);
          ASSERT_EQ(0u, info.global_size.s[2]);
          ASSERT_EQ(0u, info.global_size.s[3]);

          ASSERT_EQ(0u, info.global_id.s[0]);
          ASSERT_EQ(0u, info.global_id.s[1]);
          ASSERT_EQ(0u, info.global_id.s[2]);
          ASSERT_EQ(0u, info.global_id.s[3]);

          ASSERT_EQ(0u, info.local_size.s[0]);
          ASSERT_EQ(0u, info.local_size.s[1]);
          ASSERT_EQ(0u, info.local_size.s[2]);
          ASSERT_EQ(0u, info.local_size.s[3]);

          ASSERT_EQ(0u, info.local_id.s[0]);
          ASSERT_EQ(0u, info.local_id.s[1]);
          ASSERT_EQ(0u, info.local_id.s[2]);
          ASSERT_EQ(0u, info.local_id.s[3]);

          ASSERT_EQ(0u, info.num_groups.s[0]);
          ASSERT_EQ(0u, info.num_groups.s[1]);
          ASSERT_EQ(0u, info.num_groups.s[2]);
          ASSERT_EQ(0u, info.num_groups.s[3]);

          ASSERT_EQ(0u, info.group_id.s[0]);
          ASSERT_EQ(0u, info.group_id.s[1]);
          ASSERT_EQ(0u, info.group_id.s[2]);
          ASSERT_EQ(0u, info.group_id.s[3]);

          ASSERT_EQ(0u, info.global_offset.s[0]);
          ASSERT_EQ(0u, info.global_offset.s[1]);
          ASSERT_EQ(0u, info.global_offset.s[2]);
          ASSERT_EQ(0u, info.global_offset.s[3]);

          ASSERT_EQ(0u, info.work_dim);
        } else {
          ASSERT_EQ(val.global_work_size[0], info.global_size.s[0]);
          ASSERT_EQ(val.global_work_size[1], info.global_size.s[1]);
          ASSERT_EQ(val.global_work_size[2], info.global_size.s[2]);
          ASSERT_EQ(1u, info.global_size.s[3]);

          ASSERT_EQ(x, info.global_id.s[0]);
          ASSERT_EQ(y, info.global_id.s[1]);
          ASSERT_EQ(z, info.global_id.s[2]);
          ASSERT_EQ(0u, info.global_id.s[3]);

          ASSERT_EQ(1u, info.local_size.s[0]);
          ASSERT_EQ(1u, info.local_size.s[1]);
          ASSERT_EQ(1u, info.local_size.s[2]);
          ASSERT_EQ(1u, info.local_size.s[3]);

          ASSERT_EQ(0u, info.local_id.s[0]);
          ASSERT_EQ(0u, info.local_id.s[1]);
          ASSERT_EQ(0u, info.local_id.s[2]);
          ASSERT_EQ(0u, info.local_id.s[3]);

          ASSERT_EQ(val.global_work_size[0], info.num_groups.s[0]);
          ASSERT_EQ(val.global_work_size[1], info.num_groups.s[1]);
          ASSERT_EQ(val.global_work_size[2], info.num_groups.s[2]);
          ASSERT_EQ(1u, info.num_groups.s[3]);

          ASSERT_EQ(x, info.group_id.s[0]);
          ASSERT_EQ(y, info.group_id.s[1]);
          ASSERT_EQ(z, info.group_id.s[2]);
          ASSERT_EQ(0u, info.group_id.s[3]);

          if (val.global_work_offset) {
            ASSERT_EQ(val.global_work_offset[0], info.global_offset.s[0]);
            ASSERT_EQ(val.global_work_offset[1], info.global_offset.s[1]);
            ASSERT_EQ(val.global_work_offset[2], info.global_offset.s[2]);
            ASSERT_EQ(0u, info.global_offset.s[3]);
          } else {
            ASSERT_EQ(0u, info.global_offset.s[0]);
            ASSERT_EQ(0u, info.global_offset.s[1]);
            ASSERT_EQ(0u, info.global_offset.s[2]);
            ASSERT_EQ(0u, info.global_offset.s[3]);
          }

          ASSERT_EQ(NUM_DIMENSIONS, info.work_dim);
        }
      }
    }
  }

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, mem, infos, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(fillEvent));
  ASSERT_SUCCESS(clReleaseEvent(ndRangeEvent));
}

namespace {
const size_t t0[] = {1, 1, 1};
const size_t t1[] = {
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH, 1, 1};
const size_t t2[] = {
    1, clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH, 1};
const size_t t3[] = {
    1, 1, clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH};
const size_t t4[] = {
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH,
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH, 1};
const size_t t5[] = {
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH, 1,
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH};
const size_t t6[] = {
    1, clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH,
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH};
const size_t t7[] = {
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH,
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH,
    clEnqueueNDRangeKernelWorkItemTest::DEFAULT_DIMENSION_LENGTH};

const size_t offsets[][3] = {
    {1, 1, 1}, {1, 1, 0}, {1, 0, 1}, {1, 0, 0},
    {0, 1, 1}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0},
};
}  // namespace

INSTANTIATE_TEST_CASE_P(VariousNDRangeValues,
                        clEnqueueNDRangeKernelWorkItemTest,
                        ::testing::Values(NDRangeValue(3, nullptr, t0, t0),
                                          NDRangeValue(3, nullptr, t1, t0),
                                          NDRangeValue(3, nullptr, t2, t0),
                                          NDRangeValue(3, nullptr, t3, t0),
                                          NDRangeValue(3, nullptr, t4, t0),
                                          NDRangeValue(3, nullptr, t5, t0),
                                          NDRangeValue(3, nullptr, t6, t0),
                                          NDRangeValue(3, nullptr, t7, t0),
                                          NDRangeValue(3, offsets[0], t0, t0),
                                          NDRangeValue(3, offsets[1], t0, t0),
                                          NDRangeValue(3, offsets[2], t0, t0),
                                          NDRangeValue(3, offsets[3], t0, t0),
                                          NDRangeValue(3, offsets[4], t0, t0),
                                          NDRangeValue(3, offsets[5], t0, t0),
                                          NDRangeValue(3, offsets[6], t0, t0),
                                          NDRangeValue(3, offsets[7], t0, t0)));

struct clEnqueueNDRangeImageTest
    : ucl::CommandQueueTest,
      testing::WithParamInterface<cl_mem_object_type> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!(getDeviceImageSupport() && getDeviceCompilerAvailable())) {
      GTEST_SKIP();
    }
    object_type = GetParam();
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
    if (CL_BUILD_SUCCESS != buildStatus) {
      size_t logLength = 0;
      ASSERT_SUCCESS(clGetProgramBuildInfo(
          program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logLength));
      UCL::vector<char> log(logLength);
      ASSERT_SUCCESS(clGetProgramBuildInfo(program, device,
                                           CL_PROGRAM_BUILD_LOG, logLength,
                                           log.data(), &logLength));
      (void)fprintf(stderr, "%s", log.data());
      ASSERT_TRUE(false);
    }

    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_FLOAT;
    desc.image_type = object_type;
    desc.image_width = 1;
    desc.image_height = 1;
    desc.image_depth = 1;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;

    src_desc.image_type = object_type;
    src_desc.image_width = 1;
    src_desc.image_height = 1;
    src_desc.image_depth = 1;
    src_desc.image_array_size = 1;
    src_desc.image_row_pitch = 0;
    src_desc.image_slice_pitch = 0;
    src_desc.num_mip_levels = 0;
    src_desc.num_samples = 0;
    src_desc.buffer = nullptr;

    switch (object_type) {
      case CL_MEM_OBJECT_IMAGE1D:
        desc.image_width = 16;
        src_desc.image_width = 16;
        kernel = clCreateKernel(program, "img_copy1d", &error);
        break;
      case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        desc.image_width = 16;
        desc.image_array_size = 8;
        src_desc.image_width = 16;
        src_desc.image_array_size = 8;
        kernel = clCreateKernel(program, "img_copy1d_array", &error);
        break;
      case CL_MEM_OBJECT_IMAGE1D_BUFFER:
        desc.image_width = 16;
        src_desc.image_width = 16;
        kernel = clCreateKernel(program, "img_copy1d_buffer", &error);
        break;
      case CL_MEM_OBJECT_IMAGE2D:
        desc.image_width = 16;
        desc.image_height = 16;
        src_desc.image_width = 16;
        src_desc.image_height = 16;
        kernel = clCreateKernel(program, "img_copy2d", &error);
        break;
      case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        desc.image_width = 12;
        desc.image_height = 12;
        desc.image_array_size = 4;
        src_desc.image_width = 12;
        src_desc.image_height = 12;
        src_desc.image_array_size = 4;
        kernel = clCreateKernel(program, "img_copy2d_array", &error);
        break;
      case CL_MEM_OBJECT_IMAGE3D:
        desc.image_width = 8;
        desc.image_height = 8;
        desc.image_depth = 8;
        src_desc.image_width = 8;
        src_desc.image_height = 8;
        src_desc.image_depth = 8;
        kernel = clCreateKernel(program, "img_copy3d", &error);
        break;
      default:
        (void)fprintf(stderr, "unexpected object type %ld\n",
                      (long)object_type);
        ASSERT_TRUE(false);
    }

    if (!UCL::isImageFormatSupported(context,
                                     {CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY},
                                     desc.image_type, format)) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, kernel);

    if (object_type == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
      src_desc.buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                       src_desc.image_width * sizeof(cl_float4),
                                       nullptr, &error);
      ASSERT_SUCCESS(error);
      desc.buffer =
          clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                         desc.image_width * sizeof(cl_float4), nullptr, &error);
      ASSERT_SUCCESS(error);
    }

    src_image = clCreateImage(context, CL_MEM_READ_ONLY, &format, &src_desc,
                              nullptr, &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, src_image);
    dst_image = clCreateImage(context, CL_MEM_WRITE_ONLY, &format, &desc,
                              nullptr, &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(nullptr, dst_image);
    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(src_image),
                                  static_cast<void *>(&src_image)));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(dst_image),
                                  static_cast<void *>(&dst_image)));
  }

  void TearDown() override {
    if (desc.buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(desc.buffer));
    }
    if (src_desc.buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_desc.buffer));
    }
    if (dst_image) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_image));
    }
    if (src_image) {
      EXPECT_SUCCESS(clReleaseMemObject(src_image));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_image_desc desc = {};
  cl_image_desc src_desc = {};
  cl_mem src_image = nullptr;
  cl_mem dst_image = nullptr;
  cl_mem_object_type object_type = 0;
  // TODO CA-4366 Pad the size of this class so that virtually inherited
  // ucl::ContextTest class is aligned to 8 bytes when constructed.
  const cl_int padding = 0;
};

TEST_P(clEnqueueNDRangeImageTest, DefaultCopyImage) {
  size_t numPixels = 1;
  size_t origin[3] = {0, 0, 0};
  size_t region[3] = {desc.image_width, desc.image_height, desc.image_depth};

  switch (object_type) {
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      numPixels = desc.image_width;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      numPixels = desc.image_width * desc.image_array_size;
      region[1] = desc.image_array_size;
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      numPixels = desc.image_width * desc.image_height;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      numPixels = desc.image_width * desc.image_height * desc.image_array_size;
      region[2] = desc.image_array_size;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      numPixels = desc.image_width * desc.image_height * desc.image_depth;
      break;
    default:
      (void)fprintf(stderr, "unexpected object type %ld\n", (long)object_type);
      ASSERT_TRUE(false);
  }

  UCL::vector<cl_float4> srcPixels(numPixels);
  UCL::vector<cl_float4> dstPixels(numPixels);

  for (auto &pixel : srcPixels) {
    for (size_t element = 0; element < 4; element++) {
      pixel.s[element] = (float)element;
    }
  }

  cl_event writeEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueWriteImage(command_queue, src_image, CL_FALSE, origin,
                                     region, 0, 0, srcPixels.data(), 0, nullptr,
                                     &writeEvent));
  ASSERT_NE(nullptr, writeEvent);
  size_t localWorkSize[3] = {1, 1, 1};
  cl_event workEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 3, origin,
                                        region, localWorkSize, 1, &writeEvent,
                                        &workEvent));
  ASSERT_NE(nullptr, workEvent);
  cl_event readEvent = nullptr;
  ASSERT_SUCCESS(clEnqueueReadImage(command_queue, dst_image, CL_FALSE, origin,
                                    region, 0, 0, dstPixels.data(), 1,
                                    &workEvent, &readEvent));
  ASSERT_NE(nullptr, readEvent);
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(workEvent));

  for (size_t pixel = 0; pixel < numPixels; pixel++) {
    for (size_t element = 0; element < 4; element++) {
      ASSERT_EQ(srcPixels[pixel].s[element], dstPixels[pixel].s[element])
          << "At pixel : " << pixel << '\n'
          << "Total : " << numPixels;
    }
  }
  clReleaseEvent(writeEvent);
  clReleaseEvent(workEvent);
  clReleaseEvent(readEvent);
}

INSTANTIATE_TEST_CASE_P(
    Default, clEnqueueNDRangeImageTest,
    ::testing::Values(cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D},
                      cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D_ARRAY},
                      cl_mem_object_type{CL_MEM_OBJECT_IMAGE1D_BUFFER},
                      cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D},
                      cl_mem_object_type{CL_MEM_OBJECT_IMAGE2D_ARRAY},
                      cl_mem_object_type{CL_MEM_OBJECT_IMAGE3D}));

class clEnqueueNDRangeKernelZeroDimension
    : public clEnqueueNDRangeKernelTest,
      public testing::WithParamInterface<size_t> {
 protected:
  virtual void SetUp() {
    clEnqueueNDRangeKernelTest::SetUp();
    // Returning CL_INVALID_GLOBAL_WORK_SIZE for NDRanges with a zero-sized
    // dimension was deprecated in 2.1.
    if (UCL::isDeviceVersionAtLeast({2, 1})) {
      error_code = CL_SUCCESS;
    } else {
      error_code = CL_INVALID_GLOBAL_WORK_SIZE;
    }
  }
  cl_int error_code;

  // TODO CA-4366 Pad the size of this class so that virtually inherited
  // ucl::ContextTest class is aligned to 8 bytes when constructed.
  const cl_int padding = 0;
};

TEST_P(clEnqueueNDRangeKernelZeroDimension, ZeroDimension) {
  size_t dimensions[]{1, 2, 3};
  // Zero out one of the dimensions;
  dimensions[GetParam()] = 0;
  EXPECT_EQ(error_code,
            clEnqueueNDRangeKernel(command_queue, kernel, 3, nullptr,
                                   dimensions, nullptr, 0, nullptr, nullptr));
}

INSTANTIATE_TEST_CASE_P(ZeroDimensions, clEnqueueNDRangeKernelZeroDimension,
                        testing::Values(0, 1, 2));

#if defined(CL_VERSION_2_0)
class LinearIDTest
    : public ucl::CommandQueueTest,
      public testing::WithParamInterface<
          std::tuple<size_t, std::array<size_t, 3>, std::array<size_t, 3>>> {
 public:
  static constexpr std::array<size_t, 3> dimensions{1, 2, 3};
  static constexpr std::array<std::array<size_t, 3>, 2> global_sizes{
      {{128, 128, 128}, {32, 64, 128}}};
  static constexpr std::array<std::array<size_t, 3>, 2> global_offsets{
      {{0, 0, 0}, {1, 2, 3}}};

 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    // get_local_linear_id and get_global_linear_id were
    // introduced in the OpenCL-2.0 spec.
    if (!UCL::isDeviceVersionAtLeast({2, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *get_linear_id_kernels =
        R"(
        __kernel void get_local_linear_id_kernel(__global size_t *output) {

            size_t expected_local_linear_id
                             = (get_local_id(2) * get_local_size(1) *
                               get_local_size(0)) + (get_local_id(1) *
                               get_local_size(0)) + get_local_id(0);

            size_t global_linear_id =
                                (get_global_id(2) - get_global_offset(2)) *
                                get_global_size(1) * get_global_size(0)
                                + (get_global_id(1) - get_global_offset(1)) *
                                get_global_size(0) + (get_global_id(0) -
                                get_global_offset(0));

            output[global_linear_id] = get_local_linear_id() -
                                       expected_local_linear_id;
    }

    __kernel void get_global_linear_id_kernel(__global size_t * output) {
            size_t expected_global_linear_id =
                                (get_global_id(2) - get_global_offset(2)) *
                                get_global_size(1) * get_global_size(0)
                                + (get_global_id(1) - get_global_offset(1)) *
                                get_global_size(0) + (get_global_id(0) -
                                get_global_offset(0));

            output[expected_global_linear_id] = get_global_linear_id() -
                                                expected_global_linear_id;
    }
    )";

    cl_int error_code{};
    program = clCreateProgramWithSource(context, 1, &get_linear_id_kernels,
                                        nullptr, &error_code);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(error_code);
    auto device_version = ucl::Environment::instance->deviceOpenCLVersion;
    const std::string cl_std_option =
        "-cl-std=CL" + std::to_string(device_version.major()) + '.' +
        std::to_string(device_version.minor());
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, cl_std_option.c_str(),
                                  nullptr, nullptr));
  }

  virtual void TearDown() {
    if (nullptr != output_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
    }
    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  void EnqueueShared(size_t work_dim,
                     const std::array<size_t, 3> &global_work_size,
                     const std::array<size_t, 3> &global_work_offset,
                     std::string kernel_name) {
    cl_int error_code{};
    kernel = clCreateKernel(program, kernel_name.c_str(), &error_code);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(error_code);

    size_t global_size = 1;
    for (unsigned i = 0; i < work_dim; ++i) {
      global_size *= global_work_size[i];
    }
    std::vector<size_t> output(global_size, 42);
    const size_t data_size_in_bytes =
        sizeof(decltype(output)::value_type) * output.size();
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                   data_size_in_bytes, nullptr, &error_code);
    EXPECT_TRUE(output_buffer);
    ASSERT_SUCCESS(error_code);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(output_buffer),
                                  static_cast<void *>(&output_buffer)));

    ASSERT_SUCCESS(clEnqueueNDRangeKernel(
        command_queue, kernel, work_dim, global_work_offset.data(),
        global_work_size.data(), nullptr, 0, nullptr, nullptr));
    ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                       data_size_in_bytes, output.data(), 0,
                                       nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));
    int i = 0;
    for (const auto &value : output) {
      ASSERT_EQ(value, 0) << "at index " << i;
      ++i;
    }
  }

  void EnqueueLocal(size_t work_dim,
                    const std::array<size_t, 3> &global_work_size,
                    const std::array<size_t, 3> &global_work_offset) {
    EnqueueShared(work_dim, global_work_size, global_work_offset,
                  "get_local_linear_id_kernel");
  }

  void EnqueueGlobal(size_t work_dim,
                     const std::array<size_t, 3> &global_work_size,
                     const std::array<size_t, 3> &global_work_offset) {
    EnqueueShared(work_dim, global_work_size, global_work_offset,
                  "get_global_linear_id_kernel");
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem output_buffer = nullptr;
};

TEST_P(LinearIDTest, Local) {
  const size_t work_dim = std::get<0>(GetParam());
  const auto &global_work_size = std::get<1>(GetParam());
  const auto &global_work_offset = std::get<2>(GetParam());
  EnqueueLocal(work_dim, global_work_size, global_work_offset);
}

TEST_P(LinearIDTest, Global) {
  const size_t work_dim = std::get<0>(GetParam());
  const auto &global_work_size = std::get<1>(GetParam());
  const auto &global_work_offset = std::get<2>(GetParam());
  EnqueueGlobal(work_dim, global_work_size, global_work_offset);
}

INSTANTIATE_TEST_CASE_P(
    Dimensions, LinearIDTest,
    testing::Combine(testing::ValuesIn(LinearIDTest::dimensions),
                     testing::ValuesIn(LinearIDTest::global_sizes),
                     testing::ValuesIn(LinearIDTest::global_offsets)));
#endif

class GetEnqueuedLocalSizeTest : public ucl::CommandQueueTest {
 protected:
  virtual void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    // get_enqueued_local_size was introduced for non-uniform workgroups
    // in OpenCL-2.0.
    if (!UCL::isDeviceVersionAtLeast({2, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *get_enqueued_local_size_kernels =
        R"(
          __kernel void get_enqueued_local_size_kernel(__global uint3 *out) {
            if (0 == get_local_id(0) && 0 == get_local_id(1) &&
                0 == get_local_id(2)) {
                uint linear_group_id = get_group_id(0) +
                                       get_num_groups(0) * get_group_id(1) +
                                       get_num_groups(0) * get_num_groups(1) *
                                       get_group_id(2);

                out[linear_group_id].x = get_enqueued_local_size(0);
                out[linear_group_id].y = get_enqueued_local_size(1);
                out[linear_group_id].z = get_enqueued_local_size(2);
              }
            }
           )";
    cl_int error_code{};
    program = clCreateProgramWithSource(
        context, 1, &get_enqueued_local_size_kernels, nullptr, &error_code);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(error_code);
    auto device_version = ucl::Environment::instance->deviceOpenCLVersion;
    const std::string cl_std_option =
        "-cl-std=CL" + std::to_string(device_version.major()) + '.' +
        std::to_string(device_version.minor());
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, cl_std_option.c_str(),
                                  nullptr, nullptr));
  }

  virtual void TearDown() override {
    if (nullptr != output_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
    }
    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program{nullptr};
  cl_kernel kernel{nullptr};
  cl_mem output_buffer{nullptr};
};

TEST_F(GetEnqueuedLocalSizeTest, Uniform1D) {
  cl_int error_code{};
  kernel =
      clCreateKernel(program, "get_enqueued_local_size_kernel", &error_code);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(error_code);
  // We need one element in the output for each workgroup.
  // This assumes uniform workgroups.
  const std::array<size_t, 1> global_work_size{16};
  const std::array<size_t, 1> local_work_size{2};
  const auto number_of_workgroups_x = global_work_size[0] / local_work_size[0];

  const auto number_of_workgroups = number_of_workgroups_x;

  UCL::AlignedBuffer<cl_uint3> output{number_of_workgroups};

  const auto data_size_in_bytes =
      sizeof(decltype(output)::value_type) * output.size();
  output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, data_size_in_bytes,
                                 nullptr, &error_code);
  EXPECT_TRUE(output_buffer);
  ASSERT_SUCCESS(error_code);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(output_buffer),
                                static_cast<void *>(&output_buffer)));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      command_queue, kernel, 1, nullptr, global_work_size.data(),
      local_work_size.data(), 0, nullptr, &kernel_event));
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  ASSERT_SUCCESS(clReleaseEvent(kernel_event));
  for (unsigned workgroup = 0; workgroup < number_of_workgroups; ++workgroup) {
    ASSERT_EQ(output[workgroup].x, local_work_size[0])
        << "incorrect local size in x dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].y, 1)
        << "incorrect local size in y dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].z, 1)
        << "incorrect local size in z dimension for workgroup: " << workgroup;
  }
}

TEST_F(GetEnqueuedLocalSizeTest, Uniform2D) {
  cl_int error_code{};
  kernel =
      clCreateKernel(program, "get_enqueued_local_size_kernel", &error_code);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(error_code);
  // We need one element in the output for each workgroup.
  // This assumes uniform workgroups.
  const std::array<size_t, 2> global_work_size{16, 32};
  const std::array<size_t, 2> local_work_size{2, 4};
  const auto number_of_workgroups_x = global_work_size[0] / local_work_size[0];
  const auto number_of_workgroups_y = global_work_size[1] / local_work_size[1];

  const auto number_of_workgroups =
      number_of_workgroups_x * number_of_workgroups_y;

  UCL::AlignedBuffer<cl_uint3> output{number_of_workgroups};

  const auto data_size_in_bytes =
      sizeof(decltype(output)::value_type) * output.size();
  output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, data_size_in_bytes,
                                 nullptr, &error_code);
  EXPECT_TRUE(output_buffer);
  ASSERT_SUCCESS(error_code);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(output_buffer),
                                static_cast<void *>(&output_buffer)));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      command_queue, kernel, 2, nullptr, global_work_size.data(),
      local_work_size.data(), 0, nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, &kernel_event));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  ASSERT_SUCCESS(clReleaseEvent(kernel_event));
  for (unsigned workgroup = 0; workgroup < number_of_workgroups; ++workgroup) {
    ASSERT_EQ(output[workgroup].x, local_work_size[0])
        << "incorrect local size in x dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].y, local_work_size[1])
        << "incorrect local size in y dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].z, 1)
        << "incorrect local size in z dimension for workgroup: " << workgroup;
  }
}

TEST_F(GetEnqueuedLocalSizeTest, Uniform3D) {
  cl_int error_code{};
  kernel =
      clCreateKernel(program, "get_enqueued_local_size_kernel", &error_code);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(error_code);
  // We need one element in the output for each workgroup.
  // This assumes uniform workgroups.
  const std::array<size_t, 3> global_work_size{16, 32, 64};
  const std::array<size_t, 3> local_work_size{2, 4, 8};
  const auto number_of_workgroups_x = global_work_size[0] / local_work_size[0];
  const auto number_of_workgroups_y = global_work_size[1] / local_work_size[1];
  const auto number_of_workgroups_z = global_work_size[2] / local_work_size[2];

  const auto number_of_workgroups =
      number_of_workgroups_x * number_of_workgroups_y * number_of_workgroups_z;

  UCL::AlignedBuffer<cl_uint3> output{number_of_workgroups};

  const auto data_size_in_bytes =
      sizeof(decltype(output)::value_type) * output.size();
  output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, data_size_in_bytes,
                                 nullptr, &error_code);
  EXPECT_TRUE(output_buffer);
  ASSERT_SUCCESS(error_code);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(output_buffer),
                                static_cast<void *>(&output_buffer)));

  cl_event kernel_event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      command_queue, kernel, 3, nullptr, global_work_size.data(),
      local_work_size.data(), 0, nullptr, &kernel_event));
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(kernel_event));
  ASSERT_SUCCESS(clReleaseEvent(kernel_event));
  for (unsigned workgroup = 0; workgroup < number_of_workgroups; ++workgroup) {
    ASSERT_EQ(output[workgroup].x, local_work_size[0])
        << "incorrect local size in x dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].y, local_work_size[1])
        << "incorrect local size in y dimension for workgroup: " << workgroup;
    ASSERT_EQ(output[workgroup].z, local_work_size[2])
        << "incorrect local size in z dimension for workgroup: " << workgroup;
  }
}
