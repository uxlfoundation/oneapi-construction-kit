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
#include <CL/cl_ext.h>
#include <common.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define CL_CHECK(ERROR)                            \
  if (ERROR) {                                     \
    std::cerr << "OpenCL error: " << ERROR << "\n" \
              << "at line: " << __LINE__ << "\n";  \
    return ERROR;                                  \
  }

int main(const int argc, const char **argv) {
  const char *platform_name = NULL;
  const char *device_name = NULL;
  parseArguments(argc, argv, &platform_name, &device_name);

  cl_platform_id platform = selectPlatform(platform_name);
  cl_device_id device = selectDevice(platform, device_name);

  size_t extension_names_size = 0;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr,
                           &extension_names_size));
  std::string extension_names(extension_names_size, '\0');

  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, extension_names.size(),
                           extension_names.data(), nullptr));

  // Mutable-dispatch extension is only reported when cl_khr_command_buffer is
  // also enabled, so don't need to check for cl_khr_command_buffer as well.
  if (std::string::npos ==
      extension_names.find("cl_khr_command_buffer_mutable_dispatch")) {
    std::cerr << "cl_khr_command_buffer_mutable_dispatch not supported "
                 "by device, skipping example.\n";
    return 0;
  }

  cl_bool device_compiler_available;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,
                           sizeof(cl_bool), &device_compiler_available,
                           nullptr));
  if (!device_compiler_available) {
    std::cerr
        << "compiler not available for selected device, skipping example.\n";
    return 0;
  }

  cl_mutable_dispatch_fields_khr mutable_capabilities;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR,
                           sizeof(mutable_capabilities), &mutable_capabilities,
                           nullptr));
  if (!(mutable_capabilities & CL_MUTABLE_DISPATCH_ARGUMENTS_KHR)) {
    std::cerr
        << "Device does not support update arguments to a mutable-dispatch, "
           "skipping example.\n";
    return 0;
  }

#define CL_GET_EXTENSION_ADDRESS(FUNC)          \
  FUNC##_fn FUNC = reinterpret_cast<FUNC##_fn>( \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC));

  CL_GET_EXTENSION_ADDRESS(clCreateCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clFinalizeCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clReleaseCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clEnqueueCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clCommandNDRangeKernelKHR);
  CL_GET_EXTENSION_ADDRESS(clUpdateMutableCommandsKHR);

