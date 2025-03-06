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

#include "cl_khr_command_buffer.h"

struct CommandBufferEnqueueTest : public cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests inheriting from this class build programs from source and hence
    // require an online compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const bool simultaneous_support =
        capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

    if (!simultaneous_support) {
      GTEST_SKIP();
    }
  }
};

TEST_F(CommandBufferEnqueueTest, NullCommandBuffer) {
  EXPECT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clEnqueueCommandBufferKHR(0, nullptr, nullptr, 0, nullptr, nullptr));
}

TEST_F(CommandBufferEnqueueTest, InvalidCommandBuffer) {
  cl_int error;
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Command-buffer not finalized
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                              nullptr, nullptr));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Tests whether we can reuse a command buffer twice.
TEST_F(CommandBufferEnqueueTest, IncrementKernelTwice) {
  // Set up the kernel.
  // We need something we can check was enqueued twice.
  const char *code = R"OpenCLC(
  __kernel void increment_kernel(global int *counter) {
    ++(counter[0]);
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "increment_kernel", &error);
  EXPECT_SUCCESS(error);

  cl_mem counter_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                         sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the counter to zero.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, counter_buffer, &zero,
                                     sizeof(cl_int), 0, sizeof(cl_int), 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&counter_buffer)));

  // Set up the command buffer to allow multiple enqueues without a wait.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  constexpr size_t global_size = 1;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results, at this point the command buffer should have been
  // enqueued twice, so the counter should have value two.
  cl_int counter_result = -1;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, counter_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &counter_result, 0,
                                     nullptr, nullptr));
  EXPECT_EQ(2, counter_result);

  // Clean up.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(counter_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

// Tests whether we can reuse a command buffer twice on different queues.
TEST_F(CommandBufferEnqueueTest, IncrementKernelTwiceDifferentQueues) {
  // Set up the kernel.
  // We need something we can check was enqueued twice.
  const char *code = R"OpenCLC(
  __kernel void increment_kernel(global int *counter) {
    atomic_inc(&counter[0]);
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "increment_kernel", &error);
  EXPECT_SUCCESS(error);

  cl_mem counter_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                         sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the counter to zero.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, counter_buffer, &zero,
                                     sizeof(cl_int), 0, sizeof(cl_int), 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&counter_buffer)));

  // Set up the command buffer to allow multiple enqueues without a wait.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  constexpr size_t global_size = 1;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a second command queue with the same properties as the first
  // targeting the same device.
  auto second_queue = clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue two copies of the command buffer, one to each queue.
  // Have them each wait on a user event so we can start them at the same time.
  auto user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(1, &command_queue, command_buffer, 1,
                                           &user_event, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(1, &second_queue, command_buffer, 1,
                                           &user_event, nullptr));

  // Trigger the command buffers.
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Explicitly finish each queue so we know they have both completed.
  EXPECT_SUCCESS(clFinish(command_queue));
  EXPECT_SUCCESS(clFinish(second_queue));

  // Check the results, at this point the command buffer should have been
  // enqueued twice, so the counter should have value two. We can do this on
  // either queue since they both should have finished executing.
  cl_int counter_result = -1;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, counter_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &counter_result, 0,
                                     nullptr, nullptr));
  EXPECT_EQ(2, counter_result);

  // Clean up.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandQueue(second_queue));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(counter_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

// Check we return the correct error code when trying to use command-buffers
// simultaneously without setting the flag on creation.
TEST_F(CommandBufferEnqueueTest, SimultaneousUseWithoutFlag) {
  cl_int error;
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));

  // We didn't set the simultaneous use flag, so this is invalid
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                              nullptr, nullptr));

  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Generic smoke test designed to check the most basic functionality of a
