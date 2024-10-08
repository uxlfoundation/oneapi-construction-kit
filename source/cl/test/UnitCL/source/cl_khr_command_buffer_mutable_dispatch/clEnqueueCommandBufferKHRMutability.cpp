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
#include "cl_khr_command_buffer_mutable_dispatch.h"

// Test fixture for checking we can update kernel arguments that are buffers.
class CommandBufferMutableBufferArgTest : public MutableDispatchTest {
 public:
  CommandBufferMutableBufferArgTest()
      : input_data(global_size), output_data(global_size) {}

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());
    // Set up the kernel. This contrived example just does a copy in parallel
    // between two buffers, allowing us to check we can update the arguments.
    const char *code = R"OpenCLC(
  kernel void parallel_copy(global int *src, global int *dst) {
    size_t gid = get_global_id(0);
    dst[gid] = src[gid];
  }
)OpenCLC";
    const size_t code_length = std::strlen(code);

    // Build the kernel.
    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

    parallel_copy_kernel = clCreateKernel(program, "parallel_copy", &error);
    EXPECT_SUCCESS(error);

    // Create initial buffers for the input and output.
    src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Fill the input buffer with random numbers.
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                        data_size_in_bytes, input_data.data(),
                                        0, nullptr, nullptr));

    // Set up the initial kernel arguments.
    EXPECT_SUCCESS(clSetKernelArg(parallel_copy_kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&src_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(parallel_copy_kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);
  }

  void TearDown() override {
    // Cleanup.
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (nullptr != parallel_copy_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(parallel_copy_kernel));
    }
    if (nullptr != src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }
    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    MutableDispatchTest::TearDown();
  }

  std::vector<cl_int> input_data;
  std::vector<cl_int> output_data;
  cl_program program = nullptr;
  cl_kernel parallel_copy_kernel = nullptr;
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mutable_command_khr command_handle = nullptr;
  static constexpr size_t global_size = 256;
  static constexpr const size_t data_size_in_bytes =
      global_size * sizeof(cl_int);
};

