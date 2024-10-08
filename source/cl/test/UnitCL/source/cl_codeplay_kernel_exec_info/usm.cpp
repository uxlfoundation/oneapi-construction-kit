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

#include <cargo/small_vector.h>

#include "cl_codeplay_kernel_exec_info.h"

namespace {
// Fixture for running kernels where the USM pointers are accessed indirectly,
// and so must be set in clSetKernelExecInfoCodeplay
class KernelExecInfoCodeplayUSMPtrs : public USMKernelExecInfoCodeplayTest {
  void BuildKernel() {
    const char *source =
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

    const size_t length = std::strlen(source);
    cl_int err = !CL_SUCCESS;
    exec_info_program =
        clCreateProgramWithSource(context, 1, &source, &length, &err);
    ASSERT_TRUE(exec_info_program != nullptr);
    ASSERT_SUCCESS(err);

    ASSERT_SUCCESS(clBuildProgram(exec_info_program, 1, &device, "",
                                  ucl::buildLogCallback, nullptr));
    exec_info_kernel = clCreateKernel(exec_info_program, "foo", &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(exec_info_kernel != nullptr);
  }

 public:
  // Verify result in first N elements of output cl_mem buffer
  void VerifyOutputBuffer(size_t N) {
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
  void VerifyUSMAlloc(void *usm_ptr, const size_t N) {
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
    UCL_RETURN_ON_FATAL_FAILURE(USMKernelExecInfoCodeplayTest::SetUp());
    BuildKernel();

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
    const cl_uchar zero_pattern = 0;
    ASSERT_SUCCESS(clEnqueueFillBuffer(queue, output_buffer, &zero_pattern,
                                       sizeof(zero_pattern), 0, bytes, 0,
                                       nullptr, nullptr));

    // Initialize USM allocations to pattern
    err = clEnqueueMemFillINTEL(queue, device_ptr, &pattern, sizeof(pattern),
                                bytes, 0, nullptr, nullptr);
    ASSERT_SUCCESS(err);
    ASSERT_SUCCESS(clFinish(queue));
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

    if (exec_info_program) {
      EXPECT_SUCCESS(clReleaseProgram(exec_info_program));
    }

    if (exec_info_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(exec_info_kernel));
    }
    USMKernelExecInfoCodeplayTest::TearDown();
  }

  cl_uint device_pointer_size = 0;

  static const cl_uchar pattern;

  cl_mem input_buffer = nullptr;
  cl_mem output_buffer = nullptr;
  cl_command_queue queue = nullptr;

  cl_kernel exec_info_kernel = nullptr;
  cl_program exec_info_program = nullptr;

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

const cl_uchar KernelExecInfoCodeplayUSMPtrs::pattern = 42;

}  // namespace

TEST_F(KernelExecInfoCodeplayUSMPtrs, IndirectDevicePointer) {
  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass indirect USM pointers to runtime
  void *indirect_usm_pointers[1] = {device_ptr};
  const cl_int err = clSetKernelExecInfoCODEPLAY(
      exec_info_kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, exec_info_kernel, 1, nullptr,
                                        &elements, nullptr, 0, nullptr,
                                        nullptr));

  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
  // Verify kernel output argument
  VerifyOutputBuffer(elements);
}

TEST_F(KernelExecInfoCodeplayUSMPtrs, OffsetDevicePointer) {
  // Wrap a pointer to halfway into the device USM allocation in input struct
  const size_t half_elements = elements / 2;
  const size_t offset = half_elements * sizeof(cl_uchar);
  void *offset_device_ptr = reinterpret_cast<cl_uchar *>(device_ptr) + offset;
  SetInputBuffer(offset_device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Pass base device USM pointer to runtime as used indirectly
  void *indirect_usm_pointers[1] = {device_ptr};
  const cl_int err = clSetKernelExecInfoCODEPLAY(
      exec_info_kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(void *),
      static_cast<void *>(indirect_usm_pointers));
  ASSERT_SUCCESS(err);

  // Run 1-D kernel with a global size of half the number of buffer elements
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, exec_info_kernel, 1, nullptr,
                                        &half_elements, nullptr, 0, nullptr,
                                        nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(half_elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(offset_device_ptr, half_elements);
}

TEST_F(KernelExecInfoCodeplayUSMPtrs, DeviceAccessFlag) {
  // Set flag allowing the kernel to access any device USM allocation
  // indirectly.
  cl_bool indirect_device_pointers = CL_TRUE;
  const cl_int err = clSetKernelExecInfoCODEPLAY(
      exec_info_kernel, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
      sizeof(cl_bool), &indirect_device_pointers);
  ASSERT_SUCCESS(err);

  // Wrap device USM pointer in a struct
  SetInputBuffer(device_ptr);

  // Set kernel arguments
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&input_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(exec_info_kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&output_buffer)));

  // Run 1-D kernel with a global size of 'elements'
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(queue, exec_info_kernel, 1, nullptr,
                                        &elements, nullptr, 0, nullptr,
                                        nullptr));

  // Verify kernel output argument
  VerifyOutputBuffer(elements);
  // Verify USM allocation used indirectly was modified
  VerifyUSMAlloc(device_ptr, elements);
}

// Test clSetKernelExecInfoCODEPLAY handling of USM flags
using KernelExecInfoCodeplayUSMFlags =
    USMExecInfoCodeplayWithParam<cl_kernel_exec_info_codeplay>;
TEST_P(KernelExecInfoCodeplayUSMFlags, ValidUsage) {
  const cl_kernel_exec_info_codeplay param_name = GetParam();
  if (CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL == param_name) {
    const cl_int err = clSetKernelExecInfoCODEPLAY(
        kernel, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL, sizeof(device_ptr),
        static_cast<void *>(&device_ptr));
    ASSERT_SUCCESS(err);
  } else {
    cl_bool flag = CL_FALSE;
    cl_int err =
        clSetKernelExecInfoCODEPLAY(kernel, param_name, sizeof(cl_bool), &flag);
    ASSERT_SUCCESS(err);

    flag = CL_TRUE;
    err =
        clSetKernelExecInfoCODEPLAY(kernel, param_name, sizeof(cl_bool), &flag);
    ASSERT_SUCCESS(err);
  }
}

TEST_P(KernelExecInfoCodeplayUSMFlags, InvalidUsage) {
  const cl_kernel_exec_info_codeplay param_name = GetParam();
  if (CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL == param_name) {
    // Invalid kernel argument
    cl_int err = clSetKernelExecInfoCODEPLAY(nullptr, param_name, 0, nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_KERNEL);

    // Invalid param_value_size
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, 0,
                                      static_cast<void *>(&device_ptr));
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, sizeof(device_ptr),
                                      nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value_size and param_value
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, 0, nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  } else {
    // Invalid kernel argument
    cl_int err = clSetKernelExecInfoCODEPLAY(nullptr, param_name, 0, nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_KERNEL);

    // Invalid param_value_size
    cl_bool flag = CL_FALSE;
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, 0, &flag);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, sizeof(cl_bool),
                                      nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);

    // Invalid param_value_size and param_value
    err = clSetKernelExecInfoCODEPLAY(kernel, param_name, 0, nullptr);
    ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  }
}

// Test exec info flags defined by cl_intel_unified_shared_memory
INSTANTIATE_TEST_SUITE_P(
    KernelExecInfoCodeplayTests, KernelExecInfoCodeplayUSMFlags,
    ::testing::Values(CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
                      CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL));