#undef GET_EXTENSION_ADDRESS

  cl_int error;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  CL_CHECK(error);

  const char *code = R"OpenCLC(
  kernel void vector_addition(global int* input_A, global int* input_B,
                              global int* output) {
    size_t index = get_global_id(0);
    output[index] = input_A[index] + input_B[index];
  }
  )OpenCLC";
  const size_t length = std::strlen(code);

  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &length, &error);
  CL_CHECK(error);

  CL_CHECK(clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "vector_addition", &error);
  CL_CHECK(error);

  // Set the parameters of the frames
  constexpr size_t iterations = 60;
  constexpr size_t elem_size = sizeof(cl_int);
  constexpr size_t frame_width = 32;
  constexpr size_t frame_count = frame_width * frame_width;
  constexpr size_t frame_size = frame_count * elem_size;

  // Create the buffer to swap between even and odd kernel iterations
  cl_mem input_A_buffers[2] = {nullptr, nullptr};
  cl_mem input_B_buffers[2] = {nullptr, nullptr};
  cl_mem output_buffers[2] = {nullptr, nullptr};

  for (size_t i = 0; i < 2; i++) {
    input_A_buffers[i] =
        clCreateBuffer(context, CL_MEM_READ_ONLY, frame_size, nullptr, &error);
    CL_CHECK(error);

    input_B_buffers[i] =
        clCreateBuffer(context, CL_MEM_READ_ONLY, frame_size, nullptr, &error);
    CL_CHECK(error);

    output_buffers[i] =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, frame_size, nullptr, &error);
    CL_CHECK(error);
  }

  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  CL_CHECK(error);

  // Create command-buffer with mutable flag so we can update it
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  CL_CHECK(error);

  CL_CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                          static_cast<void *>(&input_A_buffers[0])));
  CL_CHECK(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                          static_cast<void *>(&input_B_buffers[0])));
  CL_CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem),
                          static_cast<void *>(&output_buffers[0])));

  // Instruct the nd-range command to allow for mutable kernel arguments
  cl_ndrange_kernel_command_properties_khr mutable_properties[] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};

  // Create command handle for mutating nd-range command
  cl_mutable_command_khr command_handle = nullptr;

  // Add the nd-range kernel command
  error = clCommandNDRangeKernelKHR(command_buffer, nullptr, mutable_properties,
                                    kernel, 1, nullptr, &frame_count, nullptr,
                                    0, nullptr, nullptr, &command_handle);
  CL_CHECK(error);

  CL_CHECK(clFinalizeCommandBufferKHR(command_buffer));

  // Prepare for random input generation
  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  std::uniform_int_distribution<cl_int> random_distribution{
      std::numeric_limits<cl_int>::min() / 2,
      std::numeric_limits<cl_int>::max() / 2};

  // Iterate over each frame
  for (size_t i = 0; i < iterations; i++) {
    // Set the buffers for the current frame
    cl_mem input_A_buffer = input_A_buffers[i % 2];
    cl_mem input_B_buffer = input_B_buffers[i % 2];
    cl_mem output_buffer = output_buffers[i % 2];

    // Generate input A data
    std::vector<cl_int> input_a(frame_count);
    std::generate(std::begin(input_a), std::end(input_a),
                  [&]() { return random_distribution(random_engine); });

    // Write the generated data to the input A buffer
    error =
        clEnqueueWriteBuffer(command_queue, input_A_buffer, CL_FALSE, 0,
                             frame_size, input_a.data(), 0, nullptr, nullptr);
    CL_CHECK(error);

    // Generate input B data
    std::vector<cl_int> input_b(frame_count);
    std::generate(std::begin(input_b), std::end(input_b),
                  [&]() { return random_distribution(random_engine); });

    // Write the generated data to the input B buffer
    error =
        clEnqueueWriteBuffer(command_queue, input_B_buffer, CL_FALSE, 0,
                             frame_size, input_b.data(), 0, nullptr, nullptr);
    CL_CHECK(error);

    // If not executing the first frame
    if (i != 0) {
      // Configure the mutable configuration to update the kernel arguments
      const cl_mutable_dispatch_arg_khr arg_0{
          0, sizeof(cl_mem), static_cast<void *>(&input_A_buffer)};
      const cl_mutable_dispatch_arg_khr arg_1{
          1, sizeof(cl_mem), static_cast<void *>(&input_B_buffer)};
      const cl_mutable_dispatch_arg_khr arg_2{
          2, sizeof(cl_mem), static_cast<void *>(&output_buffer)};
      const cl_mutable_dispatch_arg_khr args[] = {arg_0, arg_1, arg_2};
      const cl_mutable_dispatch_config_khr dispatch_config{
          CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
          nullptr,
          command_handle,
          3 /* num_args */,
          0 /* num_svm_arg */,
          0 /* num_exec_infos */,
          0 /* work_dim - 0 means no change to dimensions */,
          args /* arg_list */,
          nullptr /* arg_svm_list - nullptr means no change*/,
          nullptr /* exec_info_list */,
          nullptr /* global_work_offset */,
          nullptr /* global_work_size */,
          nullptr /* local_work_size */};
      const cl_mutable_base_config_khr mutable_config{
          CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1,
          &dispatch_config};

      // Update the command buffer with the mutable configuration
      error = clUpdateMutableCommandsKHR(command_buffer, &mutable_config);
      CL_CHECK(error);
    }

    // Enqueue the command buffer
    error = clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0, nullptr,
                                      nullptr);
    CL_CHECK(error);

    // Allocate memory for the output data
    std::vector<cl_int> output(frame_count);

    // Read the output data from the output buffer
    error = clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                frame_size, output.data(), 0, nullptr, nullptr);
    CL_CHECK(error);

    // Flush and execute the read buffer
    error = clFinish(command_queue);
    CL_CHECK(error);

    // Verify the results of the frame
    for (size_t i = 0; i < frame_count; ++i) {
      const cl_int result = input_a[i] + input_b[i];
      if (output[i] != result) {
        std::cerr << "Error: Incorrect result at index " << i << " - Expected "
                  << output[i] << " was " << result << '\n';
        std::exit(1);
      }
    }
  }

  std::cout << "Result verified\n";

  CL_CHECK(clReleaseCommandBufferKHR(command_buffer));
  for (size_t i = 0; i < 2; i++) {
    CL_CHECK(clReleaseMemObject(input_A_buffers[i]));
    CL_CHECK(clReleaseMemObject(input_B_buffers[i]));
    CL_CHECK(clReleaseMemObject(output_buffers[i]));
  }
  CL_CHECK(clReleaseCommandQueue(command_queue));
  CL_CHECK(clReleaseKernel(kernel));
  CL_CHECK(clReleaseProgram(program));
  CL_CHECK(clReleaseContext(context));
  CL_CHECK(clReleaseDevice(device));
  return 0;
}
