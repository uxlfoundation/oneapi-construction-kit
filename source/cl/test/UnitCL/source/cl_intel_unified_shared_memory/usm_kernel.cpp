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

#include <Common.h>
#include <cargo/small_vector.h>

#include "cl_intel_unified_shared_memory.h"

struct USMKernelTest : public cl_intel_unified_shared_memory_Test {
  // Build kernel named "foo" from OpenCL-C source string argument
  void BuildKernel(const char *source) {
    const size_t length = std::strlen(source);
    cl_int err = !CL_SUCCESS;
    program = clCreateProgramWithSource(context, 1, &source, &length, &err);
    ASSERT_NE(program, nullptr);
    ASSERT_SUCCESS(err);

    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, "",
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "foo", &err);
    ASSERT_SUCCESS(err);
    ASSERT_NE(kernel, nullptr);
  }

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
        sizeof(shared_capabilities), &shared_capabilities, nullptr));

    initPointers(bytes, align);
    user_ptr = malloc(bytes);

    cl_device_unified_shared_memory_capabilities_intel single_capabilities,
        cross_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
        sizeof(single_capabilities), &single_capabilities, nullptr));
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL,
        sizeof(cross_capabilities), &cross_capabilities, nullptr));
  }

  void TearDown() override {
    if (user_ptr) {
      free(user_ptr);
    }

    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    cl_intel_unified_shared_memory_Test::TearDown();
  }

  const size_t elements = 64;
  const size_t bytes = elements * sizeof(cl_int);
  const cl_uint align = sizeof(cl_int);

  void *user_ptr = nullptr;

  cl_kernel kernel = nullptr;
  cl_program program = nullptr;
};

template <typename T>
struct USMKernelUsageTest : public USMKernelTest,
                            public ::testing::WithParamInterface<T> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(USMKernelTest::SetUp());

    BuildKernel(source);
  }

  static const char *source;
};

template <typename T>
const char *USMKernelUsageTest<T>::source =
    R"(
struct bar {
  int irn_bru;
};

void kernel foo(__global int* a,
                __constant int* b,
                __local int* c,
                struct bar d) {
   a[0] = b[0] * c[0] * d.irn_bru;
}
)";

// Test for invalid API usage of clSetKernelArgMemPointerINTEL()
using USMSetKernelArgMemPointerTest = USMKernelUsageTest<cl_uint>;
TEST_P(USMSetKernelArgMemPointerTest, InvalidUsage) {
  cl_int err = clSetKernelArgMemPointerINTEL(nullptr, 0, device_ptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_KERNEL);

  for (auto ptr : allPointers()) {
    err = clSetKernelArgMemPointerINTEL(kernel, CL_UINT_MAX, ptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_ARG_INDEX);

    err = clSetKernelArgMemPointerINTEL(kernel, 4, ptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_ARG_INDEX);
  }

  // The cl_intel_unified_shared_memory specification has an open question
  // whether invalid pointers should result in an error. We accept this as Intel
  // passes invalid pointers in valid SYCL code.
  cl_uint usm_arg_index = GetParam();
  err = clSetKernelArgMemPointerINTEL(kernel, usm_arg_index, user_ptr);
  ASSERT_SUCCESS(err);

  for (auto ptr : allPointers()) {
    err = clSetKernelArgMemPointerINTEL(kernel, 2, ptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_ARG_VALUE);

    err = clSetKernelArgMemPointerINTEL(kernel, 3, ptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_ARG_VALUE);
  }
}

// Test for valid API usage of clSetKernelArgMemPointerINTEL()
TEST_P(USMSetKernelArgMemPointerTest, ValidUsage) {
  cl_uint arg_index = GetParam();

  for (auto ptr : allPointers()) {
    void *offset_ptr = getPointerOffset(ptr, sizeof(cl_int));
    EXPECT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, arg_index, ptr));

    EXPECT_SUCCESS(
        clSetKernelArgMemPointerINTEL(kernel, arg_index, offset_ptr));
  }

  EXPECT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, arg_index, nullptr));
}
INSTANTIATE_TEST_SUITE_P(USMTests, USMSetKernelArgMemPointerTest,
                         ::testing::Values(0, 1));