// command buffer: can we enqueue a simple kernel that copies between buffers?
TEST_F(CommandBufferEnqueueTest, ParallelCopyKernel) {
  // Set up the kernel.
  const char *code = R"OpenCLC(
  __kernel void parallel_copy(__global int *src, __global int *dst) {
    size_t gid = get_global_id(0);
    dst[gid] = src[gid];
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "parallel_copy", &error);
  ASSERT_SUCCESS(error);

  // Set up the buffers.
  constexpr size_t global_size = 256;
  std::vector<cl_int> input_data(global_size);
  const size_t data_size_in_bytes =
      input_data.size() * sizeof(decltype(input_data)::value_type);
  cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                     data_size_in_bytes, nullptr, &error);
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      data_size_in_bytes, input_data.data(), 0,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_SUCCESS(error);
  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                     data_size_in_bytes, nullptr, &error);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&src_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_EQ(input_data, output_data);

  // Clean up.
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clReleaseMemObject(src_buffer));
  ASSERT_SUCCESS(clReleaseMemObject(dst_buffer));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

// Tests whether we can enqueue a command buffer containing a mix of commands.
TEST_F(CommandBufferEnqueueTest, MixedCommands) {
  // Set up the kernel.
  const char *code = R"OpenCLC(
  __kernel void empty_kernel() {}
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "empty_kernel", &error);
  ASSERT_SUCCESS(error);

  // Set up some buffers.
  constexpr size_t data_size_in_bytes = 1024;
  cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     data_size_in_bytes, nullptr, &error);
  ASSERT_SUCCESS(error);

  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     data_size_in_bytes, nullptr, &error);
  ASSERT_SUCCESS(error);

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);

  constexpr size_t global_size = 256;
  for (unsigned i = 0; i < 16; ++i) {
    ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
        nullptr, 0, nullptr, nullptr, nullptr));
    ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, src_buffer,
                                          dst_buffer, 0, 0, data_size_in_bytes,
                                          0, nullptr, nullptr, nullptr));
  }
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  for (unsigned i = 0; i < 4; ++i) {
    ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                             nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));
  }

  // Clean up.
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clReleaseMemObject(dst_buffer));
  ASSERT_SUCCESS(clReleaseMemObject(src_buffer));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

// This helper class allows us to quickly enqueue command buffers and regular
// commands in different combinations.
class InterleavedCommands : public cl_khr_command_buffer_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    // Requires a compiler to compile the kernels.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    // Set up the kernels.
    const char *code = R"OpenCLC(
    kernel void store_zero(global int *dst) { *dst = 0; }
    kernel void store_one(global int *dst) { *dst = 1; }
    kernel void store_two(global int *dst) { *dst = 2; }
)OpenCLC";
    const size_t code_length = std::strlen(code);

    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_SUCCESS(
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

    store_zero_kernel = clCreateKernel(program, "store_zero", &error);
    ASSERT_SUCCESS(error);
    store_one_kernel = clCreateKernel(program, "store_one", &error);
    ASSERT_SUCCESS(error);
    store_two_kernel = clCreateKernel(program, "store_two", &error);
    ASSERT_SUCCESS(error);

    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int),
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    ASSERT_SUCCESS(clSetKernelArg(store_zero_kernel, 0, sizeof(dst_buffer),
                                  static_cast<void *>(&dst_buffer)));
    ASSERT_SUCCESS(clSetKernelArg(store_one_kernel, 0, sizeof(dst_buffer),
                                  static_cast<void *>(&dst_buffer)));
    ASSERT_SUCCESS(clSetKernelArg(store_two_kernel, 0, sizeof(dst_buffer),
                                  static_cast<void *>(&dst_buffer)));
  }

  void TearDown() override {
    // Clean up.
    if (store_zero_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(store_zero_kernel));
    }
    if (store_one_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(store_one_kernel));
    }
    if (store_two_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(store_two_kernel));
    }
    if (dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  cl_kernel store_zero_kernel = nullptr;
  cl_kernel store_one_kernel = nullptr;
  cl_kernel store_two_kernel = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_program program = nullptr;
};

