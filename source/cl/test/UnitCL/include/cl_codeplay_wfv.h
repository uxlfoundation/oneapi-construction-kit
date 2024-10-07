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
#ifndef UNITCL_CL_CODEPLAY_WFV_H_INCLUDED
#define UNITCL_CL_CODEPLAY_WFV_H_INCLUDED

#include <CL/cl_ext_codeplay.h>

#include "Common.h"

struct cl_codeplay_wfv_Test : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_codeplay_wfv")) {
      GTEST_SKIP();
    }
    clGetKernelWFVInfoCODEPLAY =
        reinterpret_cast<clGetKernelWFVInfoCODEPLAY_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform, "clGetKernelWFVInfoCODEPLAY"));
    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }
    // Extra compile options override our desired test build options, so we skip
    // the test if any of these are set.
    if (UCL::isExtraCompileOptEnabled("-cl-wfv")) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                                   sizeof(cl_uint), &dims, nullptr));
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  void CreateProgram(const char *source) {
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
    }
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  void CompileProgram(const char *source, const char *build_options) {
    CreateProgram(source);
    ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, build_options, 0,
                                    nullptr, nullptr, nullptr, nullptr));
  }

  void BuildProgram(const char *source, const char *build_options) {
    CreateProgram(source);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, build_options, nullptr, nullptr));
  }

  void BuildKernel(const char *source, const char *name,
                   const char *build_options) {
    BuildProgram(source, build_options);
    cl_int error;
    kernel = clCreateKernel(program, name, &error);
    ASSERT_SUCCESS(error);
  }

  clGetKernelWFVInfoCODEPLAY_fn clGetKernelWFVInfoCODEPLAY = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_uint dims;
};

struct cl_codeplay_wfv_BinaryTest : cl_codeplay_wfv_Test {
  void BuildKernel(const char *source, const char *name,
                   const char *build_options) {
    BuildProgram(source, build_options);
    size_t binary_size;
    ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                    sizeof(binary_size), &binary_size,
                                    nullptr));
    std::vector<unsigned char> binary(binary_size);
    unsigned char *binaries[] = {binary.data()};
    const unsigned char *binaries_const[] = {binary.data()};
    ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                                    sizeof(binaries),
                                    static_cast<void *>(binaries), nullptr));
    ASSERT_SUCCESS(clReleaseProgram(program));
    cl_int status;
    cl_int error;
    program = clCreateProgramWithBinary(context, 1, &device, &binary_size,
                                        binaries_const, &status, &error);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(error);
    kernel = clCreateKernel(program, name, &error);
    ASSERT_SUCCESS(error);
  }
};

#endif
