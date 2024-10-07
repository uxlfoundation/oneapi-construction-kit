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
#include "kts/stdout_capture.h"

class CommandNDRangeKernelTest : public cl_khr_command_buffer_Test {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests inheriting from this class build programs from source and hence
    // require an online compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }
};

// Tests the whether we can enqueue an execute a command buffer containing a
// kernel enqueue create via clCommandNDRangeKernelKHR.
TEST_F(CommandNDRangeKernelTest, EmptyKernel) {
  // Requires a compiler to compile the kernel.
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
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

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);

  constexpr size_t global_size = 256;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Clean up.
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

// Test fixture checking whether we can successfully enqueue and execute a
// command buffer that does a parallel copy via a kernel.
// The command buffer consists of a single kernel enqueue which does the
// following: kernel void parallel_copy(__global int *src, __global int *dst) {
//   size_t gid = get_global_id(0);
//   dst[gid] = src[gid];
// }
class CommandBufferParallelCopyBase : public CommandNDRangeKernelTest {
 protected:
  CommandBufferParallelCopyBase()
      : input_data(global_size, 42), output_data(global_size, 0) {};

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandNDRangeKernelTest::SetUp());

    cl_int error = CL_SUCCESS;

    // Create input buffer.
    src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Fill input with random numbers.
    ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);
    EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                        data_size_in_bytes, input_data.data(),
                                        0, nullptr, nullptr));

    // Create output buffer.
    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Fill output buffer with zero.
    const cl_int zero = 0x0;
    EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                       sizeof(cl_int), 0, data_size_in_bytes, 0,
                                       nullptr, nullptr));

    // Flush the queue.
    EXPECT_SUCCESS(clFinish(command_queue));

    // Create a command buffer.
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    EXPECT_SUCCESS(error);
  }

  void buildKernel(std::string reqd_work_group_attribute = "") {
    cl_int error = CL_SUCCESS;
    const char *kernel_source = R"(
        REQD_WORK_GROUP_SIZE_ATTRIBUTE
        void kernel parallel_copy(global int *src, global int *dst) {
          size_t gid = get_global_id(0);
          dst[gid] = src[gid];
          }
        )";
    const size_t kernel_source_length = std::strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source,
                                        &kernel_source_length, &error);
    const std::string reqd_work_group_attribute_define =
        "-DREQD_WORK_GROUP_SIZE_ATTRIBUTE=" + reqd_work_group_attribute;
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device,
                                  reqd_work_group_attribute_define.c_str(),
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "parallel_copy", &error);
    EXPECT_SUCCESS(error);

    EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&src_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));
  }

  void TearDown() override {
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    if (nullptr != src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }

    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }

    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }

    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  std::vector<cl_int> input_data;
  std::vector<cl_int> output_data;

  static constexpr size_t global_size = 256;
  static constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
};

class ParallelCopyCommandBuffer : public CommandBufferParallelCopyBase {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandBufferParallelCopyBase::SetUp());
    buildKernel();
  }
};

TEST_F(ParallelCopyCommandBuffer, Sync) {
  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 2, sync_points, nullptr, nullptr));
}

// Tests whether we can enqueue a kernel using global_offset = NULL so that
// the runtime is forced to choose an appropriate local size.
TEST_F(ParallelCopyCommandBuffer, DefaultLocalSize) {
  // Put an ND range in the command buffer with a default local size.
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Read the results of the output buffer to check nothing strange happened.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  EXPECT_EQ(input_data, output_data);
}

// Tests whether we can enqueue a kernel using a specific local size.
TEST_F(ParallelCopyCommandBuffer, UserChosenLocalSize) {
  // Put an ND range in the command buffer with a user chosen local
  // size.
  const size_t local_size = 8;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      &local_size, 0, nullptr, nullptr, nullptr));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Read the results of the output buffer to check nothing strange happened.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  EXPECT_EQ(input_data, output_data);
}

TEST_F(ParallelCopyCommandBuffer, NullCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clCommandNDRangeKernelKHR(nullptr, nullptr, nullptr, kernel,
                                              1, nullptr, &global_size, nullptr,
                                              0, nullptr, nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidCommandBuffer) {
  // Finalize the command buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  // Put an ND range in the command buffer with a default local size.
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandNDRangeKernelKHR(
                        command_buffer, nullptr, nullptr, kernel, 1, nullptr,
                        &global_size, nullptr, 0, nullptr, nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_QUEUE,
      clCommandNDRangeKernelKHR(command_buffer, command_queue, nullptr, kernel,
                                1, nullptr, &global_size, nullptr, 0, nullptr,
                                nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidKernel) {
  ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL,
                    clCommandNDRangeKernelKHR(
                        command_buffer, nullptr, nullptr, nullptr, 1, nullptr,
                        &global_size, nullptr, 0, nullptr, nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidProperties) {
  // Properties are defined by mutable-dispatch extension
  if (UCL::hasDeviceExtensionSupport(device, "cl_codeplay_mutable_dispatch")) {
    GTEST_SKIP();
  }

  cl_ndrange_kernel_command_properties_khr valid_properties[1] = {0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, valid_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));

  cl_ndrange_kernel_command_properties_khr invalid_properties[3] = {0xDEAD,
                                                                    0xBEEF, 0};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, invalid_properties,
                                kernel, 1, nullptr, &global_size, nullptr, 0,
                                nullptr, nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidHandle) {
  // Handle is enabled by mutable-dispatch extension
  if (UCL::hasDeviceExtensionSupport(
          device, "cl_khr_command_buffer_mutable_dispatch")) {
    GTEST_SKIP();
  }

  cl_mutable_command_khr command_handle;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, nullptr, kernel, 1,
                                nullptr, &global_size, nullptr, 0, nullptr,
                                nullptr, &command_handle));
}

TEST_F(ParallelCopyCommandBuffer, InvalidSyncPoints) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandNDRangeKernelKHR(
                        command_buffer, nullptr, nullptr, kernel, 1, nullptr,
                        &global_size, nullptr, 1, nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, nullptr, kernel, 1,
                                nullptr, &global_size, nullptr, 0, &sync_point,
                                nullptr, nullptr));
}