// Tests whether we can interleave command buffers and regular commands and
// maintain in order queues.
TEST_F(InterleavedCommands, EnqueueCommandBufferThenNDRangeImplicitFlush) {
  cl_int error = CL_SUCCESS;
  // Set up the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);
  const size_t global_size = 1;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, store_zero_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_one_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Tests whether we can interleave command buffers and regular commands and
// maintain in order queues.
TEST_F(InterleavedCommands, EnqueueNDRangeThenCommandBufferImplicitFlush) {
  cl_int error = CL_SUCCESS;
  // Set up the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);
  const size_t global_size = 1;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, store_one_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_zero_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

TEST_F(InterleavedCommands, EnqueueCommandBufferThenNDRangeExplicitFlush) {
  cl_int error = CL_SUCCESS;
  // Set up the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);
  const size_t global_size = 1;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, store_zero_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_one_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  cl_int result = 42;
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  ASSERT_EQ(result, 1);
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Tests whether we can interleave command buffers and regular commands and
// maintain in order queues.
TEST_F(InterleavedCommands, EnqueueNDRangeThenCommandBufferExplicitFlush) {
  cl_int error = CL_SUCCESS;
  // Set up the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);
  const size_t global_size = 1;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, store_one_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_zero_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  cl_int result = 42;
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  ASSERT_EQ(result, 1);
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Tests whether we can interleave command buffers and regular commands and
// maintain in order queues.
TEST_F(InterleavedCommands, InterleavedCommandBuffers) {
  cl_int error = CL_SUCCESS;
  // Set up the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);
  const size_t global_size = 1;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, store_zero_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_one_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store_two_kernel, 1,
                                        nullptr, &global_size, nullptr, 0,
                                        nullptr, nullptr));
  cl_int result = 42;
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  ASSERT_EQ(result, 2);
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

// Tests that in order-ness of the command queue is respected when a command
// buffer is enqueued after two regular commands that have user event
// depdenencies that are released in reverse order.
TEST_F(CommandBufferEnqueueTest, CommandBufferAfterReversedUserEvents) {
  cl_int error = CL_SUCCESS;
  // We need 3 buffers, two for the intermediate values and one for the final
  // value.
  cl_mem intermediate_buffer_a = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_int initial_value = -1;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_a,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem intermediate_buffer_b = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  initial_value = -2;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_b,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  initial_value = -3;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Create user events which the copies will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);

  // Create a command buffer with a single copy in it.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(clCommandCopyBufferKHR(
      command_buffer, nullptr, intermediate_buffer_b, final_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Now we enqueue the copies but have them wait on user events.
  const cl_int zero = 0;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_a,
                                      CL_FALSE, 0, sizeof(cl_int), &zero, 1,
                                      &user_event_a, nullptr));
  ASSERT_SUCCESS(clEnqueueCopyBuffer(
      command_queue, intermediate_buffer_a, intermediate_buffer_b, 0, 0,
      sizeof(cl_int), 1, &user_event_b, nullptr));

  ASSERT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));
  ASSERT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));

  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  ASSERT_EQ(result, 0);

  // Cleanup.
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clReleaseEvent(user_event_a));
  ASSERT_SUCCESS(clReleaseEvent(user_event_b));
  ASSERT_SUCCESS(clReleaseMemObject(final_buffer));
  ASSERT_SUCCESS(clReleaseMemObject(intermediate_buffer_a));
  ASSERT_SUCCESS(clReleaseMemObject(intermediate_buffer_b));
}

