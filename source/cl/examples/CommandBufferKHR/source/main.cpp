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

  if (std::string::npos == extension_names.find("cl_khr_command_buffer")) {
    std::cerr << "cl_khr_command_buffer not supported by device, skipping "
                 "example.\n";
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

#define CL_GET_EXTENSION_ADDRESS(FUNC)          \
  FUNC##_fn FUNC = reinterpret_cast<FUNC##_fn>( \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC));

  CL_GET_EXTENSION_ADDRESS(clCreateCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clFinalizeCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clReleaseCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clEnqueueCommandBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clCommandCopyBufferKHR);
  CL_GET_EXTENSION_ADDRESS(clCommandNDRangeKernelKHR);

#undef GET_EXTENSION_ADDRESS

  cl_int error;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  CL_CHECK(error);

  const char *code = R"OpenCLC(
  kernel void vector_addition(global int* tile1, global int* tile2,
                              global int* res) {
    size_t index = get_global_id(0);
    res[index] = tile1[index] + tile2[index];
  }
  )OpenCLC";
  const size_t length = std::strlen(code);

  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &length, &error);
  CL_CHECK(error);

  CL_CHECK(clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "vector_addition", &error);
  CL_CHECK(error);

  constexpr size_t frame_count = 60;
  constexpr size_t frame_elements = 1024;
  constexpr size_t frame_size = frame_elements * sizeof(cl_int);

  constexpr size_t tile_count = 16;
  constexpr size_t tile_elements = frame_elements / tile_count;
  constexpr size_t tile_size = tile_elements * sizeof(cl_int);

  cl_mem buffer_tile1 =
      clCreateBuffer(context, CL_MEM_READ_ONLY, tile_size, nullptr, &error);
  CL_CHECK(error);
  cl_mem buffer_tile2 =
      clCreateBuffer(context, CL_MEM_READ_ONLY, tile_size, nullptr, &error);
  CL_CHECK(error);
  cl_mem buffer_res =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, tile_size, nullptr, &error);
  CL_CHECK(error);

  CL_CHECK(clSetKernelArg(kernel, 0, sizeof(buffer_tile1),
                          static_cast<void *>(&buffer_tile1)));
  CL_CHECK(clSetKernelArg(kernel, 1, sizeof(buffer_tile2),
                          static_cast<void *>(&buffer_tile2)));
  CL_CHECK(clSetKernelArg(kernel, 2, sizeof(buffer_res),
                          static_cast<void *>(&buffer_res)));

  // CA doesn't support OOO queues.
  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  CL_CHECK(error);

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  CL_CHECK(error);

  cl_mem buffer_src1 =
      clCreateBuffer(context, CL_MEM_READ_ONLY, frame_size, nullptr, &error);
  CL_CHECK(error);
  cl_mem buffer_src2 =
      clCreateBuffer(context, CL_MEM_READ_ONLY, frame_size, nullptr, &error);
  CL_CHECK(error);
  cl_mem buffer_dst =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, frame_size, nullptr, &error);
  CL_CHECK(error);

  for (size_t tile_index = 0; tile_index < tile_count; tile_index++) {
    CL_CHECK(clCommandCopyBufferKHR(command_buffer, nullptr, buffer_src1,
                                    buffer_tile1, tile_index * tile_size, 0,
                                    tile_size, 0, nullptr, nullptr, nullptr));
    CL_CHECK(clCommandCopyBufferKHR(command_buffer, nullptr, buffer_src2,
                                    buffer_tile2, tile_index * tile_size, 0,
                                    tile_size, 0, nullptr, nullptr, nullptr));

    CL_CHECK(clCommandNDRangeKernelKHR(command_buffer, nullptr, nullptr, kernel,
                                       1, nullptr, &tile_elements, nullptr, 0,
                                       nullptr, nullptr, nullptr));

    CL_CHECK(clCommandCopyBufferKHR(command_buffer, nullptr, buffer_res,
                                    buffer_dst, 0, tile_index * tile_size,
                                    tile_size, 0, nullptr, nullptr, nullptr));
  }

  CL_CHECK(clFinalizeCommandBufferKHR(command_buffer));

  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  std::uniform_int_distribution<cl_int> random_distribution{
      0, std::numeric_limits<cl_int>::max() / 2};
  auto random_generator = [&]() { return random_distribution(random_engine); };

  for (size_t frame_index = 0; frame_index < frame_count; frame_index++) {
    std::vector<cl_int> src1(frame_elements);
    std::generate(src1.begin(), src1.end(), random_generator);
    CL_CHECK(clEnqueueWriteBuffer(command_queue, buffer_src1, CL_FALSE, 0,
                                  frame_size, src1.data(), 0, nullptr,
                                  nullptr));
    std::vector<cl_int> src2(frame_elements);
    std::generate(src2.begin(), src2.end(), random_generator);
    CL_CHECK(clEnqueueWriteBuffer(command_queue, buffer_src2, CL_FALSE, 0,
                                  frame_size, src2.data(), 0, nullptr,
                                  nullptr));

    CL_CHECK(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0, nullptr,
                                       nullptr));

    CL_CHECK(clFinish(command_queue));
  }

  CL_CHECK(clReleaseCommandBufferKHR(command_buffer));
  CL_CHECK(clReleaseMemObject(buffer_dst));
  CL_CHECK(clReleaseMemObject(buffer_src2));
  CL_CHECK(clReleaseMemObject(buffer_src1));
  CL_CHECK(clReleaseMemObject(buffer_res));
  CL_CHECK(clReleaseMemObject(buffer_tile2));
  CL_CHECK(clReleaseMemObject(buffer_tile1));
  CL_CHECK(clReleaseCommandQueue(command_queue));
  CL_CHECK(clReleaseKernel(kernel));
  CL_CHECK(clReleaseProgram(program));
  CL_CHECK(clReleaseContext(context));
  CL_CHECK(clReleaseDevice(device));
  return 0;
}
