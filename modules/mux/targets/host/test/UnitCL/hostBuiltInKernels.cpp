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
#include <Common.h>
#include <cargo/dynamic_array.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include "Device.h"

using hostCreateProgramWithBuiltInKernelsTest = ucl::ContextTest;

TEST_F(hostCreateProgramWithBuiltInKernelsTest, ValidNameWithEmptyName) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  cl_int status;
  const std::string empty_kernel_name = "copy_buffer;";

  ASSERT_FALSE(clCreateProgramWithBuiltInKernels(
      context, 1, &device, empty_kernel_name.c_str(), &status));

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
}

TEST_F(hostCreateProgramWithBuiltInKernelsTest, BuildBuiltInProgram) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const cargo::string_view kernel_name = "print_message";
  cl_int status;

  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  ASSERT_NE(0u, size);
  UCL::Buffer<char> kernel_names(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 kernel_names, nullptr));
  auto kernel_names_len = std::strlen(kernel_names);
  ASSERT_NE(0u, kernel_names_len);
  ASSERT_EQ(kernel_names_len + 1, size);

  auto split_device_names =
      cargo::split_all(cargo::string_view(kernel_names.data()), ";");

  ASSERT_FALSE(std::none_of(
      split_device_names.begin(), split_device_names.end(),
      [&kernel_name](const cargo::string_view &reported_name) {
        return reported_name.find(kernel_name) != std::string::npos;
      }))
      << "'" << kernel_name << "'%s' kernel not present.";

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, kernel_name.data(), &status);

  ASSERT_TRUE(program);
  ASSERT_SUCCESS(status);

  // calling clBuildProgram with builtin kernels is unnecessary so it should
  // report `CL_INVALID_OPERATION`
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(hostCreateProgramWithBuiltInKernelsTest, CopyBuffer) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const cargo::string_view kernel_name = "copy_buffer";
  cl_int status;

  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  ASSERT_NE(0u, size);
  UCL::Buffer<char> kernel_names(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 kernel_names, nullptr));
  auto kernel_names_len = std::strlen(kernel_names);
  ASSERT_NE(0u, kernel_names_len);
  ASSERT_EQ(kernel_names_len + 1, size);

  auto split_device_names =
      cargo::split_all(cargo::string_view(kernel_names.data()), ";");

  ASSERT_FALSE(std::none_of(
      split_device_names.begin(), split_device_names.end(),
      [&kernel_name](const cargo::string_view &reported_name) {
        return reported_name.find(kernel_name) != std::string::npos;
      }))
      << "'" << kernel_name << "' kernel not present.";

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, kernel_name.data(), &status);

  ASSERT_TRUE(program);
  ASSERT_SUCCESS(status);

  cl_kernel kernel = clCreateKernel(program, kernel_name.data(), &status);

  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(status);
  // START
  const cl_int NUM = 24;
  cl_mem inMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  cl_mem outMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  EXPECT_TRUE(outMem);
  EXPECT_SUCCESS(status);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &inMem));

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &outMem));

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
  EXPECT_TRUE(queue);
  EXPECT_SUCCESS(status);

  UCL::vector<cl_int> inp(NUM, 2);
  // Write data to the in buffer
  EXPECT_SUCCESS(clEnqueueWriteBuffer(queue, inMem, CL_TRUE, 0,
                                      NUM * sizeof(cl_int), inp.data(), 0,
                                      nullptr, nullptr));

  const size_t global_size = NUM;
  cl_event event;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size,
                                        nullptr, 0, nullptr, &event));

  UCL::vector<cl_int> res(NUM, 1);

  EXPECT_SUCCESS(clEnqueueReadBuffer(queue, outMem, CL_TRUE, 0,
                                     NUM * sizeof(cl_int), res.data(), 1,
                                     &event, nullptr));

  for (int i = 0, e = NUM; i < e; ++i) {
    EXPECT_TRUE(res[i] == (inp[i]));
  }

  EXPECT_SUCCESS(clReleaseMemObject(inMem));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
  ASSERT_SUCCESS(status);
}

