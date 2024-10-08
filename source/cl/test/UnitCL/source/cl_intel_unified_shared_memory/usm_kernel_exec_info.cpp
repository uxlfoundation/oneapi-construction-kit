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

namespace {
struct USMKernelTest : public cl_intel_unified_shared_memory_Test {
  // Verify result in first N elements of output cl_mem buffer
  void VerifyOutputBuffer(size_t N, const cl_uchar pattern = patternA) {
    // Zero initialize all output elements
    cargo::small_vector<cl_uchar, 64> output;
    ASSERT_EQ(cargo::success, output.resize(N, 0));

    // Read data from buffer
    ASSERT_SUCCESS(clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0,
                                       N * sizeof(cl_uchar), output.data(), 0,
                                       nullptr, nullptr));

    // Verify contents are correct
    for (size_t i = 0; i < output.size(); i++) {
      const cl_uchar reference = pattern + i;
      ASSERT_EQ(output[i], reference) << " index " << i;
    }
  }

  // Verify result from a USM allocation used as kernel output or modified
  // as an indirect USM allocation.
  void VerifyUSMAlloc(void *usm_ptr, const size_t N,
                      const cl_uchar pattern = patternA) {
    // Zero initialize N output elements
    cargo::small_vector<cl_uchar, 64> output;
    ASSERT_EQ(cargo::success, output.resize(N, 0));

    // Copy USM allocation data into user vector
    ASSERT_SUCCESS(clEnqueueMemcpyINTEL(queue, CL_TRUE, output.data(), usm_ptr,
                                        N * sizeof(cl_uchar), 0, nullptr,
                                        nullptr));

    // Verify contents are correct
    for (size_t i = 0; i < output.size(); i++) {
      const cl_uchar reference = pattern + i;
      ASSERT_EQ(output[i], reference) << " index " << i;
    }
  }

  // Build kernel named "foo" from OpenCL-C source string argument
  void BuildKernel(const char *source) {
    const size_t length = std::strlen(source);
    cl_int err = !CL_SUCCESS;
    program = clCreateProgramWithSource(context, 1, &source, &length, &err);
    ASSERT_TRUE(program != nullptr);
    ASSERT_SUCCESS(err);

    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, "",
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "foo", &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(kernel != nullptr);
  }

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS,
                                   sizeof(device_pointer_size),
                                   &device_pointer_size, nullptr));

    ASSERT_TRUE(device_pointer_size == 32u || device_pointer_size == 64u);

    cl_int err;
    input_buffer = clCreateBuffer(context, 0, bytes, nullptr, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(input_buffer != nullptr);

    output_buffer = clCreateBuffer(context, 0, bytes, nullptr, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(output_buffer != nullptr);

    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);

    // Reset output buffer to zeros
    ASSERT_SUCCESS(clEnqueueFillBuffer(queue, output_buffer, &zero_pattern,
                                       sizeof(zero_pattern), 0, bytes, 0,
                                       nullptr, nullptr));
  }

  void TearDown() override {
    if (input_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
    }

    if (output_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
    }

    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    cl_intel_unified_shared_memory_Test::TearDown();
  }

  cl_uint device_pointer_size = 0;

  const size_t elements = 64;
  const size_t bytes = elements * sizeof(cl_uchar);
  const cl_uint align = sizeof(cl_uchar);

  static const cl_uchar zero_pattern;
  static const cl_uchar patternA;
  static const cl_uchar patternB;

  cl_mem input_buffer = nullptr;
  cl_mem output_buffer = nullptr;
  cl_command_queue queue = nullptr;

  cl_kernel kernel = nullptr;
  cl_program program = nullptr;
};

const cl_uchar USMKernelTest::zero_pattern = 0;
const cl_uchar USMKernelTest::patternA = 42;
const cl_uchar USMKernelTest::patternB = 0xA;

