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

#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include "Common.h"

using clCreateProgramWithBuiltInKernelsTest = ucl::ContextTest;

TEST_F(clCreateProgramWithBuiltInKernelsTest, NullErrorCode) {
  ASSERT_FALSE(
      clCreateProgramWithBuiltInKernels(nullptr, 1, &device, "", nullptr));
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, InvalidContext) {
  cl_int status;
  ASSERT_FALSE(
      clCreateProgramWithBuiltInKernels(nullptr, 1, &device, "", &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, BadNumDevices) {
  cl_int status = !CL_SUCCESS;
  ASSERT_FALSE(
      clCreateProgramWithBuiltInKernels(context, 0, &device, "", &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, BadDevices) {
  cl_int status = !CL_SUCCESS;
  ASSERT_FALSE(
      clCreateProgramWithBuiltInKernels(context, 1, nullptr, "", &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, BadBuiltinKernelNames) {
  cl_int status = !CL_SUCCESS;
  ASSERT_FALSE(
      clCreateProgramWithBuiltInKernels(context, 1, &device, nullptr, &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, BadDeviceInDevices) {
  enum { NUM_BAD_DEVICES = 2 };
  cl_device_id fakeDevices[NUM_BAD_DEVICES];
  std::memset(static_cast<void *>(fakeDevices), 0,
              sizeof(cl_device_id) * NUM_BAD_DEVICES);

  cl_int status = !CL_SUCCESS;
  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(context, NUM_BAD_DEVICES,
                                                 fakeDevices, "", &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, EmptyKernelNameSingle) {
  cl_int status;
  const std::string empty_kernel_name = "";

  // The OpenCL specification doesn't explicitly say whether passing the empty
  // string for kernel names should return an empty cl_program, or set
  // `CL_INVALID_VALUE` so we have chosen the latter.

  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(
      context, 1, &device, empty_kernel_name.c_str(), &status));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, EmptyKernelNameDouble) {
  cl_int status;
  const std::string empty_kernel_name = ";";

  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(
      context, 1, &device, empty_kernel_name.c_str(), &status));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, EmptyKernelNameTriple) {
  cl_int status;
  const std::string empty_kernel_name = ";;";

  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(
      context, 1, &device, empty_kernel_name.c_str(), &status));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(clCreateProgramWithBuiltInKernelsTest, NonExistentKernelName) {
  cl_int status;
  const std::string non_kernel_name = "thiskernelnamedoesntexist";

  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(
      context, 1, &device, non_kernel_name.c_str(), &status));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

#if defined(CL_VERSION_3_0)
TEST_F(clCreateProgramWithBuiltInKernelsTest, IL) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for a builtin kernel.
  size_t built_in_kernels_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr,
                                 &built_in_kernels_size));
  // In the case there are no built in kernels we cannot do this test.
  // Note: If there are no built-in kernels then the string returned will be
  // empty i.e. "\0" it will therefore have a size of 1 byte, not 0.
  if (built_in_kernels_size == 1) {
    GTEST_SKIP();
  }
  UCL::Buffer<char> built_in_kernels{built_in_kernels_size};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS,
                                 built_in_kernels_size, built_in_kernels.data(),
                                 nullptr));
  auto built_in_kernel =
      cargo::split(cargo::string_view(built_in_kernels), ";")[0];

  cl_int errorcode;
  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device,
      std::string(built_in_kernel.data(), built_in_kernel.size()).c_str(),
      &errorcode);
  ASSERT_SUCCESS(errorcode);
  ASSERT_NE(program, nullptr);

  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, 0, nullptr, &size));

  //  If program is created with clCreateProgramWithSource,
  //  clCreateProgramWithBinary or clCreateProgramWithBuiltInKernels the memory
  //  pointed to by param_value will be unchanged and param_value_size_ret will
  //  be set to 0.
  EXPECT_EQ(size, 0);
  UCL::Buffer<char> param_val{1};
  param_val[0] = 42;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, param_val.size(),
                                  param_val.data(), nullptr));
  ASSERT_EQ(param_val[0], 42);

  EXPECT_SUCCESS(clReleaseProgram(program));
}
#endif