TEST_F(hostCreateProgramWithBuiltInKernelsTest, Printf) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const cargo::string_view kernel_name = "print_message";
  cl_int status;

  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  ASSERT_NE(0u, size);
  UCL::Buffer<char> kernel_names(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 kernel_names, nullptr));
  auto kernel_names_len = std::strlen(kernel_names);
  ASSERT_NE(0u, kernel_names_len);
  ASSERT_EQ(kernel_names_len + 1, size);

  auto split_device_names =
      cargo::split_all(cargo::string_view(kernel_names.data()), ";");

  ASSERT_FALSE(std::none_of(
      split_device_names.begin(), split_device_names.end(),
      [&kernel_name](const cargo::string_view &reported_name) {
        return reported_name.find(kernel_name) != std::string::npos;
      }))
      << "'" << kernel_name << "' kernel not present.\n";

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, kernel_name.data(), &status);

  ASSERT_TRUE(program);
  ASSERT_SUCCESS(status);

  cl_kernel kernel = clCreateKernel(program, kernel_name.data(), &status);

  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(status);

  cl_command_queue queue;
  queue = clCreateCommandQueue(context, device, 0, &status);
  EXPECT_TRUE(queue);
  EXPECT_SUCCESS(status);

  cl_event event;
  EXPECT_SUCCESS(clEnqueueTask(queue, kernel, 0, nullptr, &event));

  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
  ASSERT_SUCCESS(status);
}

TEST_F(hostCreateProgramWithBuiltInKernelsTest, TwoKernelsFirstKernel) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const cargo::string_view kernel_name = "copy_buffer;print_message";
  cl_int status;

  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  ASSERT_NE(0u, size);
  UCL::Buffer<char> kernel_names(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 kernel_names, nullptr));
  auto kernel_names_len = std::strlen(kernel_names);
  ASSERT_NE(0u, kernel_names_len);
  ASSERT_EQ(kernel_names_len + 1, size);

  auto split_device_names =
      cargo::split_all(cargo::string_view(kernel_names.data()), ";");
  auto split_kernel_name =
      cargo::split_all(cargo::string_view(kernel_name.data()), ";");
  for (const auto &test_kernel_name : split_kernel_name) {
    ASSERT_FALSE(std::none_of(
        split_device_names.begin(), split_device_names.end(),
        [&test_kernel_name](const cargo::string_view &reported_name) {
          return reported_name.find(test_kernel_name) != std::string::npos;
        }))
        << "'" << kernel_name << "' kernel not present.";
  }

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, kernel_name.data(), &status);

  ASSERT_TRUE(program);
  ASSERT_SUCCESS(status);

  const cargo::string_view selected_kernel = "copy_buffer";
  cl_kernel kernel = clCreateKernel(program, selected_kernel.data(), &status);

  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(status);
  // START
  const cl_int NUM = 24;
  cl_mem inMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  cl_mem outMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  EXPECT_TRUE(outMem);
  EXPECT_SUCCESS(status);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &inMem));

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &outMem));

  cl_command_queue queue;
  queue = clCreateCommandQueue(context, device, 0, &status);
  EXPECT_TRUE(queue);
  EXPECT_SUCCESS(status);

  UCL::vector<cl_int> inp(NUM, 2);
  // Write data to the in buffer
  EXPECT_SUCCESS(clEnqueueWriteBuffer(queue, inMem, CL_TRUE, 0,
                                      NUM * sizeof(cl_int), inp.data(), 0,
                                      nullptr, nullptr));

  const size_t global_size = NUM;
  cl_event event;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size,
                                        nullptr, 0, nullptr, &event));

  UCL::vector<cl_int> res(NUM, 1);

  EXPECT_SUCCESS(clEnqueueReadBuffer(queue, outMem, CL_TRUE, 0,
                                     NUM * sizeof(cl_int), res.data(), 1,
                                     &event, nullptr));

  for (int i = 0, e = NUM; i < e; ++i) {
    EXPECT_TRUE(res[i] == (inp[i]));
  }

  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
  EXPECT_SUCCESS(clReleaseMemObject(inMem));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(status);
}

