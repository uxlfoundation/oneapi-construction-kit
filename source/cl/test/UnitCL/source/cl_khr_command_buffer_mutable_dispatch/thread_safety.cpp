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

/// @file
///
/// @brief Tests the thread safety.
///
/// Updating and enqueing command buffers should be atomic. Tests in this
/// file attempt to verify this is the case by updating command buffers from
/// different threads.
///
/// It is possible for these tests to produce false positives and ideally should
/// be run several times.

#include <thread>

#include "cl_khr_command_buffer_mutable_dispatch.h"

struct MutableDispatchThreadSafetyTest : public MutableDispatchTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());

    // Tests inheriting from this class build programs from source and hence
    // require an online compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }
};

// Tests that clUpdateMutableCommandsKHR is thread safe.
TEST_F(MutableDispatchThreadSafetyTest, UpdateInParallel) {
  // Build a program that just writes a single integer into a buffer.
  const char *kernel_source_code = R"""(
    void kernel broadcast(int input, global int *dst) {
        const size_t gid = get_global_id(0);
        dst[gid] = input;
        })""";
  const size_t source_length = std::strlen(kernel_source_code);

  cl_int error = CL_SUCCESS;

  cl_program program = clCreateProgramWithSource(
      context, 1, &kernel_source_code, &source_length, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                ucl::buildLogCallback, nullptr));

  // TODO: Update thread count to std::thread::hardware_concurrency()
  // once CA-3232 is complete (currently enqueuing the same command buffer twice
  // to a queue will cause an assert).
  const size_t thread_count = 1;
  const size_t element_count = 64;
  const size_t buffer_size_in_bytes = element_count * sizeof(cl_int);

  cl_kernel kernel = clCreateKernel(program, "broadcast", &error);
  EXPECT_SUCCESS(error);

  // Allocate a destination buffer to hold the result of the broadcast
  // operation.
  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                     buffer_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Set the kernel args.
  const cl_int initial_value = 42;
  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(initial_value), &initial_value));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&dst_buffer)));

  // Create command-buffer with mutable flag so we can update it
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the ND range to the kernel getting a handle so we can update it
  // later.
  cl_mutable_command_khr command_handle;
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &element_count, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a function to update and enqueue the command buffer on a spawned
  // thread.
  auto update_input_value_and_enqueue = [&](size_t id) {
    const cl_int updated_input_value = id;
    // Create a mutable config.
    const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_int),
                                          &updated_input_value};
    const cl_mutable_dispatch_config_khr dispatch_config{
        CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
        nullptr,
        command_handle,
        1,
        0,
        0,
        0,
        &arg,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr};
    const cl_mutable_base_config_khr mutable_config{
        CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
        &dispatch_config};
    // Update the nd range.
    EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
    EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                             nullptr, nullptr));
  };

  // Each thread updates the input argument and enqueues the command buffer.
  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    threads.emplace_back(update_input_value_and_enqueue, i);
  }

  // Join all the threads and check their error codes.
  for (unsigned i = 0; i < thread_count; ++i) {
    threads[i].join();
  }

  // Check the result is equal to one of the thread IDs.
  std::vector<cl_int> results(element_count, 0x00);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     buffer_size_in_bytes, results.data(), 0,
                                     nullptr, nullptr));
  for (unsigned i = 0; i < element_count; ++i) {
    auto result = results[i];
    EXPECT_TRUE(0 <= result && result < (cl_int)thread_count);
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}