// Test for passing USM allocations indirectly to kernel via the
// CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL argument to clSetKernelExecInfo().
struct USMIndirectAccessTest : public USMKernelTest {
  void SetInputBuffer(void *usm_ptr) {
#if INTPTR_MAX == INT64_MAX
    const cl_ulong ptr_as_ulong = reinterpret_cast<cl_ulong>(usm_ptr);
    if (device_pointer_size == 64u) {
      SinglePointerWrapper64Bit ptr_wrapper = {ptr_as_ulong};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    } else {
      const cl_uint truncated_ptr = static_cast<cl_uint>(ptr_as_ulong);
      SinglePointerWrapper32Bit ptr_wrapper = {truncated_ptr};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    }
#elif INTPTR_MAX == INT32_MAX
    const cl_uint ptr_as_uint = reinterpret_cast<cl_uint>(usm_ptr);
    if (device_pointer_size == 64u) {
      const cl_uint extended_ptr = static_cast<cl_ulong>(ptr_as_uint);
      SinglePointerWrapper64Bit ptr_wrapper = {extended_ptr};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    } else {
      SinglePointerWrapper32Bit ptr_wrapper = {ptr_as_uint};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    }
#else
#error Compiling with an unsupported pointer size
#endif
  }

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(USMKernelTest::SetUp());

    initPointers(bytes, align);
    cl_int err;
    BuildKernel(source);

    // Initialize USM allocations to patternA
    err = clEnqueueMemFillINTEL(queue, device_ptr, &patternA, sizeof(patternA),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);

    if (host_ptr) {
      err = clEnqueueMemFillINTEL(queue, host_ptr, &patternA, sizeof(patternA),
                                  bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);
    }

    if (shared_ptr) {
      err = clEnqueueMemFillINTEL(queue, shared_ptr, &patternA,
                                  sizeof(patternA), bytes, 0, nullptr, nullptr);
      ASSERT_SUCCESS(err);
    }
    ASSERT_SUCCESS(clFinish(queue));
  }

  static const char *source;

  // The pointer size between host and device may not match, so define two
  // separate structs with a unsigned integer member in place of the
  // `__global int*` in kernel code.
#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
#define PACKED /* deliberately blank */
#endif

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

  struct PACKED SinglePointerWrapper32Bit {
    cl_uint input_ptr;
  };

  struct PACKED SinglePointerWrapper64Bit {
    cl_ulong input_ptr;
  };

#if defined(_MSC_VER)
#pragma pack(pop)
#endif
};

const char *USMIndirectAccessTest::source =
    R"(
typedef struct {
  __global uchar* input_ptr;
 } ptr_wrapper;

void kernel foo(__global ptr_wrapper* input, __global uchar* output) {
  size_t id = get_global_id(0);
  int updated_value = input->input_ptr[id] + id;
  output[id] = updated_value;
  input->input_ptr[id] = updated_value;
}
)";

// Tests for passing multiple USM allocations indirectly to kernel via the
// CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL argument to clSetKernelExecInfo().
struct USMMultiIndirectAccessTest : public USMKernelTest {
  void SetInputBuffer(void *usm_ptrA, void *usm_ptrB) {
#if INTPTR_MAX == INT64_MAX
    const cl_ulong ptrA_as_ulong = reinterpret_cast<cl_ulong>(usm_ptrA);
    const cl_ulong ptrB_as_ulong = reinterpret_cast<cl_ulong>(usm_ptrB);
    if (device_pointer_size == 64u) {
      PairPointerWrapper64Bit ptr_wrapper = {ptrA_as_ulong, ptrB_as_ulong};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    } else {
      const cl_uint truncated_ptrA = static_cast<cl_uint>(ptrA_as_ulong);
      const cl_uint truncated_ptrB = static_cast<cl_uint>(ptrB_as_ulong);
      PairPointerWrapper32Bit ptr_wrapper = {truncated_ptrA, truncated_ptrB};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    }
#elif INTPTR_MAX == INT32_MAX
    const cl_uint ptrA_as_uint = reinterpret_cast<cl_uint>(usm_ptrA);
    const cl_uint ptrB_as_uint = reinterpret_cast<cl_uint>(usm_ptrB);
    if (device_pointer_size == 64u) {
      const cl_uint extended_ptrA = static_cast<cl_ulong>(ptrA_as_uint);
      const cl_uint extended_ptrB = static_cast<cl_ulong>(ptrB_as_uint);
      PairPointerWrapper64Bit ptr_wrapper = {extended_ptrA, extended_ptrB};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    } else {
      PairPointerWrapper32Bit ptr_wrapper = {ptrA_as_uint, ptrB_as_uint};
      ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                          sizeof(ptr_wrapper), &ptr_wrapper, 0,
                                          nullptr, nullptr));
    }