TEST_F(CommandBufferEnqueueTest, EnqueueInLoopWithBlockingRead) {
  // Set up the kernel.
  const char *code = R"OpenCLC(
  kernel void increment(global int *accumulator) {
    size_t gid = get_global_id(0);
    accumulator[gid]++;
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "increment", &error);
  EXPECT_SUCCESS(error);

  // Set up the buffers.
  constexpr size_t global_size = 256;
  constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
  cl_mem accumulator_buffer = clCreateBuffer(
      context, CL_MEM_WRITE_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, accumulator_buffer, &zero,
                                     sizeof(zero), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(accumulator_buffer),
                                static_cast<void *>(&accumulator_buffer)));

  // Set up the command buffer.
  // TODO CA-3358 - After command-buffer cleanup is resolved we should be able
  // to remove this simultaneous use property.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer in a loop.
  for (unsigned i = 0; i < 100; ++i) {
    EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                             nullptr, nullptr))
        << " Unable to enqueue on iteration " << i;
    // Check the results.
    std::vector<cl_int> output_data(global_size);
    EXPECT_SUCCESS(clEnqueueReadBuffer(
        command_queue, accumulator_buffer, CL_TRUE, 0, data_size_in_bytes,
        output_data.data(), 0, nullptr, nullptr));
    const std::vector<cl_int> expected_result(global_size, i + 1);
    EXPECT_EQ(expected_result, output_data)
        << "Result mismatch on iteration " << i;
  }

  // Clean up.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(accumulator_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(CommandBufferEnqueueTest, EnqueueInLoopWithoutBlockingRead) {
  // Set up the kernel.
  const char *code = R"OpenCLC(
  kernel void increment(global int *accumulator) {
    size_t gid = get_global_id(0);
    accumulator[gid]++;
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "increment", &error);
  EXPECT_SUCCESS(error);

  // Set up the buffers.
  constexpr size_t global_size = 256;
  constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
  cl_mem accumulator_buffer = clCreateBuffer(
      context, CL_MEM_WRITE_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, accumulator_buffer, &zero,
                                     sizeof(zero), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(accumulator_buffer),
                                static_cast<void *>(&accumulator_buffer)));

  // Set up the command buffer to allow multiple enqueues without a wait.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer in a loop.
  constexpr unsigned iterations = 100;
  for (unsigned i = 0; i < iterations; ++i) {
    EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                             nullptr, nullptr))
        << " Unable to enqueue on iteration " << i;
  }

  // Check the results.
  // The blocking read *should* flush the queue.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, accumulator_buffer, CL_TRUE,
                                     0, data_size_in_bytes, output_data.data(),
                                     0, nullptr, nullptr));
  const std::vector<cl_int> expected_result(global_size, iterations);
  EXPECT_EQ(expected_result, output_data);

  // Clean up.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(accumulator_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

class SubstituteCommandQueueTest : public cl_khr_command_buffer_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    auto error = CL_SUCCESS;
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
    ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  }

  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
};

TEST_F(SubstituteCommandQueueTest, CompatibleQueue) {
  // Create a compatible command queue.
  auto error = CL_SUCCESS;
  cl_command_queue compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer substituting the compatible command buffer for
  // replay.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(
      1, &compatible_command_queue, command_buffer, 0, nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(compatible_command_queue));
}

TEST_F(SubstituteCommandQueueTest, CompatibleQueueSimultaneousNoFlag) {
  // Create a compatible command queue.
  auto error = CL_SUCCESS;
  cl_command_queue compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer twice without sync, substituting the
  // compatible command buffer for replay in the second enqueue.

  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(1, &command_queue, command_buffer, 1,
                                           &user_event, nullptr));
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueCommandBufferKHR(1, &compatible_command_queue, command_buffer, 0,
                                nullptr, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandQueue(compatible_command_queue));
}

TEST_F(SubstituteCommandQueueTest, CompatibleQueueSimultaneousWithFlag) {
  const bool simultaneous_support =
      capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

  if (!simultaneous_support) {
    GTEST_SKIP();
  }

  // Create a command-buffer with the simultaneous use property set
  auto error = CL_SUCCESS;
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr simultaneous_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(simultaneous_command_buffer));

  // Create a compatible command queue.
  cl_command_queue compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer twice without sync, substituting the
  // compatible command buffer for replay in the second enqueue.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(
      1, &command_queue, simultaneous_command_buffer, 0, nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(1, &compatible_command_queue,
                                           simultaneous_command_buffer, 0,
                                           nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(compatible_command_queue));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(simultaneous_command_buffer));
}

TEST_F(SubstituteCommandQueueTest, NullQueues) {
  // Enqueue the command buffer substituting with null command queue parameter
  // but non-zero command queue length.
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueCommandBufferKHR(1, nullptr, command_buffer, 0,
                                              nullptr, nullptr));
}

