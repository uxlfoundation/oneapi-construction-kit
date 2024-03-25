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

#include <CL/cl_ext_codeplay.h>

#include "Common.h"

struct cl_codeplay_performance_counters_Test : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isPlatformExtensionSupported("cl_khr_create_command_queue") ||
        !isDeviceExtensionSupported("cl_codeplay_performance_counters")) {
      GTEST_SKIP();
    }
    cl_int error;
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
    ASSERT_SUCCESS(error);
    clCreateCommandQueueWithPropertiesKHR =
        reinterpret_cast<clCreateCommandQueueWithPropertiesKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform, "clCreateCommandQueueWithPropertiesKHR"));
    ASSERT_NE(nullptr, clCreateCommandQueueWithPropertiesKHR);
  }

  void TearDown() override {
    if (command_queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));
    }
    ContextTest::TearDown();
  }

  clCreateCommandQueueWithPropertiesKHR_fn
      clCreateCommandQueueWithPropertiesKHR = nullptr;
  cl_command_queue command_queue = nullptr;
};

TEST_F(cl_codeplay_performance_counters_Test, Default) {
  // Get the list of available performance counters.
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY, 0, nullptr, &size));
  if (size == 0) {  // There are no available performance counters.
    return;
  }
  std::vector<cl_performance_counter_codeplay> counters{
      size / sizeof(cl_performance_counter_codeplay)};
  ASSERT_SUCCESS(clGetDeviceInfo(device,
                                 CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY, size,
                                 counters.data(), nullptr));
  // Enable the first performance counter.
  cl_performance_counter_desc_codeplay counter_desc = {counters[0].uuid,
                                                       nullptr};
  cl_performance_counter_config_codeplay counter_config = {1, &counter_desc};
  std::array<cl_queue_properties_khr, 3> properties{{
      CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY,
      reinterpret_cast<cl_queue_properties_khr>(&counter_config),
      0,
  }};
  cl_int error;
  command_queue = clCreateCommandQueueWithPropertiesKHR(
      context, device, properties.data(), &error);
  ASSERT_SUCCESS(error);
  // Prepare and enqueue a kernel workload.
  const char *source = "void kernel foo() {}";
  const size_t length = strlen(source);
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, &length, &error);
  ASSERT_SUCCESS(error);
  EXPECT_SUCCESS(clBuildProgram(program, 1, &device, "", nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &error);
  EXPECT_SUCCESS(error);
  cl_event event;
  const size_t global_work_size = 1;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_work_size, nullptr, 0, nullptr,
                                        &event));
  EXPECT_SUCCESS(clFinish(command_queue));
  // Read the performance counter data.
  EXPECT_SUCCESS(clGetEventProfilingInfo(
      event, CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY, 0, nullptr,
      &size));
  EXPECT_EQ(sizeof(cl_performance_counter_result_codeplay), size);
  cl_performance_counter_result_codeplay result;
  EXPECT_SUCCESS(clGetEventProfilingInfo(
      event, CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY, size, &result,
      nullptr));
  // Display the result, we don't know what the valid range of values is so
  // there is no way to expect a specific value.
  switch (counters[0].storage) {
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT32_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.int32 << "\n";
      break;
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT64_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.int64 << "\n";
      break;
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT32_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.uint32 << "\n";
      break;
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT64_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.uint64 << "\n";
      break;
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT32_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.float32 << "\n";
      break;
    case CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT64_CODEPLAY:
      std::cout << counters[0].name << " has value: " << result.float64 << "\n";
      break;
    default:
      UCL_ABORT("invalid storage type %d", (int)counters[0].storage);
  }
  // Cleanup
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}