#else
#error Compiling with an unsupported pointer size
#endif
  }

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(USMKernelTest::SetUp());

    cl_int err;
    device_ptrA = (cl_uchar *)clDeviceMemAllocINTEL(context, device, nullptr,
                                                    bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptrA != nullptr);

    device_ptrB = (cl_uchar *)clDeviceMemAllocINTEL(context, device, nullptr,
                                                    bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptrB != nullptr);

    BuildKernel(source);

    // Initialize default value of input USM allocations
    err = clEnqueueMemFillINTEL(queue, device_ptrA, &patternA, sizeof(patternA),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);

    err = clEnqueueMemFillINTEL(queue, device_ptrB, &patternB, sizeof(patternB),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);

    ASSERT_SUCCESS(clFinish(queue));
  }

  void TearDown() override {
    if (device_ptrA) {
      const cl_int err = clMemBlockingFreeINTEL(context, device_ptrA);
      EXPECT_SUCCESS(err);
    }

    if (device_ptrB) {
      const cl_int err = clMemBlockingFreeINTEL(context, device_ptrB);
      EXPECT_SUCCESS(err);
    }
    USMKernelTest::TearDown();
  }

  cl_uchar *device_ptrA = nullptr;
  cl_uchar *device_ptrB = nullptr;

  static const char *source;

  // The pointer size between host and device may not match, so define two
  // separate structs with a unsigned integer member in place of the
  // `__global int*` in kernel code.
#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

  struct PACKED PairPointerWrapper32Bit {
    cl_uint inputA_ptr;
    cl_uint inputB_ptr;
  };

  struct PACKED PairPointerWrapper64Bit {
    cl_ulong inputA_ptr;
    cl_ulong inputB_ptr;
  };

#if defined(_MSC_VER)
#pragma pack(pop)
#endif
#undef PACKED
};

const char *USMMultiIndirectAccessTest::source =
    R"(
typedef struct {
  __global uchar* inputA_ptr;
  __global uchar* inputB_ptr;
 } ptr_wrapper;

void kernel foo(__global ptr_wrapper* input, __global uchar* output) {
  size_t id = get_global_id(0);
  output[id] = input->inputA_ptr[id] + input->inputB_ptr[id] + id;
  input->inputA_ptr[id] += id;
  input->inputB_ptr[id] += id;
}
)";
}  // namespace

#if defined(CL_VERSION_3_0)
TEST_F(USMIndirectAccessTest, IndirectDevicePointer) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect USM pointers to runtime
  void *indirect_usm_pointers[1] = {device_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
  // Verify kernel output argument
  VerifyOutputBuffer(elements);
}

TEST_F(USMIndirectAccessTest, IndirectHostPointer) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Wrap host USM pointer in a struct
  SetInputBuffer(host_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptr));

  // Pass indirect USM pointers to runtime
  void *indirect_usm_pointers[1] = {host_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyUSMAlloc(device_ptr, elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(host_ptr, elements);
}

TEST_F(USMIndirectAccessTest, IndirectSharedPointer) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!shared_capabilities) {
    GTEST_SKIP();
  }

  // Wrap shared USM pointer in a struct
  SetInputBuffer(shared_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, device_ptr));

  // Pass indirect USM pointers to runtime
  void *indirect_usm_pointers[1] = {shared_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyUSMAlloc(device_ptr, elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(shared_ptr, elements);
}

TEST_F(USMIndirectAccessTest, IndirectDevicePtrInsideHostPtr) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Wrap device USM pointer in a struct and copy struct into host USM
  // allocation. The host and device pointer size must match to support host
  // USM allocations, a capability we have already checked before reaching here.
#if INTPTR_MAX == INT64_MAX
  const cl_ulong ptr_as_ulong = reinterpret_cast<cl_ulong>(device_ptr);
  SinglePointerWrapper64Bit ptr_wrapper = {ptr_as_ulong};
#else
  const cl_uint ptr_as_uint = reinterpret_cast<cl_uint>(device_ptr);
  SinglePointerWrapper32Bit ptr_wrapper = {ptr_as_uint};
#endif
  std::memset(host_ptr, 0, bytes);
  std::memcpy(host_ptr, &ptr_wrapper, sizeof(ptr_wrapper));

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, host_ptr));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect USM pointers to runtime
  void *indirect_usm_pointers[1] = {device_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
}

