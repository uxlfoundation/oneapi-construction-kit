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
/// Enqueing commands to a command buffer should be atomic. Tests in this file
/// attempt to verify this is the case by enqueuing commands to command buffers
/// from different threads, as well as enqueing command buffers to command
/// queues from different threads.
///
/// It is possible for these tests to produce false positives and ideally should
/// be run several times.

#include <thread>

#include "cl_khr_command_buffer.h"

struct CommandBufferThreadSafetyTest : public cl_khr_command_buffer_Test {};

struct CommandBufferNDRangeThreadSafetyTest
    : public cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests inheriting from this class build programs from source and hence
    // require an online compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }
};

// Tests that clCommandFillBuffer is thread safe.
TEST_F(CommandBufferThreadSafetyTest, FillInParallel) {
  cl_int error = CL_SUCCESS;
  const size_t thread_count = std::thread::hardware_concurrency();
  const size_t element_count = 64;
  const size_t buffer_size_in_bytes = element_count * sizeof(cl_char);

  // Allocate a small buffer of 64 cl_chars for each thread the system can
  // launch.
  std::vector<cl_mem> buffers_to_fill;
  buffers_to_fill.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);
    buffers_to_fill.push_back(buffer);
  }

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Launch thread_count threads, each one enqueing a fill command to the
  // command buffer.
  auto command_queue = this->command_queue;
  auto enqueue_fill_command = [&](cl_int &error, size_t id) {
    const cl_char pattern = id;
    error = clCommandFillBufferKHR(
        command_buffer, nullptr, buffers_to_fill[id], &pattern, sizeof(pattern),
        0, buffer_size_in_bytes, 0, nullptr, nullptr, nullptr);
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  std::vector<cl_int> thread_errors(thread_count, CL_SUCCESS);
  thread_errors.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    threads.emplace_back(enqueue_fill_command, std::ref(thread_errors[i]), i);
  }

  // Join all the threads and check their error codes.
  for (unsigned i = 0; i < thread_count; ++i) {
    threads[i].join();
    EXPECT_SUCCESS(thread_errors[i]);
  }

  // Finalize the command buffer. This has to be done after the joining of
  // threads since enqueuing commands to a finalized command buffer is
  // not allowed.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for each fill command that was enqueued asynchronously.
  for (unsigned i = 0; i < thread_count; ++i) {
    std::vector<cl_char> results(buffer_size_in_bytes, 0x00);
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffers_to_fill[i],
                                       CL_TRUE, 0, buffer_size_in_bytes,
                                       results.data(), 0, nullptr, nullptr));
    for (unsigned j = 0; j < buffer_size_in_bytes; ++j) {
      auto result = results[j];
      EXPECT_EQ(result, i) << "Result mismatch in buffer " << i << " at index "
                           << j << "\nExpected " << i << " got " << result;
    }
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));

  for (unsigned i = 0; i < thread_count; ++i) {
    ASSERT_SUCCESS(clReleaseMemObject(buffers_to_fill[i]));
  }
}

// Tests that clCommandCopyBuffer is thread safe.
TEST_F(CommandBufferThreadSafetyTest, CopyInParallel) {
  cl_int error = CL_SUCCESS;
  const size_t thread_count = std::thread::hardware_concurrency();
  const size_t element_count = 64;
  const size_t buffer_size_in_bytes = element_count * sizeof(cl_char);

  // Allocate two small buffers of 64 cl_char for each thread the system can
  // launch.
  std::vector<cl_mem> src_buffers;
  src_buffers.reserve(thread_count);
  std::vector<cl_mem> dst_buffers;
  dst_buffers.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    src_buffers.push_back(src_buffer);
    dst_buffers.push_back(dst_buffer);
  }

  // Fill the input buffers with values for each thread.
  for (unsigned i = 0; i < thread_count; ++i) {
    const cl_char pattern = i;
    EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, src_buffers[i], &pattern,
                                       sizeof(pattern), 0, buffer_size_in_bytes,
                                       0, nullptr, nullptr));
  }

  // Wait for all the fills to finish.
  EXPECT_SUCCESS(clFinish(command_queue));

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Launch thread_count threads, each one enqueing a copy command to the
  // command buffer.
  auto command_queue = this->command_queue;
  auto enqueue_copy_command = [&](cl_int &error, size_t id) {
    error = clCommandCopyBufferKHR(command_buffer, nullptr, src_buffers[id],
                                   dst_buffers[id], 0, 0, buffer_size_in_bytes,
                                   0, nullptr, nullptr, nullptr);
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  std::vector<cl_int> thread_errors(thread_count, CL_SUCCESS);
  thread_errors.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    threads.emplace_back(enqueue_copy_command, std::ref(thread_errors[i]), i);
  }

  // Join all the threads and check their error codes.
  for (unsigned i = 0; i < thread_count; ++i) {
    threads[i].join();
    EXPECT_SUCCESS(thread_errors[i]);
  }

  // Finalize the command buffer. This has to be done after the joining of
  // threads since enqueuing commands to a finalized command buffer is
  // not allowed.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for each copy command that was enqueued asynchronously.
  for (unsigned i = 0; i < thread_count; ++i) {
    std::vector<cl_char> results(buffer_size_in_bytes, 0x00);
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffers[i], CL_TRUE,
                                       0, buffer_size_in_bytes, results.data(),
                                       0, nullptr, nullptr));
    for (unsigned j = 0; j < buffer_size_in_bytes; ++j) {
      auto result = results[j];
      EXPECT_EQ(result, i) << "Result mismatch in buffer " << i << " at index "
                           << j << "\nExpected " << i << " got " << result;
    }
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));

  for (unsigned i = 0; i < thread_count; ++i) {
    EXPECT_SUCCESS(clReleaseMemObject(src_buffers[i]));
    EXPECT_SUCCESS(clReleaseMemObject(dst_buffers[i]));
  }
}