// Test for valid API usage of clSetKernelExecInfo() with extension defined
// parameters
#if defined(CL_VERSION_3_0)
using USMSetKernelExecInfoTest = USMKernelUsageTest<cl_kernel_exec_info>;
TEST_P(USMSetKernelExecInfoTest, ValidUsage) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  const cl_kernel_exec_info param_name = GetParam();

  if (CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL == param_name) {
    cargo::small_vector<void *, 3> indirect_usm_pointers = allPointers();

    EXPECT_SUCCESS(
        clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                            sizeof(void *) * indirect_usm_pointers.size(),
                            indirect_usm_pointers.data()));
  } else {
    cl_bool flag = CL_FALSE;
    EXPECT_SUCCESS(
        clSetKernelExecInfo(kernel, param_name, sizeof(cl_bool), &flag));

    flag = CL_TRUE;
    EXPECT_SUCCESS(
        clSetKernelExecInfo(kernel, param_name, sizeof(cl_bool), &flag));
  }
}

TEST_P(USMSetKernelExecInfoTest, InvalidUsage) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  const cl_kernel_exec_info param_name = GetParam();
  if (CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL == param_name) {
    // Invalid kernel argument
    cl_int err = clSetKernelExecInfo(nullptr, param_name, 0, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_KERNEL);

    cargo::small_vector<void *, 3> indirect_usm_pointers = allPointers();

    // Invalid param_value_size
    err = clSetKernelExecInfo(kernel, param_name, 0,
                              indirect_usm_pointers.data());
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value
    err = clSetKernelExecInfo(kernel, param_name,
                              sizeof(void *) * indirect_usm_pointers.size(),
                              nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value_size and param_value
    err = clSetKernelExecInfo(kernel, param_name, 0, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  } else {
    // Invalid kernel argument
    cl_int err = clSetKernelExecInfo(nullptr, param_name, 0, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_KERNEL);

    // Invalid param_value_size
    cl_bool flag = CL_FALSE;
    err = clSetKernelExecInfo(kernel, param_name, 0, &flag);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value
    err = clSetKernelExecInfo(kernel, param_name, sizeof(cl_bool), nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value_size and param_value
    err = clSetKernelExecInfo(kernel, param_name, 0, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  }
}

INSTANTIATE_TEST_SUITE_P(
    USMTests, USMSetKernelExecInfoTest,
    ::testing::Values(CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL));
#endif

// Test for setting USM allocations as input arguments to a kernel running
// vector add, with a cl_mem buffer as the output argument.
struct USMVectorAddKernelTest : public USMKernelTest {
  // For N elements reads the cl_mem buffer used by default as the output
  // argument, then verifies that the results it contains correspond to
  // the parameter constants `inputA` & `inputB` plus the global id.
  void verifyOutputBuffer(const size_t N, const cl_int inputA = patternA,
                          const cl_int inputB = patternB) const {
    // Blocking read of output buffer
    cargo::small_vector<cl_int, 64> output;
    ASSERT_EQ(cargo::success, output.resize(N));
    ASSERT_SUCCESS(clEnqueueReadBuffer(queue, cl_mem_buffer, CL_TRUE, 0,
                                       N * sizeof(cl_int), output.data(), 0,
                                       nullptr, nullptr));

    // Verify output result
    for (size_t i = 0; i < N; i++) {
      const cl_int reference = inputA + inputB + i;
      ASSERT_EQ(output[i], reference) << " index " << i;
    }
  }

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(USMKernelTest::SetUp());

    cl_int err;
    if (host_capabilities) {
      host_ptrB = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_NE(host_ptrB, nullptr);
    }

    if (shared_capabilities) {
      shared_ptrB =
          clSharedMemAllocINTEL(context, device, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_NE(shared_ptrB, nullptr);
    }

    device_ptrB =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_NE(device_ptrB, nullptr);

    cl_mem_buffer = clCreateBuffer(context, 0, bytes, nullptr, &err);
    ASSERT_SUCCESS(err);
    ASSERT_NE(cl_mem_buffer, nullptr);

    initPointers(bytes, align);
    BuildKernel(source);

    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_NE(queue, nullptr);
    ASSERT_SUCCESS(err);

    // Initialize default value of input USM allocations
    err = clEnqueueMemFillINTEL(queue, device_ptr, &patternA, sizeof(patternA),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);

    err = clEnqueueMemFillINTEL(queue, device_ptrB, &patternB, sizeof(patternB),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);

    if (host_capabilities) {
      err = clEnqueueMemFillINTEL(queue, host_ptr, &patternA, sizeof(patternA),
                                  bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);

      err = clEnqueueMemFillINTEL(queue, host_ptrB, &patternB, sizeof(patternB),
                                  bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);
    }

    if (shared_capabilities) {
      err = clEnqueueMemFillINTEL(queue, shared_ptr, &patternA,
                                  sizeof(patternA), bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);

      err = clEnqueueMemFillINTEL(queue, shared_ptrB, &patternB,
                                  sizeof(patternB), bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);
    }
  }

  void TearDown() override {
    if (host_ptrB) {
      EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, host_ptrB));
    }

    if (shared_ptrB) {
      EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, shared_ptrB));
    }

    if (device_ptrB) {
      EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptrB));
    }

    if (cl_mem_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(cl_mem_buffer));
    }

    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    USMKernelTest::TearDown();
  }

  void *host_ptrB = nullptr;
  void *device_ptrB = nullptr;
  void *shared_ptrB = nullptr;
  cl_mem cl_mem_buffer = nullptr;

  cl_command_queue queue = nullptr;
  static const char *source;
  static const cl_int patternA;
  static const cl_int patternB;
};