TEST_F(USMIndirectAccessTest, IndirectDevicePtrThenHostPtr) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect device USM pointer to runtime
  void *indirect_usm_pointers[1] = {device_ptr};
  cl_int err = clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                                   sizeof(void *),
                                   static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);

  // Reset output buffer to zeros
  ASSERT_SUCCESS(clEnqueueFillBuffer(queue, output_buffer, &zero_pattern,
                                     sizeof(zero_pattern), 0, bytes, 0, nullptr,
                                     nullptr));

  // Now execute the kernel again, but wrapping a host USM pointer in the input
  SetInputBuffer(host_ptr);

  // Pass indirect host USM pointer to runtime, this overwrites earlier setting
  // of indirect device USM pointer
  indirect_usm_pointers[0] = {host_ptr};
  err = clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                            sizeof(void *),
                            static_cast<void *>(indirect_usm_pointers));
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(host_ptr, elements);
}

TEST_F(USMIndirectAccessTest, IndirectDevicePtrAndHostPtr) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass both USM pointers to runtime as used indirectly, but only use one in
  // each execution
  void *indirect_usm_pointers[2] = {device_ptr, host_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);

  // Reset output buffer to zeros
  ASSERT_SUCCESS(clEnqueueFillBuffer(queue, output_buffer, &zero_pattern,
                                     sizeof(zero_pattern), 0, bytes, 0, nullptr,
                                     nullptr));

  // Now execute the kernel again, but wrapping a host USM pointer in the input
  SetInputBuffer(host_ptr);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(host_ptr, elements);
}

TEST_F(USMIndirectAccessTest, OffsetDevicePointer) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Wrap a pointer to halfway into the device USM allocation in input struct
  const size_t half_elements = elements / 2;
  void *offset_device_ptr =
      getPointerOffset(device_ptr, half_elements * sizeof(cl_uchar));
  SetInputBuffer(offset_device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass base device USM pointer to runtime as used indirectly
  void *indirect_usm_pointers[1] = {device_ptr};
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of half the number of buffer elements
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      queue, kernel, 1, nullptr, &half_elements, nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(half_elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(offset_device_ptr, half_elements);
}

// Test setting CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL after the
// device USM pointer has already been allocated.
TEST_F(USMIndirectAccessTest, DeviceAccessFlag) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any device USM allocation
  // indirectly.
  cl_bool indirect_device_pointers = CL_TRUE;
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_device_pointers);
  EXPECT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
}

// Test setting CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL after the
// host USM pointer has already been allocated.
TEST_F(USMIndirectAccessTest, HostAccessFlag) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any host USM allocation indirectly.
  cl_bool indirect_host_pointers = CL_TRUE;
  const cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_host_pointers);
  EXPECT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(host_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(host_ptr, elements);
}

// Test setting CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL before the
// device USM pointer is allocated.
TEST_F(USMIndirectAccessTest, DeviceFlagBeforeAlloc) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any device USM allocation
  // indirectly.
  cl_bool indirect_device_pointers = CL_TRUE;
  cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_device_pointers);
  EXPECT_SUCCESS(err);

  // Allocate device USM memory to use indirectly
  void *deferred_device_alloc =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(deferred_device_alloc != nullptr);

  // Initialize USM allocations to patternA
  err = clEnqueueMemFillINTEL(queue, deferred_device_alloc, &patternA,
                              sizeof(patternA), bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(deferred_device_alloc);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(deferred_device_alloc, elements);

  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, deferred_device_alloc));
}

// Test setting CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL before the
// host USM pointer is allocated.
TEST_F(USMIndirectAccessTest, HostFlagBeforeAlloc) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  if (!host_capabilities) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any host USM allocation indirectly.
  cl_bool indirect_host_pointers = CL_TRUE;
  cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_host_pointers);
  EXPECT_SUCCESS(err);

  // Allocate host USM memory to use indirectly
  void *deferred_host_alloc =
      clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(deferred_host_alloc != nullptr);

  // Initialize USM allocations to patternA
  err = clEnqueueMemFillINTEL(queue, deferred_host_alloc, &patternA,
                              sizeof(patternA), bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(deferred_host_alloc);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(deferred_host_alloc, elements);

  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, deferred_host_alloc));
}