TEST_F(CommandBufferMutableBufferArgTest, UpdateOutputBufferOnce) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, UpdateOutputBufferTwice) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create two new output buffers.
  cl_mem first_updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem second_updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr first_arg{
      1, sizeof(cl_mem), static_cast<void *>(&first_updated_dst_buffer)};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr first_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &first_dispatch_config};

  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &first_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check we were able to successfully update the output buffer.
  std::vector<cl_int> first_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, first_updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      first_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, first_updated_output_data);

  // Enqueue the command buffer again updating the output buffer a second time.
  const cl_mutable_dispatch_arg_khr second_arg{
      1, sizeof(cl_mem), static_cast<void *>(&second_updated_dst_buffer)};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr second_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &second_dispatch_config};
  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &second_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check we were able to successfully update the output buffer a second time.
  std::vector<cl_int> second_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, second_updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_updated_dst_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, UpdateInputBufferOnce) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new input buffer filling it with random values.
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the input buffer.
  const cl_mutable_dispatch_arg_khr arg{
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the updated input.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(updated_input_data, output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, UpdateInputBufferTwice) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create two new input buffers filling them with random values.
  cl_mem first_updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem second_updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  std::vector<cl_int> first_updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      first_updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, first_updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      first_updated_input_data.data(), 0, nullptr, nullptr));

  std::vector<cl_int> second_updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      second_updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, second_updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_updated_input_data.data(), 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the input buffer.
  const cl_mutable_dispatch_arg_khr first_arg{
      0, sizeof(cl_mem), static_cast<void *>(&first_updated_src_buffer)};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr first_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &first_dispatch_config};
  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &first_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the updated input.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(first_updated_input_data, output_data);

  // Enqueue the command buffer a second time updating the input bufer again.
  const cl_mutable_dispatch_arg_khr second_arg{
      0, sizeof(cl_mem), static_cast<void *>(&second_updated_src_buffer)};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr second_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &second_dispatch_config};
  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &second_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the second updated input.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(second_updated_input_data, output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_updated_src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_updated_src_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest,
       UpdateInputAndOutputBuffersSameMutableDispatchConfig) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new input buffer filling it with random values.
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the input and output
  // buffers.
  const cl_mutable_dispatch_arg_khr arg_1{
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
  const cl_mutable_dispatch_arg_khr arg_2{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  cl_mutable_dispatch_arg_khr args[] = {arg_1, arg_2};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      2,
      0,
      0,
      0,
      args,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the updated buffers.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, updated_dst_buffer, CL_TRUE,
                                     0, data_size_in_bytes, output_data.data(),
                                     0, nullptr, nullptr));
  EXPECT_EQ(updated_input_data, output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest,
       UpdateInputAndOutputBuffersDifferentMutableDispatchConfigs) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new input buffer filling it with random values.
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the input and output
  // buffers.
  const cl_mutable_dispatch_arg_khr arg_1{
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
  const cl_mutable_dispatch_arg_khr arg_2{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config_1{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg_1,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_dispatch_config_khr dispatch_config_2{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg_2,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_dispatch_config_khr dispatch_configs[] = {dispatch_config_1,
                                                       dispatch_config_2};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, dispatch_configs};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the updated buffers.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, updated_dst_buffer, CL_TRUE,
                                     0, data_size_in_bytes, output_data.data(),
                                     0, nullptr, nullptr));
  EXPECT_EQ(updated_input_data, output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, UpdateToBiggerBufferSize) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;

  // Create a new output buffer which is bigger than the original output
  // buffer filling it with some value.
  const auto num_extra_elements = 99;
  const auto updated_buffer_size_in_bytes =
      data_size_in_bytes + (num_extra_elements * sizeof(cl_int));
  cl_mem updated_dst_buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, updated_buffer_size_in_bytes,
                     nullptr, &error);
  EXPECT_SUCCESS(error);

  const cl_int forty_two = 0x42;
  EXPECT_SUCCESS(clEnqueueFillBuffer(
      command_queue, updated_dst_buffer, &forty_two, sizeof(cl_int), 0,
      updated_buffer_size_in_bytes, 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> updated_output_data(global_size + num_extra_elements);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, updated_dst_buffer, CL_TRUE,
                                     0, updated_buffer_size_in_bytes,
                                     updated_output_data.data(), 0, nullptr,
                                     nullptr));
  for (unsigned i = 0; i < global_size + num_extra_elements; ++i) {
    EXPECT_EQ(updated_output_data[i], (i < global_size) ? input_data[i] : 0x42)
        << "Result mismatch at index:" << i;
  }

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, CheckUpdatePersists) {
  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results for the updated buffers.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, updated_output_data);

  // Now zero the output buffer and enqueue the command buffer again, this time
  // with no mutable config to check the update is persistent.
  constexpr cl_int zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, updated_dst_buffer, &zero,
                                     sizeof(cl_int), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> persistent_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      persistent_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, FillThenNDRange) {
  // Create a new buffer to fill.
  cl_int error = CL_SUCCESS;
  cl_mem extra_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);

  // Enqueue a fill to the command buffer to fill this buffer. It doesn't
  // matter that we don't actually do anything with the result, the point here
  // is to test we can have other commands in the command buffer with the
  // mutable dispatch.
  const cl_int zero = 0x0;
  EXPECT_SUCCESS(clCommandFillBufferKHR(
      command_buffer, nullptr, extra_buffer, &zero, sizeof(cl_int), 0,
      data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  // Now enqueue the mutable dispatch.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, NDRangeThenFill) {
  // Enqueue the mutable dispatch.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Create a new buffer to fill.
  cl_int error = CL_SUCCESS;
  cl_mem extra_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);

  // Enqueue a fill to the command buffer to fill this buffer. It doesn't
  // matter that we don't actually do anything with the result, the point here
  // is to test we can have other commands in the command buffer with the
  // mutable dispatch.
  const cl_int zero = 0x0;
  EXPECT_SUCCESS(clCommandFillBufferKHR(
      command_buffer, nullptr, extra_buffer, &zero, sizeof(cl_int), 0,
      data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, FillTwiceThenNDRange) {
  // Create a new buffer to fill.
  cl_int error = CL_SUCCESS;
  cl_mem extra_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);

  // Enqueue two fills to the command buffer to fill this buffer. It doesn't
  // matter that we don't actually do anything with the result, the point here
  // is to test we can have other commands in the command buffer with the
  // mutable dispatch.
  const cl_int zero = 0x0;
  EXPECT_SUCCESS(clCommandFillBufferKHR(
      command_buffer, nullptr, extra_buffer, &zero, sizeof(cl_int), 0,
      data_size_in_bytes, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clCommandFillBufferKHR(
      command_buffer, nullptr, extra_buffer, &zero, sizeof(cl_int), 0,
      data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  // Now enqueue the mutable dispatch.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, CopyBufferThenNDRange) {
  // Create two new buffers to fill.
  cl_int error = CL_SUCCESS;
  cl_mem first_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  cl_mem second_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);

  // Enqueue a copy between these extra buffers. It doesn't
  // matter that we don't actually do anything with the result, the point here
  // is to test we can have other commands in the command buffer with the
  // mutable dispatch.
  ASSERT_SUCCESS(clCommandCopyBufferKHR(
      command_buffer, nullptr, first_extra_buffer, second_extra_buffer, 0, 0,
      data_size_in_bytes, 0, nullptr, nullptr, nullptr));

  // Now enqueue the mutable dispatch.

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, CopyBufferRectThenNDRange) {
  // Create two new buffers to fill.
  cl_int error = CL_SUCCESS;
  cl_mem first_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  cl_mem second_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);

  // Enqueue a rectangular copy between these extra buffers. It doesn't
  // matter that we don't actually do anything with the result, the point here
  // is to test we can have other commands in the command buffer with the
  // mutable dispatch.
  const size_t origin[]{0, 0, 0};
  const size_t region[]{data_size_in_bytes, 1, 1};
  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, first_extra_buffer, second_extra_buffer, origin,
      origin, region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr));

  // Now enqueue the mutable dispatch.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

TEST_F(CommandBufferMutableBufferArgTest, RegularNDRangeThenMutableNDRange) {
  // Create two new buffers.
  cl_int error = CL_SUCCESS;
  cl_mem first_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  cl_mem second_extra_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);

  // Enqueue a (non-mutable) nd range to parallel copy between these buffers. It
  // doesn't matter that we don't actually do anything with the result, the
  // point here is to test we can have other commands in the command buffer with
  // the mutable dispatch.
  cl_kernel second_parallel_copy_kernel =
      clCreateKernel(program, "parallel_copy", &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clSetKernelArg(second_parallel_copy_kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&first_extra_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(second_parallel_copy_kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&second_extra_buffer)));

  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, second_parallel_copy_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, nullptr));

  // Now enqueue the mutable dispatch.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new output buffer.
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the output buffer.
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the output buffer.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_extra_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_extra_buffer));
  EXPECT_SUCCESS(clReleaseKernel(second_parallel_copy_kernel));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