const char *USMVectorAddKernelTest::source =
    R"(
void kernel foo(__global int* a,
                __global int* b,
                __global int* c) {
   size_t id = get_global_id(0);
   c[id] = a[id] + b[id] + id;
}
)";

const cl_int USMVectorAddKernelTest::patternA = 42;
const cl_int USMVectorAddKernelTest::patternB = 0xA;

// Two device USM allocation input arguments, and a cl_mem buffer output arg
TEST_F(USMVectorAddKernelTest, DeviceInputs) {
  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(elements);
}

// Two host USM allocation input arguments, and a cl_mem buffer output arg
TEST_F(USMVectorAddKernelTest, HostInputs) {
  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, host_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, host_ptrB));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(elements);
}

// Two shared USM allocation input arguments, and a cl_mem buffer output arg
TEST_F(USMVectorAddKernelTest, SharedInputs) {
  if (!shared_capabilities) {
    GTEST_SKIP();
  }

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, shared_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, shared_ptrB));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(elements);
}

// Multiple different types for arguments, with a cl_mem buffer output argument.
TEST_F(USMVectorAddKernelTest, MixedInputs) {
  const std::array<std::pair<void *, void *>, 6> options{{
      {host_ptr, device_ptrB},
      {host_ptr, shared_ptrB},
      {shared_ptr, device_ptrB},
      {device_ptr, host_ptrB},
      {shared_ptr, host_ptrB},
      {device_ptr, shared_ptrB},
  }};

  for (auto ptr : options) {
    if (!ptr.first || !ptr.second) {
      continue;
    }
    // Set kernel arguments
    ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, ptr.first));
    ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, ptr.second));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

    // Run 1-D kernel with a global size of 'elements'
    ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                          nullptr, 0, nullptr, nullptr));

    verifyOutputBuffer(elements);
  }
}

// Two device USM allocation input arguments, and a host USM allocation output
// argument
TEST_F(USMVectorAddKernelTest, HostOutput) {
  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Zero host allocation as it'll be used for output argument
  std::memset(host_ptr, 0, bytes);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 2, host_ptr));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(queue));

  // Verify output result
  const cl_int *output = static_cast<cl_int *>(host_ptr);
  for (size_t i = 0; i < elements; i++) {
    const cl_int reference = patternA + patternB + i;
    ASSERT_EQ(output[i], reference) << " index " << i;
  }
}

// A single host USM allocation used across two input arguments, with a cl_mem
// buffer output argument
TEST_F(USMVectorAddKernelTest, OffsetHostInput) {
  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Find pointer addressing halfway into the memory allocation
  const size_t half_bytes = bytes / 2;
  void *offset_host_ptr = getPointerOffset(host_ptr, half_bytes);
  ASSERT_SUCCESS(clEnqueueMemFillINTEL(queue, offset_host_ptr, &patternB,
                                       sizeof(patternB), half_bytes, 0, nullptr,
                                       nullptr));

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, host_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_host_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements);
}

// A single shared USM allocation used across two input arguments, with a cl_mem
// buffer output argument
TEST_F(USMVectorAddKernelTest, OffsetSharedInput) {
  if (!shared_capabilities) {
    GTEST_SKIP();
  }

  // Find pointer addressing halfway into the memory allocation
  const size_t half_bytes = bytes / 2;
  void *offset_shared_ptr = getPointerOffset(shared_ptr, half_bytes);
  ASSERT_SUCCESS(clEnqueueMemFillINTEL(queue, offset_shared_ptr, &patternB,
                                       sizeof(patternB), half_bytes, 0, nullptr,
                                       nullptr));

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, shared_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_shared_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements);
}

