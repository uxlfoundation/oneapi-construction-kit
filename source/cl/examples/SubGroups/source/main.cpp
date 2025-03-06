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

#include <CL/cl.h>
#include <common.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#define CL_CHECK(ERROR)                            \
  if (ERROR) {                                     \
    std::cerr << "OpenCL error: " << ERROR << "\n" \
              << "at line: " << __LINE__ << "\n";  \
    std::exit(1);                                  \
  }

int main(const int argc, const char **argv) {
  const char *platform_name = NULL;
  const char *device_name = NULL;
  parseArguments(argc, argv, &platform_name, &device_name);

  cl_platform_id platform = selectPlatform(platform_name);
  cl_device_id device = selectDevice(platform, device_name);

  // Sub-groups were introduced in 2.X.
  size_t device_version_length = 0;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr,
                           &device_version_length));
  std::string device_version(device_version_length, '\0');
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_VERSION, device_version_length,
                           device_version.data(), nullptr));

  // The device version string must be of the form
  // OpenCL<space><major_version.minor_version><space><vendor-specific
  // information>.
  auto dot_position = device_version.find('.');
  auto major_version = device_version[dot_position - 1] - '0';

  // Skip the example if the OpenCL driver is early than 2.X since sub-groups
  // didn't exist.
  if (major_version < 2) {
    std::cerr << "Sub-groups are not an OpenCL feature before OpenCL 2.0, "
                 "skipping sub-group example.\n";
    return 0;
  }

  // Sub-groups were made optional in OpenCL 3.0, so check that they are
  // supported if we have a 3.0 driver or later.
  if (major_version >= 3) {
    cl_uint max_num_sub_groups = 0;
    CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                             sizeof(cl_uint), &max_num_sub_groups, nullptr));
    if (0 == max_num_sub_groups) {
      std::cerr << "Sub-groups are not supported on this device, "
                   "skipping sub-group example.\n";
      return 0;
    }
  }

  // A compiler is required to compile the example kernel, if there isn't one
  // skip.
  cl_bool device_compiler_available;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,
                           sizeof(cl_bool), &device_compiler_available,
                           nullptr));
  if (!device_compiler_available) {
    std::cerr << "compiler not available for the device, skipping sub-group"
                 "example.\n";
    return 0;
  }

  auto error = CL_SUCCESS;
  auto context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  CL_CHECK(error);

  const char *code = R"OPENCLC(
kernel void reduction(global int *in, local int *tmp, global int *out) {
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t wgid = get_group_id(0);
  const size_t sgid = get_sub_group_id();
  const size_t sg_count = get_num_sub_groups();

  int partial_reduction = sub_group_reduce_add(in[gid]);
  tmp[sgid] = partial_reduction;

  barrier(CLK_LOCAL_MEM_FENCE);

  for (unsigned i = sg_count / 2; i != 0; i /= 2) {
    if (lid < i) {
      tmp[lid] = tmp[lid] + tmp[lid + i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (lid == 0) {
    out[wgid] = *tmp;
  }
}
)OPENCLC";
  const auto code_length = std::strlen(code);

  auto program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  CL_CHECK(error);

  CL_CHECK(
      clBuildProgram(program, 1, &device,
                     major_version == 2 ? "-cl-std=CL2.0" : "-cl-std=CL3.0",
                     nullptr, nullptr));

  auto kernel = clCreateKernel(program, "reduction", &error);
  CL_CHECK(error);

  constexpr size_t global_size = 1024;
  constexpr size_t local_size = 32;
  constexpr size_t work_group_count = global_size / local_size;
  size_t sub_group_count = 0;
  CL_CHECK(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE, sizeof(size_t),
      &local_size, sizeof(size_t), &sub_group_count, nullptr));
  std::cout << "Sub-group count for local size (" << local_size
            << ", 1, 1): " << sub_group_count << '\n';
  constexpr size_t input_buffer_size = global_size * sizeof(cl_int);
  constexpr size_t output_buffer_size = work_group_count * sizeof(cl_int);
  const size_t local_buffer_size = sub_group_count * sizeof(cl_int);

  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  std::uniform_int_distribution<cl_int> random_distribution{
      std::numeric_limits<cl_int>::min() / static_cast<cl_int>(global_size),
      std::numeric_limits<cl_int>::max() / static_cast<cl_int>(global_size)};
  std::vector<cl_int> input_data(global_size);
  std::generate(std::begin(input_data), std::end(input_data),
                [&]() { return random_distribution(random_engine); });

  auto input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                     input_buffer_size, nullptr, &error);
  CL_CHECK(error);

  auto output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                      output_buffer_size, nullptr, &error);
  CL_CHECK(error);

  auto command_queue = clCreateCommandQueue(context, device, 0, &error);
  CL_CHECK(error);

  CL_CHECK(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                input_buffer_size, input_data.data(), 0,
                                nullptr, nullptr));

  CL_CHECK(clSetKernelArg(kernel, 0, sizeof(input_buffer),
                          static_cast<void *>(&input_buffer)));
  CL_CHECK(clSetKernelArg(kernel, 1, local_buffer_size, nullptr));
  CL_CHECK(clSetKernelArg(kernel, 2, sizeof(output_buffer),
                          static_cast<void *>(&output_buffer)));

  CL_CHECK(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                  &global_size, &local_size, 0, nullptr,
                                  nullptr));
  std::vector<cl_int> output_data(work_group_count, 0);
  CL_CHECK(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                               output_buffer_size, output_data.data(), 0,
                               nullptr, nullptr));

  auto result =
      std::accumulate(std::begin(output_data), std::end(output_data), 0);
  auto expected =
      std::accumulate(std::begin(input_data), std::end(input_data), 0);

  CL_CHECK(clReleaseCommandQueue(command_queue));
  CL_CHECK(clReleaseMemObject(input_buffer));
  CL_CHECK(clReleaseMemObject(output_buffer));
  CL_CHECK(clReleaseKernel(kernel));
  CL_CHECK(clReleaseProgram(program));
  CL_CHECK(clReleaseContext(context));

  if (result != expected) {
    std::cerr << "Result did not validate, expected: " << expected
              << " but got: " << result << " exiting...\n";
    std::exit(1);
  }

  std::cout
      << "Result validated, sub-groups example ran successfully, exiting...\n";
  return 0;
}