// Test fixture checking we can update kernel arguments that are buffers in
// multiple dispatches within the same command queue.
class CommandBufferMultiMutableBufferArgTest
    : public CommandBufferMutableBufferArgTest {
 public:
  CommandBufferMultiMutableBufferArgTest()
      : CommandBufferMutableBufferArgTest(),
        second_input_data(global_size),
        second_output_data(global_size) {}

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandBufferMutableBufferArgTest::SetUp());
    // Enqueue the first mutable dispatch.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

    // Create a second kernel which is the same as the first one.
    cl_int error = CL_SUCCESS;
    second_parallel_copy_kernel =
        clCreateKernel(program, "parallel_copy", &error);
    EXPECT_SUCCESS(error);

    // Create the buffers for the second kernel.
    second_src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    second_dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);

    // Fill the input buffer with random numbers.
    ucl::Environment::instance->GetInputGenerator().GenerateData(
        second_input_data);
    EXPECT_SUCCESS(clEnqueueWriteBuffer(
        command_queue, second_src_buffer, CL_TRUE, 0, data_size_in_bytes,
        second_input_data.data(), 0, nullptr, nullptr));

    // Set up the initial kernel arguments.
    EXPECT_SUCCESS(clSetKernelArg(second_parallel_copy_kernel, 0,
                                  sizeof(cl_mem),
                                  static_cast<void *>(&second_src_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(second_parallel_copy_kernel, 1,
                                  sizeof(cl_mem),
                                  static_cast<void *>(&second_dst_buffer)));
    EXPECT_SUCCESS(error);

    // Enqueue a mutable dispatch to the command buffer.
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties,
        second_parallel_copy_kernel, 1, nullptr, &global_size, nullptr, 0,
        nullptr, nullptr, &second_command_handle));
  }

  void TearDown() override {
    // Cleanup.
    if (nullptr != second_parallel_copy_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(second_parallel_copy_kernel));
    }

    if (nullptr != second_src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(second_src_buffer));
    }

    if (nullptr != second_dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(second_dst_buffer));
    }

    CommandBufferMutableBufferArgTest::TearDown();
  }

  std::vector<cl_int> second_input_data;
  std::vector<cl_int> second_output_data;
  cl_kernel second_parallel_copy_kernel = nullptr;
  cl_mem second_src_buffer = nullptr;
  cl_mem second_dst_buffer = nullptr;
  cl_mutable_command_khr second_command_handle = nullptr;
};

TEST_F(CommandBufferMultiMutableBufferArgTest, UpdateSingleDispatch) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new input buffer for the first nd range filling it with random
  // values.
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(second_input_data, second_output_data);

  // Now try and enqueue the command buffer updating the output buffer of the
  // first nd range.
  const cl_mutable_dispatch_arg_khr arg{
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the first nd range in the command buffer got its input buffer
  // updated.
  std::vector<cl_int> first_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      first_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(updated_input_data, first_updated_output_data);

  // Check the second nd range in the command buffer didn't get its input buffer
  // updated.
  std::vector<cl_int> second_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(second_input_data, second_updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
}

TEST_F(CommandBufferMultiMutableBufferArgTest, UpdateMultipleDispatches) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = CL_SUCCESS;
  // Create a new input buffer both nd ranges filling them with random values.
  cl_mem first_updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> first_updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      first_updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, first_updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      first_updated_input_data.data(), 0, nullptr, nullptr));

  cl_mem second_updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);
  std::vector<cl_int> second_updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      second_updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, second_updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_updated_input_data.data(), 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(second_input_data, second_output_data);

  // Now try and enqueue the command buffer updating the input buffer of both
  // nd ranges.
  const cl_mutable_dispatch_arg_khr first_arg{
      0, sizeof(cl_mem), static_cast<void *>(&first_updated_src_buffer)};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_dispatch_arg_khr second_arg{
      0, sizeof(cl_mem), static_cast<void *>(&second_updated_src_buffer)};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      second_command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  cl_mutable_dispatch_config_khr dispatch_configs[]{first_dispatch_config,
                                                    second_dispatch_config};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, dispatch_configs};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that both nd ranges in the command buffer got their input buffers
  // updated.
  std::vector<cl_int> first_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      first_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(first_updated_input_data, first_updated_output_data);

  std::vector<cl_int> second_updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(second_updated_input_data, second_updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(first_updated_src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(second_updated_src_buffer));
}