// A single device USM allocation used across two input arguments, with a
// cl_mem buffer output argument
TEST_F(USMVectorAddKernelTest, OffsetDeviceInput) {
  // Find pointer addressing halfway into the memory allocation
  const size_t half_bytes = bytes / 2;
  void *offset_device_ptr = getPointerOffset(device_ptr, half_bytes);
  ASSERT_SUCCESS(clEnqueueMemFillINTEL(queue, offset_device_ptr, &patternB,
                                       sizeof(patternB), half_bytes, 0, nullptr,
                                       nullptr));

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_device_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements);
}

// Tests overwriting USM arguments already set using
// `clSetKernelArgMemPointerINTEL`
TEST_F(USMVectorAddKernelTest, OverwriteUSMArg) {
  // Set kernel arguments, index 0 will be overwritten
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptrB));

  // Find pointer addressing halfway into the memory allocation
  const size_t half_bytes = bytes / 2;
  void *offset_device_ptr = getPointerOffset(device_ptr, half_bytes);

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_device_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Overwrite Pointer arg
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements, patternA, patternA);
}

// Tests overwriting arguments set with `clSetKernelArgMemPointerINTEL` using
// `clSetKernelArg`
TEST_F(USMVectorAddKernelTest, OverwriteCLMemArg) {
  // Find pointer addressing halfway into the memory allocation
  const size_t half_bytes = bytes / 2;
  void *offset_device_ptr = getPointerOffset(device_ptr, half_bytes);
  ASSERT_SUCCESS(clEnqueueMemFillINTEL(queue, offset_device_ptr, &patternB,
                                       sizeof(patternB), half_bytes, 0, nullptr,
                                       nullptr));

  // Set kernel arguments, index 0 and 2 will be overwritten
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &cl_mem_buffer));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 2, device_ptrB));

  // Overwrite args
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements);
}

// Tests setting kernel arguments without enqueuing the kernel
TEST_F(USMVectorAddKernelTest, SetArgsWithoutEnqueue) {
  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Release kernel and check USM allocations are still usable
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  kernel = nullptr;

  ASSERT_SUCCESS(clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, device_ptrB,
                                      bytes, 0, nullptr, nullptr));
}

// Tests creating two cl_kernel objects from the same program kernel with
// different arguments
TEST_F(USMVectorAddKernelTest, MultipleKernels) {
  // Create a new kernel object
  cl_int err;
  cl_kernel kernel2 = clCreateKernel(program, "foo", &err);
  ASSERT_SUCCESS(err);

  // Set original kernel arguments
  const size_t half_bytes = bytes / 2;
  void *offset_device_ptr = getPointerOffset(device_ptr, half_bytes);
  void *offset_deviceB_ptr = getPointerOffset(device_ptrB, half_bytes);

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_device_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Set arguments on new kernel
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel2, 0, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel2, 1, offset_deviceB_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel2, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run original 1-D kernel with a global size equal to half the number of
  // cl_int elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements, patternA, patternA);

  // Run new kernel with same configuration
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel2, 1, nullptr,
                                        &half_elements, nullptr, 0, nullptr,
                                        nullptr));

  verifyOutputBuffer(half_elements, patternB, patternB);

  if (kernel2) {
    EXPECT_SUCCESS(clReleaseKernel(kernel2));
  }
}

// Tests enqueueing a kernel more than once, changing the kernel arguments
// in-between.
TEST_F(USMVectorAddKernelTest, RepeatedEnqueue) {
  // Set kernel arguments
  const size_t half_bytes = bytes / 2;
  void *offset_device_ptr = getPointerOffset(device_ptr, half_bytes);
  void *offset_deviceB_ptr = getPointerOffset(device_ptrB, half_bytes);

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_device_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Run 1-D kernel with a global size equal to half the number of cl_int
  // elements in the buffer
  const size_t half_elements = elements / 2;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(queue));

  // Set new arguments on the kernel and run again
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, offset_deviceB_ptr));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  verifyOutputBuffer(half_elements, patternB, patternB);
}

#if defined(CL_VERSION_3_0)
// Tests interaction with `clCloneKernel()`
TEST_F(USMVectorAddKernelTest, ClonedKernel) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set arguments on original kernel
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptr));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptrB));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mem_buffer));

  // Clone kernel, arguments should be copied
  cl_int err = !CL_SUCCESS;
  cl_kernel cloned_kernel = clCloneKernel(kernel, &err);
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, cloned_kernel, 1, nullptr,
                                        &elements, nullptr, 0, nullptr,
                                        nullptr));

  verifyOutputBuffer(elements);

  EXPECT_SUCCESS(clReleaseKernel(cloned_kernel));
}
#endif