// Tests that clCommandCopyBuffer is thread safe.
TEST_F(CommandBufferThreadSafetyTest, CopyRectInParallel) {
  cl_int error = CL_SUCCESS;
  const size_t thread_count = std::thread::hardware_concurrency();
  const size_t element_count = 64;
  const size_t buffer_size_in_bytes = element_count * sizeof(cl_char);

  // Allocate two small buffers of 64 cl_chars for each thread the system can
  // launch.
  std::vector<cl_mem> src_buffers;
  src_buffers.reserve(thread_count);
  std::vector<cl_mem> dst_buffers;
  src_buffers.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    src_buffers.push_back(src_buffer);
    dst_buffers.push_back(dst_buffer);
  }

  // Fill the input buffers with values for each thread.
  for (unsigned i = 0; i < thread_count; ++i) {
    const cl_char pattern = i;
    EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, src_buffers[i], &pattern,
                                       sizeof(pattern), 0, buffer_size_in_bytes,
                                       0, nullptr, nullptr));
  }

  // Wait for all the fills to finish.
  EXPECT_SUCCESS(clFinish(command_queue));

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Launch thread_count threads, each one enqueing a rectangular copy command
  // to the command buffer.
  auto command_queue = this->command_queue;
  auto enqueue_copy_command = [&](cl_int &error, size_t id) {
    const size_t origin[]{0, 0, 0};
    const size_t region[]{buffer_size_in_bytes, 1, 1};
    error = clCommandCopyBufferRectKHR(
        command_buffer, nullptr, src_buffers[id], dst_buffers[id], origin,
        origin, region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  std::vector<cl_int> thread_errors(thread_count, CL_SUCCESS);
  thread_errors.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    threads.emplace_back(enqueue_copy_command, std::ref(thread_errors[i]), i);
  }

  // Join all the threads and check their error codes.
  for (unsigned i = 0; i < thread_count; ++i) {
    threads[i].join();
    EXPECT_SUCCESS(thread_errors[i]);
  }

  // Finalize the command buffer. This has to be done after the joining of
  // threads since enqueuing commands to a finalized command buffer is
  // not allowed.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for each copy command that was enqueued asynchronously.
  for (unsigned i = 0; i < thread_count; ++i) {
    std::vector<cl_char> results(buffer_size_in_bytes, 0x00);
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffers[i], CL_TRUE,
                                       0, buffer_size_in_bytes, results.data(),
                                       0, nullptr, nullptr));
    for (unsigned j = 0; j < buffer_size_in_bytes; ++j) {
      auto result = results[j];
      EXPECT_EQ(result, i) << "Result mismatch in buffer " << i << " at index "
                           << j << "\nExpected " << i << " got " << result;
    }
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));

  for (unsigned i = 0; i < thread_count; ++i) {
    EXPECT_SUCCESS(clReleaseMemObject(src_buffers[i]));
    EXPECT_SUCCESS(clReleaseMemObject(dst_buffers[i]));
  }
}