TEST_F(MutableDispatchTest, UpdateConstantBuffer) {
  // Set up the kernel. This contrived example just does a copy in parallel
  // between two buffers, allowing us to check we can update the arguments.
  const char *code = R"OpenCLC(
  kernel void parallel_copy(constant int *src, global int *dst) {
    size_t gid = get_global_id(0);
    dst[gid] = src[gid];
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  // Build the kernel.
  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                ucl::buildLogCallback, nullptr));

  cl_kernel parallel_copy_kernel =
      clCreateKernel(program, "parallel_copy", &error);
  EXPECT_SUCCESS(error);

  // Create initial buffers for the input and output.
  constexpr size_t global_size = 256;
  const size_t data_size_in_bytes = global_size * sizeof(cl_int);

  cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Fill the input buffer with random numbers.
  std::vector<cl_int> input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      data_size_in_bytes, input_data.data(), 0,
                                      nullptr, nullptr));

  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(clSetKernelArg(parallel_copy_kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&src_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(parallel_copy_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Create command-buffer with mutable flag so we can update it
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  cl_mutable_command_khr command_handle;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, parallel_copy_kernel, 1,
      nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a new input buffer filling it with random values.
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);

  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);

  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit queue flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Now try and enqueue the command buffer updating the input buffer.
  const cl_mutable_dispatch_arg_khr arg{
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(updated_input_data, output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseKernel(parallel_copy_kernel));
  EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
}

// Test fixture for checking we can update __local arguments to a kernel.
// By the definition of local buffers there isn't really any way to verify that
// the buffer has actually changed value, but we can still check that the OpenCL
// API calls succeed.
class CommandBufferMutableLocalBufferArgTest : public MutableDispatchTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());
    // Set up the kernel. This contrived example doesn't actually do anything
    // but allows us to try and updated an __local arguments.
    const char *code = R"OpenCLC(
    kernel void update_local_arg(local int *arg) {}
)OpenCLC";
    const size_t code_length = std::strlen(code);

    // Build the kernel.
    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));

    kernel = clCreateKernel(program, "update_local_arg", &error);
    EXPECT_SUCCESS(error);

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);

    // Set the kernel's local argument to some initial size.
    EXPECT_SUCCESS(clSetKernelArg(kernel, 0, 64, nullptr));

    // Enqueue a mutable dispatch to the command buffer.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
        &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

    // Finalize the command buffer.
    EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  }

  void TearDown() override {
    // Cleanup.
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    MutableDispatchTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mutable_command_khr command_handle = nullptr;
  static constexpr size_t global_size = 256;
  static constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
};

TEST_F(CommandBufferMutableLocalBufferArgTest, UpdateOnce) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Flush the command queue (see CA-3232).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Update the local buffer size.
  const cl_mutable_dispatch_arg_khr arg{0, 32, nullptr};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
}

TEST_F(CommandBufferMutableLocalBufferArgTest, UpdateTwice) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Flush the command queue (see CA-3232).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Update the local buffer size.
  const cl_mutable_dispatch_arg_khr arg{0, 32, nullptr};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Flush the command queue (see CA-3232).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Update the local buffer size a second time.
  const cl_mutable_dispatch_arg_khr arg_2{0, 32, nullptr};
  const cl_mutable_dispatch_config_khr dispatch_config_2{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg_2,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config_2{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &dispatch_config_2};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config_2));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
}

// Test fixture for checking we can update global buffers to and from NULL.
//
// This test fixture makes use of the `NULL` macros which is only defined
// after OpenCL 2.0. ComputeAorta currently doesn't define this macro
// (see CA-3044). When support for `NULL` is added we should renable these
// tests.
class DISABLED_CommandBufferMutableNullArgTest : public MutableDispatchTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());

    // NULL isn't defined for OpenCL versions before 2.0.
    if (!UCL::isDeviceVersionAtLeast({2, 0})) {
      GTEST_SKIP();
    }

    // Set up the kernel. This contrived example just checks whether its input
    // buffer is null and broadcasts the result to the output buffer.
    const char *code = R"OpenCLC(
    kernel void is_input_null(global int *src, global int *dst) {
    size_t gid = get_global_id(0);
    dst[gid] = (src == NULL);
  }
)OpenCLC";
    const size_t code_length = std::strlen(code);

    // Build the kernel.
    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));

    null_test_kernel = clCreateKernel(program, "is_input_null", &error);
    EXPECT_SUCCESS(error);

    const size_t data_size_in_bytes = global_size * sizeof(cl_int);

    src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Fill the output buffer with some known value.
    const cl_int forty_two = 0x42;
    EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &forty_two,
                                       sizeof(cl_int), 0, data_size_in_bytes, 0,
                                       nullptr, nullptr));
    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);
  }

  void TearDown() override {
    // Cleanup.
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (nullptr != src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }
    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    if (nullptr != null_test_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(null_test_kernel));
    }
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    MutableDispatchTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel null_test_kernel = nullptr;
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mutable_command_khr command_handle = nullptr;
  static constexpr size_t global_size = 256;
  static constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
};

