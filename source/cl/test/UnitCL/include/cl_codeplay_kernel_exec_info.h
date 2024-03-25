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
#ifndef UNITCL_CODEPLAY_EXEC_INFO_H_INCLUDED
#define UNITCL_CODEPLAY_EXEC_INFO_H_INCLUDED

#include <CL/cl_ext.h>
#include <CL/cl_ext_codeplay.h>

#include "Common.h"

// Fixture checks extension is enabled and creates a simple kernel to set the
// execution info on
struct clSetKernelExecInfoCODEPLAYTest : public ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());

    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    if (!isPlatformExtensionSupported("cl_codeplay_kernel_exec_info")) {
      GTEST_SKIP();
    }

    clSetKernelExecInfoCODEPLAY = (clSetKernelExecInfoCODEPLAY_fn)
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clSetKernelExecInfoCODEPLAY");
    ASSERT_NE(clSetKernelExecInfoCODEPLAY, nullptr);

    const char *code = R"(
kernel void test(global int* out) {
  size_t id = get_global_id(0);
  out[id] = (int)id;
}
)";
    const size_t length = std::strlen(code);
    cl_int error = !CL_SUCCESS;
    program = clCreateProgramWithSource(context, 1, &code, &length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "test", &error);
    ASSERT_SUCCESS(error);
    ASSERT_NE(kernel, nullptr);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  clSetKernelExecInfoCODEPLAY_fn clSetKernelExecInfoCODEPLAY;
};

// Setup function pointers to USM entry points and allocate a device USM pointer
// for use when testing cl_codeplay_exec_info combined with USM
struct USMKernelExecInfoCodeplayTest : public clSetKernelExecInfoCODEPLAYTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(clSetKernelExecInfoCODEPLAYTest::SetUp());
    if (!UCL::hasDeviceExtensionSupport(device,
                                        "cl_intel_unified_shared_memory")) {
      GTEST_SKIP();
    }

#define CL_GET_EXTENSION_ADDRESS(FUNC)                            \
  FUNC = reinterpret_cast<FUNC##_fn>(                             \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC)); \
  ASSERT_NE(nullptr, FUNC);
    CL_GET_EXTENSION_ADDRESS(clDeviceMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clMemBlockingFreeINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemFillINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemcpyINTEL);
#undef GET_EXTENSION_ADDRESS

    cl_int err = !CL_SUCCESS;
    device_ptr =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);
  }

  void TearDown() override {
    if (device_ptr) {
      const cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
      EXPECT_SUCCESS(err);
    }

    clSetKernelExecInfoCODEPLAYTest::TearDown();
  }

  // Function pointers to USM extension entry points
  clDeviceMemAllocINTEL_fn clDeviceMemAllocINTEL = nullptr;
  clMemBlockingFreeINTEL_fn clMemBlockingFreeINTEL = nullptr;
  clEnqueueMemFillINTEL_fn clEnqueueMemFillINTEL = nullptr;
  clEnqueueMemcpyINTEL_fn clEnqueueMemcpyINTEL = nullptr;

  // Allocate 64 bytes for our USM device pointer
  static constexpr size_t elements = 64;
  static constexpr cl_uint align = sizeof(cl_uchar);
  static constexpr size_t bytes = elements * sizeof(cl_uchar);

  void *device_ptr = nullptr;
};

template <typename T>
struct USMExecInfoCodeplayWithParam : public USMKernelExecInfoCodeplayTest,
                                      public ::testing::WithParamInterface<T> {
};
#endif  // UNITCL_CODEPLAY_EXEC_INFO_H_INCLUDED