TEST_F(hostCreateProgramWithBuiltInKernelsTest, TwoKernelsSecondKernel) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const cargo::string_view kernel_name = "print_message;copy_buffer";
  cl_int status;

  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  ASSERT_NE(0u, size);
  UCL::Buffer<char> kernel_names(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 kernel_names, nullptr));
  auto kernel_names_len = std::strlen(kernel_names);
  ASSERT_NE(0u, kernel_names_len);
  ASSERT_EQ(kernel_names_len + 1, size);

  auto split_device_names =
      cargo::split_all(cargo::string_view(kernel_names.data()), ";");
  auto split_kernel_name =
      cargo::split_all(cargo::string_view(kernel_name.data()), ";");
  for (const auto &test_kernel_name : split_kernel_name) {
    ASSERT_FALSE(std::none_of(
        split_device_names.begin(), split_device_names.end(),
        [&test_kernel_name](const cargo::string_view &reported_name) {
          return reported_name.find(test_kernel_name) != std::string::npos;
        }))
        << "'" << kernel_name << "' kernel not present.";
  }

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, kernel_name.data(), &status);

  ASSERT_TRUE(program);
  ASSERT_SUCCESS(status);

  const cargo::string_view selected_kernel = "copy_buffer";
  cl_kernel kernel = clCreateKernel(program, selected_kernel.data(), &status);

  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(status);
  // START
  const cl_int NUM = 24;
  cl_mem inMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  cl_mem outMem =
      clCreateBuffer(context, 0, NUM * sizeof(cl_int), nullptr, &status);
  EXPECT_TRUE(outMem);
  EXPECT_SUCCESS(status);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &inMem));

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &outMem));

  cl_command_queue queue;
  queue = clCreateCommandQueue(context, device, 0, &status);
  EXPECT_TRUE(queue);
  EXPECT_SUCCESS(status);

  UCL::vector<cl_int> inp(NUM, 2);
  // Write data to the in buffer
  EXPECT_SUCCESS(clEnqueueWriteBuffer(queue, inMem, CL_TRUE, 0,
                                      NUM * sizeof(cl_int), inp.data(), 0,
                                      nullptr, nullptr));

  const size_t global_size = NUM;
  cl_event event;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size,
                                        nullptr, 0, nullptr, &event));

  UCL::vector<cl_int> res(NUM, 1);

  EXPECT_SUCCESS(clEnqueueReadBuffer(queue, outMem, CL_TRUE, 0,
                                     NUM * sizeof(cl_int), res.data(), 1,
                                     &event, nullptr));

  for (int i = 0, e = NUM; i < e; ++i) {
    EXPECT_TRUE(res[i] == (inp[i]));
  }

  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
  EXPECT_SUCCESS(clReleaseMemObject(inMem));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(status);
}