TEST_F(DISABLED_CommandBufferMutableNullArgTest,
       UpdateInputBufferToNullByValue) {
  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&src_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, null_test_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));

  // Update the input to null and enqueue a second time.
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem), nullptr};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results again to see if we updated the argument to null.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));
}

TEST_F(DISABLED_CommandBufferMutableNullArgTest,
       UpdateInputBufferToNullByAddress) {
  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&src_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Enqueue a mutable dispatch to the command buffer.
  cl_mutable_command_khr command_handle;
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, null_test_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));

  // Update the input to pointer to null.
  const cl_int *null = nullptr;
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem),
                                        static_cast<void *>(&null)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));
}

TEST_F(DISABLED_CommandBufferMutableNullArgTest,
       UpdateInputBufferFromNullByValue) {
  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(
      clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer), nullptr));
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Enqueue a mutable dispatch to the command buffer.
  cl_mutable_command_khr command_handle;
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, null_test_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));

  // Update the input to null.
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem),
                                        static_cast<void *>(&src_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));
}

TEST_F(DISABLED_CommandBufferMutableNullArgTest,
       UpdateInputBufferFromNullByAddress) {
  // Set up the initial kernel arguments.
  const cl_int *null = nullptr;
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&null)));
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Enqueue a mutable dispatch to the command buffer.
  cl_mutable_command_khr command_handle;
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, null_test_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));

  // Update the input to null.
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem),
                                        static_cast<void *>(&src_buffer)};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));
}

TEST_F(DISABLED_CommandBufferMutableNullArgTest, CheckUpdatePersists) {
  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer),
                                static_cast<void *>(&src_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Enqueue a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, null_test_kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));

  // Update the input to null and enqueue a second time.
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem), nullptr};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results again to see if we updated the argument to null.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));

  // Now zero the output buffer and enqueue the command buffer again, this time
  // with no mutable config to check the update is persistent.
  constexpr cl_int zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                     sizeof(cl_int), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> persistent_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      persistent_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(persistent_output_data, std::vector<cl_int>(global_size, 0x1));
}

class DISABLED_CommandBufferMultiMutableNullArgTest
    : public DISABLED_CommandBufferMutableNullArgTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(
        DISABLED_CommandBufferMutableNullArgTest::SetUp());

    // Set up the initial kernel arguments of the first ND range.
    EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 0, sizeof(src_buffer),
                                  static_cast<void *>(&src_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(null_test_kernel, 1, sizeof(dst_buffer),
                                  static_cast<void *>(&dst_buffer)));

    // Enqueue a mutable dispatch to the command buffer.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, null_test_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

    // Create a second kernel which is the same as the first one.
    cl_int error = CL_SUCCESS;
    second_null_test_kernel = clCreateKernel(program, "is_input_null", &error);
    EXPECT_SUCCESS(error);

    // Create the input and output buffers of the second kernel.
    second_src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    second_dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);
    EXPECT_SUCCESS(error);

    // Set up the initial kernel arguments of the second nd range.
    EXPECT_SUCCESS(clSetKernelArg(second_null_test_kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&second_src_buffer)));
    EXPECT_SUCCESS(clSetKernelArg(second_null_test_kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&second_dst_buffer)));

    // Enqueue a mutable dispatch to the command buffer.
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, second_null_test_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr,
        &second_command_handle));

    // Finalize the command buffer.
    EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  }

  void TearDown() override {
    if (nullptr != second_dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(second_dst_buffer));
    }
    if (nullptr != second_src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(second_src_buffer));
    }
    if (nullptr != second_null_test_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(second_null_test_kernel));
    }
    DISABLED_CommandBufferMutableNullArgTest::TearDown();
  }

  cl_kernel second_null_test_kernel = nullptr;
  cl_mem second_src_buffer = nullptr;
  cl_mem second_dst_buffer = nullptr;
  cl_mutable_command_khr second_command_handle = nullptr;
};

TEST_F(DISABLED_CommandBufferMultiMutableNullArgTest, UpdateSingleDispatch) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  std::vector<cl_int> second_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));
  EXPECT_EQ(second_output_data, std::vector<cl_int>(global_size, 0x0));

  // Update the input of the first nd range to null and enqueue a second time.
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem), nullptr};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results again to see if we updated the argument to the first nd
  // range to null and not the second nd range.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));
  EXPECT_EQ(second_output_data, std::vector<cl_int>(global_size, 0x0));
}

