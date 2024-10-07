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
#include "EventWaitList.h"

class clEnqueueTaskTest : public ucl::CommandQueueTest, TestWithEventWaitList {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "kernel void foo(global int *input, global int *output)"
        "{"
        "  size_t i = get_global_id(0);"
        "  output[i] = input[i];"
        "}";
    const size_t source_length = strlen(source);
    cl_int status;
    program =
        clCreateProgramWithSource(context, 1, &source, &source_length, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);
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

  cl_int SetUpProgram(const char *source) {
    EXPECT_SUCCESS(clReleaseKernel(kernel));
    EXPECT_SUCCESS(clReleaseProgram(program));

    cl_int status = CL_SUCCESS;
    const size_t source_length = strlen(source);
    program =
        clCreateProgramWithSource(context, 1, &source, &source_length, &status);
    EXPECT_TRUE(program);
    UCL_SUCCESS_OR_RETURN_ERR(status);

    UCL_SUCCESS_OR_RETURN_ERR(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    UCL_SUCCESS_OR_RETURN_ERR(status);
    return CL_SUCCESS;
  }

  void EventWaitListAPICall(cl_int err, cl_uint num_events,
                            const cl_event *events, cl_event *event) override {
    const size_t buffer_size = sizeof(cl_int);
    cl_int status;
    cl_mem input_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size,
                                      nullptr, &status);
    EXPECT_TRUE(input_mem);
    ASSERT_SUCCESS(status);

    EXPECT_EQ_ERRCODE(CL_SUCCESS,
                      clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                     static_cast<void *>(&input_mem)));
    cl_mem output_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size,
                                       nullptr, &status);
    EXPECT_TRUE(output_mem);
    ASSERT_SUCCESS(status);

    EXPECT_EQ_ERRCODE(CL_SUCCESS,
                      clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                     static_cast<void *>(&output_mem)));
    ASSERT_EQ_ERRCODE(
        err, clEnqueueTask(command_queue, kernel, num_events, events, event));

    ASSERT_SUCCESS(clReleaseMemObject(input_mem));
    ASSERT_SUCCESS(clReleaseMemObject(output_mem));
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

// Redmine #5145: Check CL_INVALID_PROGRAM_EXECUTABLE

TEST_F(clEnqueueTaskTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE,
                    clEnqueueTask(nullptr, kernel, 0, nullptr, nullptr));
}

TEST_F(clEnqueueTaskTest, InvalidKernel) {
  ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL,
                    clEnqueueTask(command_queue, nullptr, 0, nullptr, nullptr));
}

TEST_F(clEnqueueTaskTest, InvalidKernelArgs) {
  ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL_ARGS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, nullptr));
}

// Redmine #5145: Check CL_INVALID_WORK_GROUP_SIZE
// Redmine #5120: Check Check CL_MISALIGNED_SUB_BUFFER_OFFSET
// Redmine #5116: Check Check CL_INVALID_IMAGE_SIZE
// Redmine #5116: Check Check CL_INVALID_IMAGE_FORMAT
// Redmine #5117: Check CL_OUT_OF_RESOURCES
// Redmine #5123: Check CL_MEM_OBJECT_ALLOCATION_FAILURE

TEST_F(clEnqueueTaskTest, DefaultNoEventWaitList) {
  const size_t buffer_size = sizeof(cl_int);
  cl_int status;
  cl_mem input_mem =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr, &status);
  EXPECT_TRUE(input_mem);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                   static_cast<void *>(&input_mem)));
  cl_mem output_mem =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &status);
  EXPECT_TRUE(output_mem);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                   static_cast<void *>(&output_mem)));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  EXPECT_TRUE(event);

  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseMemObject(output_mem));
  ASSERT_SUCCESS(clReleaseMemObject(input_mem));
}