struct hostBuiltInKernelsArgsTest : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    size_t size;
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
    if (size) {
      builtin_kernels.resize(size);
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                     builtin_kernels.data(), nullptr));
    }
  }

  void TearDown() override {
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  bool hasBuiltinKernel(const std::string &name) {
    if (builtin_kernels.empty()) {
      return false;
    }
    if (builtin_kernels.find(name) == std::string::npos) {
      return false;
    }
    return true;
  }

  struct kernel_arg_info {
    cl_kernel_arg_address_qualifier address_qualifier;
    cl_kernel_arg_access_qualifier access_qualifier;
    cl_kernel_arg_type_qualifier type_qualifier;
    std::string type_name;
    std::string name;
  };

  kernel_arg_info getKernelArgInfo(cl_uint arg_indx, cl_int *error) {
    kernel_arg_info info;
    *error =
        clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
                           sizeof(cl_kernel_arg_address_qualifier),
                           &info.address_qualifier, nullptr);
    if (*error) {
      return {};
    }
    *error =
        clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_ACCESS_QUALIFIER,
                           sizeof(cl_kernel_arg_access_qualifier),
                           &info.access_qualifier, nullptr);
    if (*error) {
      return {};
    }
    *error = clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_TYPE_QUALIFIER,
                                sizeof(cl_kernel_arg_type_qualifier),
                                &info.type_qualifier, nullptr);
    if (*error) {
      return {};
    }
    size_t size;
    *error = clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_TYPE_NAME, 0,
                                nullptr, &size);
    if (*error) {
      return {};
    }
    std::vector<char> temp(size);
    *error = clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_TYPE_NAME, size,
                                temp.data(), &size);
    if (*error) {
      return {};
    }
    info.type_name = temp.data();
    *error = clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_NAME, 0,
                                nullptr, &size);
    if (*error) {
      return {};
    }
    temp.resize(size);
    *error = clGetKernelArgInfo(kernel, arg_indx, CL_KERNEL_ARG_NAME, size,
                                temp.data(), &size);
    if (*error) {
      return {};
    }
    info.name = temp.data();
    return info;
  }

  cl_int createProgramAndKernel(std::string kernel_name) {
    cl_int error;
    program = clCreateProgramWithBuiltInKernels(context, 1, &device,
                                                kernel_name.c_str(), &error);
    if (error) {
      return error;
    }
    kernel = clCreateKernel(program, kernel_name.c_str(), &error);
    if (error) {
      return error;
    }
    return CL_SUCCESS;
  }

  void test_numeric_args(std::string type, size_t type_size,
                         bool test_value_types = true) {
    // Since this is a host-specific test we want to skip it if we aren't
    // running on host.
    // FIXME: This would be better in a shared SetUp function. See CA-4720.
    if (!UCL::isDevice_host(device)) {
      GTEST_SKIP() << "Not running on host device, skipping test.\n";
    }
    if (!test_value_types) {
      (void)type_size;
    }
    const std::string kernel_name = "args_" + type;
    if (std::string::npos != type.find("half") &&
        !UCL::hasDeviceExtensionSupport(device, "cl_khr_fp16")) {
      GTEST_SKIP();
    } else if (std::string::npos != type.find("double") &&
               !UCL::hasDoubleSupport(device)) {
      GTEST_SKIP();
    }
    ASSERT_TRUE(hasBuiltinKernel(kernel_name))
        << "'" << kernel_name << "' kernel is not present.";
    ASSERT_SUCCESS(createProgramAndKernel(kernel_name));

    cl_int error;
    cl_uint arg_index = 0;
    cargo::dynamic_array<unsigned char> constant_buffer;
    ASSERT_EQ(cargo::success, constant_buffer.alloc(type_size));
    if (test_value_types) {
      {  // <type> t
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_PRIVATE, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ("t", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(clSetKernelArg(kernel, arg_index, type_size,
                                      constant_buffer.data()));
        arg_index++;
      }

      {  // const <type> ct
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_PRIVATE, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        // This is a value so CL_KERNEL_ARG_TYPE_CONST is not set.
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ("ct", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(clSetKernelArg(kernel, arg_index, type_size,
                                      constant_buffer.data()));
        arg_index++;
      }

      {  // volatile <type> vt
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_PRIVATE, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        // This is a value so CL_KERNEL_ARG_TYPE_VOLATILE is not set.
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ("vt", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(clSetKernelArg(kernel, arg_index, type_size,
                                      constant_buffer.data()));
        arg_index++;
      }

      {  // const volatile <type> cvt
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_PRIVATE, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        // This is a value so CL_KERNEL_ARG_TYPE_CONST |
        // CL_KERNEL_ARG_TYPE_VOLATILE is not set.
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ("cvt", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(clSetKernelArg(kernel, arg_index, type_size,
                                      constant_buffer.data()));
        arg_index++;
      }
    }

    // Testing pointer types from now on.
    type = type + "*";

    for (auto address_qualifier :
         {CL_KERNEL_ARG_ADDRESS_LOCAL, CL_KERNEL_ARG_ADDRESS_GLOBAL,
          CL_KERNEL_ARG_ADDRESS_CONSTANT}) {
      std::string addrspace;
      switch (address_qualifier) {
        case CL_KERNEL_ARG_ADDRESS_LOCAL:
          addrspace = "lo";
          break;
        case CL_KERNEL_ARG_ADDRESS_GLOBAL:
          addrspace = "gl";
          break;
        case CL_KERNEL_ARG_ADDRESS_CONSTANT:
          addrspace = "co";
          break;
        default:
          FAIL() << "Unexpected address qualifier.";
      }

      {  // <address_qualifier> <type>* <addrspace>p
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        if (CL_KERNEL_ARG_ADDRESS_CONSTANT == address_qualifier) {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST, arg.type_qualifier)
              << "Argument index: " << arg_index;
        } else {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
              << "Argument index: " << arg_index;
        }
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "p", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> const <type>* <addrspace>cp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "cp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> volatile <type>* <addrspace>vp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        if (CL_KERNEL_ARG_ADDRESS_CONSTANT == address_qualifier) {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_VOLATILE,
                    arg.type_qualifier)
              << "Argument index: " << arg_index;
        } else {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_VOLATILE, arg.type_qualifier)
              << "Argument index: " << arg_index;
        }
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "vp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> <type>* restrict <addrspace>rp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        if (CL_KERNEL_ARG_ADDRESS_CONSTANT == address_qualifier) {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_RESTRICT,
                    arg.type_qualifier)
              << "Argument index: " << arg_index;
        } else {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_RESTRICT, arg.type_qualifier)
              << "Argument index: " << arg_index;
        }
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "rp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> const volatile <type>* <addrspace>vp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_VOLATILE,
                  arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "cvp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> const volatile <type>* <addrspace>vp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_RESTRICT,
                  arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "crp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> volatile <type>* restrict <addrspace>vp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        if (CL_KERNEL_ARG_ADDRESS_CONSTANT == address_qualifier) {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_VOLATILE |
                        CL_KERNEL_ARG_TYPE_RESTRICT,
                    arg.type_qualifier)
              << "Argument index: " << arg_index;
        } else {
          EXPECT_EQ(CL_KERNEL_ARG_TYPE_VOLATILE | CL_KERNEL_ARG_TYPE_RESTRICT,
                    arg.type_qualifier)
              << "Argument index: " << arg_index;
        }
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "vrp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }

      {  // <address_qualifier> const volatile <type>* restrict <addrspace>cvrp
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error);
        EXPECT_EQ(address_qualifier, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_VOLATILE |
                      CL_KERNEL_ARG_TYPE_RESTRICT,
                  arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ(addrspace + "cvrp", arg.name)
            << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
        arg_index++;
      }
    }
  }

  void test_image_args(std::string image_type) {
    if (!UCL::hasImageSupport(device)) {
      GTEST_SKIP();
    }
    // Since this is a host-specific test we want to skip it if we aren't
    // running on host.
    // FIXME: This would be better in a shared SetUp function. See CA-4720.
    if (!UCL::isDevice_host(device)) {
      GTEST_SKIP() << "Not running on host device, skipping test.\n";
    }
    const std::string kernel_name = "args_" + image_type;
    ASSERT_TRUE(hasBuiltinKernel(kernel_name))
        << "'" << kernel_name << "' kernel is not present";
    ASSERT_SUCCESS(createProgramAndKernel(kernel_name));

    cl_int error;
    cl_uint arg_index = 0;

    {  // <image_type> i
      auto arg = getKernelArgInfo(arg_index, &error);
      EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_GLOBAL, arg.address_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_ACCESS_READ_ONLY, arg.access_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(image_type, arg.type_name) << "Argument index: " << arg_index;
      EXPECT_EQ("i", arg.name) << "Argument index: " << arg_index;
      EXPECT_SUCCESS(
          clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
    }

    {  // read_only <image_type> roi
      arg_index++;
      auto arg = getKernelArgInfo(arg_index, &error);
      EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_GLOBAL, arg.address_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_ACCESS_READ_ONLY, arg.access_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
          << "Argument index: " << arg_index;
      EXPECT_EQ(image_type, arg.type_name) << "Argument index: " << arg_index;
      EXPECT_EQ("roi", arg.name) << "Argument index: " << arg_index;
      EXPECT_SUCCESS(
          clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
    }

    {  // write_only <image_type> woi
      const bool image3d_writes =
          UCL::hasDeviceExtensionSupport(device, "cl_khr_3d_image_writes");
      size_t size;
      ASSERT_EQ(CL_SUCCESS, clGetPlatformInfo(platform, CL_PLATFORM_PROFILE, 0,
                                              nullptr, &size));
      std::string platform_profile(size, '\0');
      ASSERT_EQ(CL_SUCCESS,
                clGetPlatformInfo(platform, CL_PLATFORM_PROFILE, size,
                                  platform_profile.data(), nullptr));
      bool image2d_array_writes = true;
      if (platform_profile == "EMBEDDED") {
        image2d_array_writes = UCL::hasDeviceExtensionSupport(
            device, "cles_khr_2d_image_array_writes");
      }

      if ((image_type != "image3d_t" || image_type != "image2d_array_t") ||
          (image_type == "image3d_t" && image3d_writes) ||
          (image_type == "image2d_array_t" && image2d_array_writes)) {
        arg_index++;
        auto arg = getKernelArgInfo(arg_index, &error);
        EXPECT_SUCCESS(error) << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_GLOBAL, arg.address_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_ACCESS_WRITE_ONLY, arg.access_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier)
            << "Argument index: " << arg_index;
        EXPECT_EQ(image_type, arg.type_name) << "Argument index: " << arg_index;
        EXPECT_EQ("woi", arg.name) << "Argument index: " << arg_index;
        EXPECT_SUCCESS(
            clSetKernelArg(kernel, arg_index, sizeof(cl_mem), nullptr));
      }
    }
  }

  std::string builtin_kernels;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

#define TEST_BUILTIN_KERNEL_NUMERIC_ARGS(TYPE, SIZE)    \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE) {     \
    test_numeric_args(#TYPE, SIZE);                     \
  }                                                     \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE##2) {  \
    test_numeric_args(#TYPE STR(2), SIZE * 2);          \
  }                                                     \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE##3) {  \
    test_numeric_args(#TYPE STR(3), SIZE * 4);          \
  }                                                     \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE##4) {  \
    test_numeric_args(#TYPE STR(4), SIZE * 4);          \
  }                                                     \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE##8) {  \
    test_numeric_args(#TYPE STR(8), SIZE * 8);          \
  }                                                     \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE##16) { \
    test_numeric_args(#TYPE STR(16), SIZE * 16);        \
  }

TEST_BUILTIN_KERNEL_NUMERIC_ARGS(char, sizeof(cl_char))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(uchar, sizeof(cl_uchar))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(short, sizeof(cl_short))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(ushort, sizeof(cl_ushort))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(int, sizeof(cl_int))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(uint, sizeof(cl_uint))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(long, sizeof(cl_long))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(ulong, sizeof(cl_ulong))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(float, sizeof(cl_float))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(double, sizeof(cl_double))
TEST_BUILTIN_KERNEL_NUMERIC_ARGS(half, sizeof(cl_half))

TEST_F(hostBuiltInKernelsArgsTest, args_void) {
  // The `type_size` parameter only applies to types that can be used as
  // constant parameters. `void` can only be used as a pointer (`void*`), so
  // it's size doesn't matter. It's set to 0.
  test_numeric_args("void", 0, false);
}

TEST_F(hostBuiltInKernelsArgsTest, args_address_qualifiers) {
  // Since these are host specific test we want to skip it if we aren't
  // running on host.
  // FIXME: This would be better in a shared SetUp function. See CA-4720.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const std::string kernel_name = "args_address_qualifiers";
  ASSERT_TRUE(hasBuiltinKernel(kernel_name))
      << "'" << kernel_name << "' kernel is not present.";
  ASSERT_SUCCESS(createProgramAndKernel(kernel_name));

  cl_int error;
  cl_mem buffer = clCreateBuffer(context, 0, 16, nullptr, &error);
  ASSERT_SUCCESS(error);

  {  // __local int loi
    auto arg = getKernelArgInfo(0, &error);
    ASSERT_SUCCESS(error);
    EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_LOCAL, arg.address_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier);
    EXPECT_STREQ("int*", arg.type_name.c_str());
    EXPECT_STREQ("loi", arg.name.c_str());
    EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(nullptr), nullptr));
  }

  {  // __global int gli
    auto arg = getKernelArgInfo(1, &error);
    ASSERT_SUCCESS(error);
    EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_GLOBAL, arg.address_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier);
    EXPECT_STREQ("int*", arg.type_name.c_str());
    EXPECT_STREQ("gli", arg.name.c_str());
    EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer));
  }

  {  // __constant int coi
    auto arg = getKernelArgInfo(2, &error);
    ASSERT_SUCCESS(error);
    EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_CONSTANT, arg.address_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier);
    EXPECT_EQ(CL_KERNEL_ARG_TYPE_CONST, arg.type_qualifier);
    EXPECT_STREQ("int*", arg.type_name.c_str());
    EXPECT_STREQ("coi", arg.name.c_str());
    EXPECT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&buffer));
  }

  EXPECT_SUCCESS(clReleaseMemObject(buffer));
}