// Test setting CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL before the
// device USM pointer is allocated, and then passing the allocated device
// pointer explicitly with CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL. The second
// operation should be superfluous but test that we handle it correctly.
TEST_F(USMIndirectAccessTest, DeviceFlagAndExplicitPtr) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any device USM allocation
  // indirectly.
  cl_bool indirect_device_pointers = CL_TRUE;
  cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_device_pointers);
  EXPECT_SUCCESS(err);

  // Allocate device USM memory to use indirectly
  void *deferred_device_alloc =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(deferred_device_alloc != nullptr);

  // Specify deferred device allocation pointer will be used explicitly
  void *indirect_usm_pointers[1] = {deferred_device_alloc};
  err = clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                            sizeof(void *),
                            static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Initialize USM allocations to patternA
  err = clEnqueueMemFillINTEL(queue, deferred_device_alloc, &patternA,
                              sizeof(patternA), bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(deferred_device_alloc);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(deferred_device_alloc, elements);

  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, deferred_device_alloc));
}

// Test setting the indirect access flag for all allocation types to false,
// after previously setting them to true. False is the default behaviour,
// but we should test that we overwrite the earlier true value.
TEST_F(USMIndirectAccessTest, DisableAllFlags) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set flag allowing the kernel to access any USM allocation: device, host,
  // or shared.
  cl_bool indirect_flag = CL_TRUE;
  cl_int err = clSetKernelExecInfo(
      kernel, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL, sizeof(cl_bool),
      &indirect_flag);
  EXPECT_SUCCESS(err);

  err = clSetKernelExecInfo(kernel,
                            CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
                            sizeof(cl_bool), &indirect_flag);
  EXPECT_SUCCESS(err);

  err = clSetKernelExecInfo(kernel,
                            CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL,
                            sizeof(cl_bool), &indirect_flag);
  EXPECT_SUCCESS(err);

  // Flip all flags, preventing the kernel from accessing any USM allocation not
  // explicitly listed in CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL. This is the
  // default behaviour, but should override setting CL_TRUE ealier.
  indirect_flag = CL_FALSE;
  err = clSetKernelExecInfo(kernel,
                            CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
                            sizeof(cl_bool), &indirect_flag);
  EXPECT_SUCCESS(err);

  err = clSetKernelExecInfo(kernel,
                            CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
                            sizeof(cl_bool), &indirect_flag);
  EXPECT_SUCCESS(err);

  err = clSetKernelExecInfo(kernel,
                            CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL,
                            sizeof(cl_bool), &indirect_flag);
  EXPECT_SUCCESS(err);

  // Explicitly set that device_ptr is used indirectly by the kernel.
  void *indirect_usm_pointers[1] = {device_ptr};
  err = clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                            sizeof(void *),
                            static_cast<void *>(indirect_usm_pointers));
  EXPECT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
}

// Check accessing two separate USM device allocations indirectly in a single
// kernel execution
TEST_F(USMMultiIndirectAccessTest, Default) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Wrap device USM pointers in a struct
  SetInputBuffer(device_ptrA, device_ptrB);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect USM pointers to runtime
  std::array<void *, 2> indirect_usm_pointers = {device_ptrA, device_ptrB};
  const cl_int err =
      clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                          sizeof(void *) * indirect_usm_pointers.size(),
                          static_cast<void *>(indirect_usm_pointers.data()));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Verify expected behaviour
  VerifyOutputBuffer(elements, patternA + patternB);
  VerifyUSMAlloc(device_ptrA, elements, patternA);
  VerifyUSMAlloc(device_ptrB, elements, patternB);
}

// Test that clMemBlockingFreeINTEL waits on kernels enqueued with USM
// allocations set indirectly with clSetKernelExecInfo().
TEST_F(USMMultiIndirectAccessTest, BlockingFree) {
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Wrap device USM pointers in a struct
  SetInputBuffer(device_ptrA, device_ptrB);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect USM pointers to runtime
  std::array<void *, 2> indirect_usm_pointers = {device_ptrA, device_ptrB};
  const cl_int err =
      clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                          sizeof(void *) * indirect_usm_pointers.size(),
                          static_cast<void *>(indirect_usm_pointers.data()));
  EXPECT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &elements,
                                        nullptr, 0, nullptr, nullptr));

  // Blocking free should flush the queue and wait for kernel execution to
  // complete before freeing USM allocation
  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptrA));
  device_ptrA = nullptr;

  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptrB));
  device_ptrB = nullptr;

  // Verify kernel executed successfully
  VerifyOutputBuffer(elements, patternA + patternB);
}
#endif