// Tests that clCommandNDRange is thread safe.
TEST_F(CommandBufferNDRangeThreadSafetyTest, NDRangeInParallel) {
  // Build a program that does a parallel vector addition.
  const char *kernel_source_code = R"""(
    void kernel vector_add(global int *src_a, global int *src_b, global int *dst) {
        const size_t gid = get_global_id(0);
        dst[gid] = src_a[gid] + src_b[gid];
        })""";
  const size_t source_length = std::strlen(kernel_source_code);

  cl_int error = CL_SUCCESS;

  cl_program program = clCreateProgramWithSource(
      context, 1, &kernel_source_code, &source_length, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                ucl::buildLogCallback, nullptr));

  const size_t thread_count = std::thread::hardware_concurrency();
  const size_t element_count = 64;
  const size_t buffer_size_in_bytes = element_count * sizeof(cl_int);

  std::vector<cl_kernel> kernels;
  kernels.reserve(thread_count);
  std::vector<cl_mem> src_a_buffers;
  src_a_buffers.reserve(thread_count);
  std::vector<cl_mem> src_b_buffers;
  src_b_buffers.reserve(thread_count);
  std::vector<cl_mem> dst_buffers;
  dst_buffers.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    // Create a kernel for each thread to enqueue to the command buffer.
    cl_kernel kernel = clCreateKernel(program, "vector_add", &error);
    EXPECT_SUCCESS(error);
    kernels.push_back(kernel);

    // Allocate two small input buffers and one ouput buffer each of 64
    // cl_ints for each thread the system can launch.
    cl_mem src_a_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                         buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);
    src_a_buffers.push_back(src_a_buffer);

    cl_mem src_b_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                         buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);
    src_b_buffers.push_back(src_b_buffer);

    cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                       buffer_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);
    dst_buffers.push_back(dst_buffer);

    // Set the kernel args for each kernel.
    EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&src_a_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&src_b_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));
  }

  // Fill the input buffers with values for each thread.
  for (unsigned i = 0; i < thread_count; ++i) {
    const cl_int pattern = i;
    EXPECT_SUCCESS(clEnqueueFillBuffer(
        command_queue, src_a_buffers[i], &pattern, sizeof(pattern), 0,
        buffer_size_in_bytes, 0, nullptr, nullptr));
    EXPECT_SUCCESS(clEnqueueFillBuffer(
        command_queue, src_b_buffers[i], &pattern, sizeof(pattern), 0,
        buffer_size_in_bytes, 0, nullptr, nullptr));
  }

  // Wait for all the fills to finish.
  EXPECT_SUCCESS(clFinish(command_queue));

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Launch thread_count threads, each one enqueing an nd range to the command
  // buffer.
  auto command_queue = this->command_queue;
  // element_count needs to be explicitly captured to work around a bug in gcc7.
  auto enqueue_ndrange_command = [&, element_count](cl_int &error, size_t id) {
    error = clCommandNDRangeKernelKHR(command_buffer, nullptr, nullptr,
                                      kernels[id], 1, nullptr, &element_count,
                                      nullptr, 0, nullptr, nullptr, nullptr);
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  std::vector<cl_int> thread_errors(thread_count, CL_SUCCESS);
  thread_errors.reserve(thread_count);
  for (unsigned i = 0; i < thread_count; ++i) {
    threads.emplace_back(enqueue_ndrange_command, std::ref(thread_errors[i]),
                         i);
  }

  // Join all the threads and check their error codes.
  for (unsigned i = 0; i < thread_count; ++i) {
    threads[i].join();
    EXPECT_SUCCESS(thread_errors[i]);
  }

  // Finalize the command buffer. This has to be done after the joining of
  // threads since enqueuing commands to a finalized command buffer is
  // not allowed.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for each nd range command that was enqueued
  // asynchronously.
  for (unsigned i = 0; i < thread_count; ++i) {
    std::vector<cl_int> results(element_count, 0x00);
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffers[i], CL_TRUE,
                                       0, buffer_size_in_bytes, results.data(),
                                       0, nullptr, nullptr));
    for (unsigned j = 0; j < element_count; ++j) {
      auto result = results[j];
      EXPECT_EQ(result, 2 * i)
          << "Result mismatch in buffer " << i << " at index " << j
          << "\nExpected " << i << " got " << result;
    }
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));

  for (unsigned i = 0; i < thread_count; ++i) {
    EXPECT_SUCCESS(clReleaseMemObject(src_a_buffers[i]));
    EXPECT_SUCCESS(clReleaseMemObject(src_b_buffers[i]));
    EXPECT_SUCCESS(clReleaseMemObject(dst_buffers[i]));
    EXPECT_SUCCESS(clReleaseKernel(kernels[i]));
  }

  EXPECT_SUCCESS(clReleaseProgram(program));
}