TEST_F(clEnqueueTaskTest, DefaultWithEventWaitList) {
  const size_t buffer_size = sizeof(cl_int);
  cl_int status;
  cl_mem input_mem =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr, &status);
  EXPECT_TRUE(input_mem);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                   static_cast<void *>(&input_mem)));
  cl_mem output_mem =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &status);
  EXPECT_TRUE(output_mem);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                   static_cast<void *>(&output_mem)));
  cl_int pattern = 0;
  cl_event fill_event;
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueFillBuffer(command_queue, input_mem, &pattern, sizeof(cl_int), 0,
                          sizeof(cl_int), 0, nullptr, &fill_event));
  EXPECT_TRUE(fill_event);
  cl_event task_event;
  EXPECT_SUCCESS(
      clEnqueueTask(command_queue, kernel, 1, &fill_event, &task_event));
  EXPECT_TRUE(task_event);

  ASSERT_SUCCESS(clWaitForEvents(1, &task_event));

  EXPECT_SUCCESS(clReleaseEvent(task_event));
  EXPECT_SUCCESS(clReleaseEvent(fill_event));
  EXPECT_SUCCESS(clReleaseMemObject(output_mem));
  ASSERT_SUCCESS(clReleaseMemObject(input_mem));
}

TEST_F(clEnqueueTaskTest, TaskExecutesExactlyOnce) {
  const char *source =
      "kernel void foo(global int *x)"
      "{ atomic_inc(x); }";
  ASSERT_SUCCESS(SetUpProgram(source));

  cl_int status = !CL_SUCCESS;
  cl_int data = 0;

  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int),
                                 nullptr, &status);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                               static_cast<void *>(&buffer)));

  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0, sizeof(cl_int),
                           &data, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0, sizeof(cl_int),
                          &data, 0, nullptr, nullptr));
  EXPECT_EQ(1, data);
  EXPECT_SUCCESS(clReleaseMemObject(buffer));
}

GENERATE_EVENT_WAIT_LIST_TESTS(clEnqueueTaskTest)

class clEnqueueTaskTestWithReqdWorkGroupSizeTest
    : public ucl::CommandQueueTest {
 protected:
  cl_int SetUpProgram(const char *source) {
    cl_int status = CL_SUCCESS;
    const size_t source_length = strlen(source);
    program =
        clCreateProgramWithSource(context, 1, &source, &source_length, &status);
    EXPECT_TRUE(program);
    UCL_SUCCESS_OR_RETURN_ERR(status);

    UCL_SUCCESS_OR_RETURN_ERR(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    UCL_SUCCESS_OR_RETURN_ERR(status);
    return CL_SUCCESS;
  }

  cl_program program;
  cl_kernel kernel;
};

TEST_F(clEnqueueTaskTestWithReqdWorkGroupSizeTest, DefaultNoAttribute) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source =
      "kernel void foo()"
      "{int a = 42;}";
  ASSERT_SUCCESS(SetUpProgram(source));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));
  ASSERT_SUCCESS(clReleaseEvent(event));
  // kernel and program are created in SetUpProgram()
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clEnqueueTaskTestWithReqdWorkGroupSizeTest, Default) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source =
      "kernel void  __attribute__((reqd_work_group_size(1, 1, 1))) foo()"
      "{int a = 42;}";
  ASSERT_SUCCESS(SetUpProgram(source));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));
  ASSERT_SUCCESS(clReleaseEvent(event));
  // kernel and program are created in SetUpProgram()
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clEnqueueTaskTestWithReqdWorkGroupSizeTest, InvalidAttribute) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source =
      "kernel void  __attribute__((reqd_work_group_size(1, 2, 1))) foo()"
      "{int a = 42;}";
  ASSERT_SUCCESS(SetUpProgram(source));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_INVALID_WORK_GROUP_SIZE,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  // kernel and program are created in SetUpProgram()
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clEnqueueTaskTestWithReqdWorkGroupSizeTest, TwoKernelsDefault) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source =
      "kernel void not_the_one() {int b = 43;}"
      "kernel void  __attribute__((reqd_work_group_size(1, 1, 1))) foo()"
      "{int a = 42;}";
  ASSERT_SUCCESS(SetUpProgram(source));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_TRUE(UCL::hasCommandExecutionCompleted(event));
  ASSERT_SUCCESS(clReleaseEvent(event));
  // kernel and program are created in SetUpProgram()
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clEnqueueTaskTestWithReqdWorkGroupSizeTest, TwoKernelsInvalidAttribute) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source =
      "kernel void not_the_one() {int b = 43;}"
      "kernel void  __attribute__((reqd_work_group_size(1, 2, 1))) foo()"
      "{int a = 42;}";
  ASSERT_SUCCESS(SetUpProgram(source));
  cl_event event;
  EXPECT_EQ_ERRCODE(CL_INVALID_WORK_GROUP_SIZE,
                    clEnqueueTask(command_queue, kernel, 0, nullptr, &event));
  // kernel and program are created in SetUpProgram()
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}