TEST_F(ParallelCopyCommandBuffer, InvalidContext) {
  cl_int error;
  cl_context new_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_command_queue new_command_queue =
      clCreateCommandQueue(new_context, device, 0, &error);
  EXPECT_SUCCESS(error);

  cl_command_buffer_khr new_command_buffer =
      clCreateCommandBufferKHR(1, &new_command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clCommandNDRangeKernelKHR(new_command_buffer, nullptr, nullptr, kernel, 1,
                                nullptr, &global_size, nullptr, 0, nullptr,
                                nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseCommandBufferKHR(new_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandQueue(new_command_queue));
  EXPECT_SUCCESS(clReleaseContext(new_context));
}

class CommandBufferParallelCopyReqdWorkGroupSize
    : public CommandBufferParallelCopyBase {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandBufferParallelCopyBase::SetUp());
    buildKernel("__attribute__((reqd_work_group_size(8,1,1)))");
  }
};

// Tests whether we can enqueue a kernel with the reqd_work_group_size attribute
// where the user specifies a local size that matches that of the kernel
// attribute.
TEST_F(CommandBufferParallelCopyReqdWorkGroupSize, ReqdWorkGroupSizeMatch) {
  // Put an ND range in the command buffer with a user chosen local
  // size.
  const size_t local_size = 8;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      &local_size, 0, nullptr, nullptr, nullptr));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Read the results of the output buffer to check nothing strange happened.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  EXPECT_EQ(input_data, output_data);
}

// Tests whether we can enqueue a kernel with the reqd_work_group_size attribute
// where the user specifies a local size that does not matches that of the
// kernel attribute.
TEST_F(CommandBufferParallelCopyReqdWorkGroupSize, ReqdWorkGroupSizeMismatch) {
  // Put an ND range in the command buffer with a user chosen local
  // size.
  const size_t local_size = 42;
  EXPECT_EQ(CL_INVALID_WORK_GROUP_SIZE,
            clCommandNDRangeKernelKHR(command_buffer, nullptr, nullptr, kernel,
                                      1, nullptr, &global_size, &local_size, 0,
                                      nullptr, nullptr, nullptr));
}

// Test recording and replaying a kernel with a printf builtin, which requires
// the device reporting a capability for this
TEST_F(CommandNDRangeKernelTest, Printf) {
  // Requires a compiler to compile the kernel.
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  if (0 == (capabilities & CL_COMMAND_BUFFER_CAPABILITY_KERNEL_PRINTF_KHR)) {
    GTEST_SKIP();
  }

  const char *code = R"OpenCLC(
  __kernel void printf_kernel() {
    printf("Hello World\n");
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "printf_kernel", &error);
  EXPECT_SUCCESS(error);

  const bool simultaneous_use_support =
      capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  if (!simultaneous_use_support) {
    properties[1] = 0;
  }

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  constexpr size_t global_size = 4;
  constexpr size_t local_size = global_size / 2;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      &local_size, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Start capturing stdout
  kts::StdoutCapture capture;
  capture.CaptureStdout();

  // Run command-buffer with print kernel
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Try to run command-buffer with print kernel again in same submission
  if (!simultaneous_use_support) {
    EXPECT_SUCCESS(clFinish(command_queue));
  }
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Run command-buffer again, in separate submissions
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Read captured stdout
  capture.RestoreStdout();
  const std::string buf = capture.ReadBuffer();

  // Build reference based on that we enqueue the command-buffer three times
  std::ostringstream reference;
  for (unsigned i = 0; i < (3 * global_size); i++) {
    reference << "Hello World\n";
  }

  // Assert correct text was printed
  EXPECT_TRUE(buf == reference.str()) << "\nExpected:\n"
                                      << reference.str() << "\nResult:\n"
                                      << buf;

  // Clean up.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TEST_F(CommandNDRangeKernelTest, PODargument) {
  const bool simultaneous_use_support =
      capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;
  if (!simultaneous_use_support) {
    GTEST_SKIP();
  }

  const char *code = R"OpenCLC(
  __kernel void pod_kernel(global int *output, int val) {
    output[get_global_id(0)] = val;
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "pod_kernel", &error);
  EXPECT_SUCCESS(error);

  // Create input buffer.
  const size_t work_items = 4;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                 work_items * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_int data = 42;
  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&buffer)));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_int), &data));

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &work_items,
      nullptr, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int overwrite = 0xABCD;
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_int), &overwrite));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clFinish(command_queue));

  std::array<cl_int, work_items> output_data;
  const std::array<cl_int, work_items> expected_output = {data, data, data,
                                                          data};
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     sizeof(cl_int) * work_items,
                                     output_data.data(), 0, nullptr, nullptr));

  EXPECT_EQ(output_data, expected_output);

  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseMemObject(buffer));
}