TEST_F(DISABLED_CommandBufferMultiMutableNullArgTest,
       UpdateMultipleDispatches) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  std::vector<cl_int> second_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x0));
  EXPECT_EQ(second_output_data, std::vector<cl_int>(global_size, 0x0));

  // Update the input of both nd ranges to null and enqueue a second time.
  const cl_mutable_dispatch_arg_khr first_arg{0, sizeof(cl_mem), nullptr};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_dispatch_arg_khr second_arg{0, sizeof(cl_mem), nullptr};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      second_command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  cl_mutable_dispatch_config_khr dispatch_configs[]{first_dispatch_config,
                                                    second_dispatch_config};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, dispatch_configs};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results again to see if we updated the argument to the first nd
  // range to null and not the second nd range.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(output_data, std::vector<cl_int>(global_size, 0x1));
  EXPECT_EQ(second_output_data, std::vector<cl_int>(global_size, 0x1));
}

// Test fixture for checking we can update kernel arguments passed by
// value.
class CommandBufferMutablePODArgTest : public MutableDispatchTest {
 public:
  CommandBufferMutablePODArgTest()
      : input_value(ucl::Environment::instance->GetInputGenerator()
                        .GenerateInt<cl_int>()),
        output_data(global_size) {}

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());
    // Set up the kernel. This contrived example just broadcasts some input
    // value into its output buffer.
    const char *code = R"OpenCLC(
  kernel void broadcast(int src, global int *dst) {
    int gid = get_global_id(0);
    dst[gid] = src;
  }
)OpenCLC";
    const size_t code_length = std::strlen(code);

    // Build the kernel.
    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));

    broadcast_kernel = clCreateKernel(program, "broadcast", &error);
    EXPECT_SUCCESS(error);

    // Set up the single output buffer.
    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Set up the initial kernel arguments.
    EXPECT_SUCCESS(
        clSetKernelArg(broadcast_kernel, 0, sizeof(cl_int), &input_value));
    EXPECT_SUCCESS(clSetKernelArg(broadcast_kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);

    // Enqueue a mutable dispatch to the command buffer.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, broadcast_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));
  }

  void TearDown() override {
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    if (nullptr != broadcast_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(broadcast_kernel));
    }
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    MutableDispatchTest::TearDown();
  }

  cl_int input_value;
  std::vector<cl_int> output_data;
  cl_program program = nullptr;
  cl_kernel broadcast_kernel = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mutable_command_khr command_handle = nullptr;
  static constexpr size_t global_size = 256;
  static constexpr const size_t data_size_in_bytes =
      global_size * sizeof(cl_int);
};

TEST_F(CommandBufferMutablePODArgTest, InvalidArgSize) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Now try and enqueue the command buffer updating the input value.
  const auto updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_int4),
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_EQ_ERRCODE(CL_INVALID_ARG_SIZE, clUpdateMutableCommandsKHR(
                                             command_buffer, &mutable_config));
}

TEST_F(CommandBufferMutablePODArgTest, UpdateInputOnce) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value.
  const auto updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  for (const auto &val : output_data) {
    EXPECT_EQ(updated_input_value, val);
  }
}

TEST_F(CommandBufferMutablePODArgTest, UpdateInputTwice) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value.
  const auto first_updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  const cl_mutable_dispatch_arg_khr first_arg{0, sizeof(cl_int),
                                              &first_updated_input_value};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr first_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &first_dispatch_config};
  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &first_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(first_updated_input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value a second
  // time.
  const auto second_updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  const cl_mutable_dispatch_arg_khr second_arg{0, sizeof(cl_int),
                                               &second_updated_input_value};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr second_mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
      &second_dispatch_config};
  EXPECT_SUCCESS(
      clUpdateMutableCommandsKHR(command_buffer, &second_mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  for (const auto &val : output_data) {
    EXPECT_EQ(second_updated_input_value, val);
  }
}

TEST_F(CommandBufferMutablePODArgTest, CheckUpdatePersists) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value.
  const auto updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(updated_input_value, val);
  }

  // Now zero the output buffer and enqueue the command buffer again, this time
  // with no mutable config to check the update is persistent.
  constexpr cl_int zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                     sizeof(cl_int), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  std::vector<cl_int> persistent_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      persistent_output_data.data(), 0, nullptr, nullptr));

  for (const auto &val : persistent_output_data) {
    EXPECT_EQ(updated_input_value, val);
  }
}