TEST_F(SubstituteCommandQueueTest, ZeroQueues) {
  // Create a compatible command queue.
  auto error = CL_SUCCESS;
  cl_command_queue compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer substituting with non-null command queue
  // parameter but zero command queue length.
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueCommandBufferKHR(
                                          0, &compatible_command_queue,
                                          command_buffer, 0, nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(compatible_command_queue));
}

TEST_F(SubstituteCommandQueueTest, InvalidNumberQueues) {
  // Create two compatible command queues.
  auto error = CL_SUCCESS;
  cl_command_queue first_compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  cl_command_queue second_compatible_command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer substituting with more queues than at command
  // buffer creation.
  cl_command_queue command_queues[] = {first_compatible_command_queue,
                                       second_compatible_command_queue};
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clEnqueueCommandBufferKHR(2, command_queues, command_buffer,
                                              0, nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(first_compatible_command_queue));
  EXPECT_SUCCESS(clReleaseCommandQueue(second_compatible_command_queue));
}

TEST_F(SubstituteCommandQueueTest, IncompatibleQueueProperties) {
  // Create a incompatible command queue.
  // The command buffer was not created against a queue with the
  // CL_QUEUE_PROFILING_ENABLE poperty.
  auto error = CL_SUCCESS;
  cl_command_queue incompatible_command_queue =
      clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer substituting with incompatible command queue.
  EXPECT_EQ_ERRCODE(
      CL_INCOMPATIBLE_COMMAND_QUEUE_KHR,
      clEnqueueCommandBufferKHR(1, &incompatible_command_queue, command_buffer,
                                0, nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(incompatible_command_queue));
}

// TODO: This is a ucl::MultiDeviceTest
TEST_F(SubstituteCommandQueueTest, DISABLED_IncompatibleQueueDevice) {
  // This test may have to be skipped if there is only one device on the
  // platform.
  if (UCL::getNumDevices() < 2) {
    GTEST_SKIP() << "Requires more than one device in the platform to run";
  }

  auto second_device = UCL::getDevices()[1];
  const cl_device_id devices[]{device, second_device};

  // Create a context for the first two devices.
  auto error = CL_SUCCESS;
  auto shared_context = clCreateContext(nullptr, 2, devices,
                                        ucl::contextCallback, nullptr, &error);
  ASSERT_SUCCESS(error);

  // Create two command queues from this shared context.
  auto initial_command_queue =
      clCreateCommandQueue(shared_context, device, 0, &error);
  EXPECT_SUCCESS(error);

  auto substitute_command_queue =
      clCreateCommandQueue(shared_context, second_device, 0, &error);
  EXPECT_SUCCESS(error);

  // Create and finalize command buffer associated with the first queue.
  auto command_buffer =
      clCreateCommandBufferKHR(1, &initial_command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer substituting with incompatible command queue.
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clEnqueueCommandBufferKHR(1, &substitute_command_queue, command_buffer, 0,
                                nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseCommandQueue(substitute_command_queue));
  EXPECT_SUCCESS(clReleaseCommandQueue(initial_command_queue));
  EXPECT_SUCCESS(clReleaseContext(shared_context));
}

TEST_F(SubstituteCommandQueueTest, IncompatibleQueueContext) {
  // Create an incompatible queue from a different context targeting the same
  // device.
  auto error = CL_SUCCESS;
  auto new_context = clCreateContext(nullptr, 1, &device, ucl::contextCallback,
                                     nullptr, &error);
  ASSERT_SUCCESS(error);
  auto incompatible_command_queue =
      clCreateCommandQueue(new_context, device, 0, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer substituting with incompatible command queue.
  EXPECT_EQ_ERRCODE(
      CL_INCOMPATIBLE_COMMAND_QUEUE_KHR,
      clEnqueueCommandBufferKHR(1, &incompatible_command_queue, command_buffer,
                                0, nullptr, nullptr));

  // Cleanup resources.
  EXPECT_SUCCESS(clReleaseCommandQueue(incompatible_command_queue));
  EXPECT_SUCCESS(clReleaseContext(new_context));
}