#define TEST_BUILTIN_KERNEL_IMAGE_ARGS(TYPE) \
  TEST_F(hostBuiltInKernelsArgsTest, args_##TYPE) { test_image_args(#TYPE); }

TEST_BUILTIN_KERNEL_IMAGE_ARGS(image1d_t)
TEST_BUILTIN_KERNEL_IMAGE_ARGS(image1d_array_t)
TEST_BUILTIN_KERNEL_IMAGE_ARGS(image1d_buffer_t)
TEST_BUILTIN_KERNEL_IMAGE_ARGS(image2d_t)
TEST_BUILTIN_KERNEL_IMAGE_ARGS(image2d_array_t)
TEST_BUILTIN_KERNEL_IMAGE_ARGS(image3d_t)

TEST_F(hostBuiltInKernelsArgsTest, args_sampler_t) {
  if (!UCL::hasImageSupport(device)) {
    GTEST_SKIP();
  }
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  const std::string kernel_name = "args_sampler_t";

  ASSERT_TRUE(hasBuiltinKernel(kernel_name))
      << "'" << kernel_name << "' kernel is not present.";
  ASSERT_SUCCESS(createProgramAndKernel(kernel_name));

  cl_int error;

  cl_sampler sampler = clCreateSampler(context, false, CL_ADDRESS_NONE,
                                       CL_FILTER_NEAREST, &error);
  ASSERT_SUCCESS(error);

  auto arg = getKernelArgInfo(0, &error);
  ASSERT_SUCCESS(error);
  EXPECT_EQ(CL_KERNEL_ARG_ADDRESS_PRIVATE, arg.address_qualifier);
  EXPECT_EQ(CL_KERNEL_ARG_ACCESS_NONE, arg.access_qualifier);
  EXPECT_EQ(CL_KERNEL_ARG_TYPE_NONE, arg.type_qualifier);
  EXPECT_STREQ("sampler_t", arg.type_name.c_str());
  EXPECT_STREQ("s", arg.name.c_str());
  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_sampler), (void *)&sampler));
  ASSERT_SUCCESS(clReleaseSampler(sampler));
}