class CommandBufferMultiMutablePODArgTest
    : public CommandBufferMutablePODArgTest {
 public:
  CommandBufferMultiMutablePODArgTest()
      : CommandBufferMutablePODArgTest(),
        second_input_value(ucl::Environment::instance->GetInputGenerator()
                               .GenerateInt<cl_int>()),
        second_output_data(global_size) {}

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandBufferMutablePODArgTest::SetUp());

    // Create a second kernel which is the same as the first one.
    cl_int error = CL_SUCCESS;
    second_broadcast_kernel = clCreateKernel(program, "broadcast", &error);
    EXPECT_SUCCESS(error);

    // Create the buffer for the second kernel.
    second_dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       data_size_in_bytes, nullptr, &error);

    // Set up the initial kernel arguments.
    EXPECT_SUCCESS(clSetKernelArg(second_broadcast_kernel, 0, sizeof(cl_int),
                                  &second_input_value));
    EXPECT_SUCCESS(clSetKernelArg(second_broadcast_kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&second_dst_buffer)));

    // Enqueue a mutable dispatch to the command buffer.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, second_broadcast_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr,
        &second_command_handle));
  }

  void TearDown() override {
    if (nullptr != second_dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(second_dst_buffer));
    }
    if (nullptr != second_broadcast_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(second_broadcast_kernel));
    }

    CommandBufferMutablePODArgTest::TearDown();
  }

  cl_int second_input_value;
  std::vector<cl_int> second_output_data;
  cl_kernel second_broadcast_kernel = nullptr;
  cl_mem second_dst_buffer = nullptr;
  cl_mutable_command_khr second_command_handle = nullptr;
};

TEST_F(CommandBufferMultiMutablePODArgTest, UpdateSingleDispatch) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffers after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(input_value, val);
  }

  for (const auto &val : second_output_data) {
    EXPECT_EQ(second_input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value to the
  // first nd range only.
  const auto updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the first nd range had its input value updated and the second nd
  // range didn't.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  for (const auto &val : output_data) {
    EXPECT_EQ(updated_input_value, val);
  }

  for (const auto &val : second_output_data) {
    EXPECT_EQ(second_input_value, val);
  }
}

TEST_F(CommandBufferMultiMutablePODArgTest, UpdateMultipleDispatches) {
  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffers after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (const auto &val : output_data) {
    EXPECT_EQ(input_value, val);
  }

  for (const auto &val : second_output_data) {
    EXPECT_EQ(second_input_value, val);
  }

  // Now try and enqueue the command buffer updating the input value to both nd
  // ranges.
  const auto first_updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  const cl_mutable_dispatch_arg_khr first_arg{0, sizeof(cl_int),
                                              &first_updated_input_value};
  const cl_mutable_dispatch_config_khr first_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &first_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const auto second_updated_input_value =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  const cl_mutable_dispatch_arg_khr second_arg{0, sizeof(cl_int),
                                               &second_updated_input_value};
  const cl_mutable_dispatch_config_khr second_dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      second_command_handle,
      1,
      0,
      0,
      0,
      &second_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_dispatch_config_khr mutable_configs[]{first_dispatch_config,
                                                   second_dispatch_config};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, mutable_configs};

  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check both nd range commands had their inputs updated.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, second_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      second_output_data.data(), 0, nullptr, nullptr));

  for (const auto &val : output_data) {
    EXPECT_EQ(first_updated_input_value, val);
  }

  for (const auto &val : second_output_data) {
    EXPECT_EQ(second_updated_input_value, val);
  }
}

// Test fixture for updating multiple arguments of the POD type.
class CommandBufferMutablePODMultiArgTest : public MutableDispatchTest {
 public:
  CommandBufferMutablePODMultiArgTest()
      : input_x_value(ucl::Environment::instance->GetInputGenerator()
                          .GenerateInt<cl_int>()),
        input_y_value(ucl::Environment::instance->GetInputGenerator()
                          .GenerateInt<cl_int>()),
        output_data(global_size) {}

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());
    // Set up the kernel. This contrived example just broadcasts some input
    // values into its output buffer by building a vector with the two inputs
    // for each work item.
    const char *code = R"OpenCLC(
  kernel void broadcast_pair(int src_x, int src_y, global int2 *dst) {
    int gid = get_global_id(0);
    dst[gid].x = src_x;
    dst[gid].y = src_y;
  }
)OpenCLC";
    const size_t code_length = std::strlen(code);

    // Build the kernel.
    cl_int error = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &code, &code_length, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));

    broadcast_kernel = clCreateKernel(program, "broadcast_pair", &error);
    EXPECT_SUCCESS(error);

    // Set up the single output buffer.
    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    EXPECT_SUCCESS(error);

    // Set up the initial kernel arguments.
    EXPECT_SUCCESS(
        clSetKernelArg(broadcast_kernel, 0, sizeof(cl_int), &input_x_value));
    EXPECT_SUCCESS(
        clSetKernelArg(broadcast_kernel, 1, sizeof(cl_int), &input_y_value));
    EXPECT_SUCCESS(clSetKernelArg(broadcast_kernel, 2, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);

    // Enqueue a mutable dispatch to the command buffer.
    cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
        CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
        CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties, broadcast_kernel, 1,
        nullptr, &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

    // Finalize the command buffer.
    EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  }

  void TearDown() override {
    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (nullptr != broadcast_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(broadcast_kernel));
    }
    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    MutableDispatchTest::TearDown();
  }

  cl_int input_x_value;
  cl_int input_y_value;
  UCL::AlignedBuffer<cl_int2> output_data;
  cl_program program = nullptr;
  cl_kernel broadcast_kernel = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mutable_command_khr command_handle = nullptr;
  static constexpr size_t global_size = 256;
  static constexpr const size_t data_size_in_bytes =
      global_size * sizeof(cl_int2);
};

TEST_F(CommandBufferMutablePODMultiArgTest,
       UpdateTwoInputsSameMutableDispatchConfig) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (unsigned i = 0; i < output_data.size(); ++i) {
    EXPECT_EQ(input_x_value, output_data[i].x);
    EXPECT_EQ(input_y_value, output_data[i].y);
  }

  // Now try and enqueue the command buffer updating the output buffer.
  cl_int updated_input_x_data =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  cl_int updated_input_y_data =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();

  const cl_mutable_dispatch_arg_khr arg_1{0, sizeof(cl_int),
                                          &updated_input_x_data};
  const cl_mutable_dispatch_arg_khr arg_2{1, sizeof(cl_int),
                                          &updated_input_y_data};
  cl_mutable_dispatch_arg_khr args[] = {arg_1, arg_2};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      2,
      0,
      0,
      0,
      args,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  for (unsigned i = 0; i < output_data.size(); ++i) {
    EXPECT_EQ(updated_input_x_data, output_data[i].x);
    EXPECT_EQ(updated_input_y_data, output_data[i].y);
  }
}

TEST_F(CommandBufferMutablePODMultiArgTest,
       UpdateTwoInputsDifferentMutableDispatchConfigs) {
  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  for (unsigned i = 0; i < output_data.size(); ++i) {
    EXPECT_EQ(input_x_value, output_data[i].x);
    EXPECT_EQ(input_y_value, output_data[i].y);
  }

  // Now try and enqueue the command buffer updating the output buffer.
  cl_int updated_input_x_data =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();
  cl_int updated_input_y_data =
      ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_int>();

  const cl_mutable_dispatch_arg_khr arg_1{0, sizeof(cl_int),
                                          &updated_input_x_data};
  const cl_mutable_dispatch_arg_khr arg_2{1, sizeof(cl_int),
                                          &updated_input_y_data};
  const cl_mutable_dispatch_config_khr dispatch_config_1{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg_1,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_dispatch_config_khr dispatch_config_2{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg_2,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_dispatch_config_khr dispatch_configs[]{dispatch_config_1,
                                                    dispatch_config_2};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, dispatch_configs};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  for (unsigned i = 0; i < output_data.size(); ++i) {
    EXPECT_EQ(updated_input_x_data, output_data[i].x);
    EXPECT_EQ(updated_input_y_data, output_data[i].y);
  }
}

// Parent class for tests checking we can update struct kernel arguments passed
// by value.
class CommandBufferMutableStructArgTest : public MutableDispatchTest {};

TEST_F(CommandBufferMutableStructArgTest, UpdateInputOnce) {
  typedef struct _test_struct {
    int x;
    float y;
    char z;
  } test_struct;
  // Set up the kernel. This contrived example just braodcasts the struct in its
  // input to the output buffer.
  const char *code = R"OpenCLC(
  typedef struct _test_struct {
    int x;
    float y;
    char z;
  } test_struct;

  __kernel void broadcast(test_struct src, __global test_struct *dst) {
    dst[0].x = src.x;
    dst[0].y = src.y;
    dst[0].z = src.z;
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  // Build the kernel.
  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "broadcast", &error);
  EXPECT_SUCCESS(error);

  // Set up the single output buffer.
  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     sizeof(test_struct), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Pick the initial input value.
  const test_struct first_value{42, 1.0f, 'a'};

  // Set up the initial kernel arguments.
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(test_struct), &first_value));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(dst_buffer),
                                static_cast<void *>(&dst_buffer)));

  // Create command-buffer with mutable flag so we can update it
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  // Enqueue a mutable dispatch to the command buffer.
  cl_mutable_command_khr command_handle;
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  const size_t one = 1;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr, &one,
      nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the value of the output buffer after the first enqueue.
  test_struct first_result{-1, -1.0f, 'z'};
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     sizeof(test_struct), &first_result, 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(first_value.x, first_result.x);
  EXPECT_EQ(first_value.y, first_result.y);
  EXPECT_EQ(first_value.z, first_result.z);

  // Now try and enqueue the command buffer updating the output buffer.
  const test_struct second_value{99, 42.0f, 'j'};
  const cl_mutable_dispatch_arg_khr arg{0, sizeof(test_struct), &second_value};
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
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  // Check the results.
  test_struct second_result{-1, -1.0f, 'z'};
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     sizeof(test_struct), &second_result, 0,
                                     nullptr, nullptr));
  EXPECT_EQ(second_value.x, second_result.x);
  EXPECT_EQ(second_value.y, second_result.y);
  EXPECT_EQ(second_value.z, second_result.z);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}
