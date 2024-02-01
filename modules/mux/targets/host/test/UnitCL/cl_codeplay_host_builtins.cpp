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

#include "Device.h"

struct cl_codeplay_host_builtins_test : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }
};

TEST_F(cl_codeplay_host_builtins_test, KernelWithBuiltin) {
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP();
  }
  // dummy_t is defined in host's force header
  const char *source =
      "__kernel void foo(__global int *in) {dummy_t dummy = 0;}";
  cl_int errorcode;
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(program);
  EXPECT_SUCCESS(errorcode);

  // Build should fail if the force header is missing
  const bool has_force_header =
      UCL::hasDeviceExtensionSupport(device, "cl_codeplay_host_builtins");
  if (has_force_header) {
    EXPECT_SUCCESS(
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
  } else {
    EXPECT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
  }
  EXPECT_SUCCESS(clReleaseProgram(program));
}

// When an OpenCL extension is enabled, the corresponding preprocessor
// directive must be defined. cl_codeplay_host_builtins is only defined in
// builds with the debug module, so we can #error out the preprocessor when
// it's incorrectly defined or not defined.
TEST_F(cl_codeplay_host_builtins_test, KernelExtensionMacro) {
  // First kernel expects the macro to be defined, second expects undefined
  const char *source_def =
      "#ifndef cl_codeplay_host_builtins\n#error\n#endif\nkernel void k() {}";
  const char *source_ndef =
      "#ifdef cl_codeplay_host_builtins\n#error\n#endif\nkernel void k() {}";

  cl_int errorcode;
  cl_program program_def =
      clCreateProgramWithSource(context, 1, &source_def, nullptr, &errorcode);
  EXPECT_TRUE(program_def);
  EXPECT_SUCCESS(errorcode);

  cl_program program_ndef =
      clCreateProgramWithSource(context, 1, &source_ndef, nullptr, &errorcode);
  EXPECT_TRUE(program_ndef);
  EXPECT_SUCCESS(errorcode);

  // On host with the extension (with the debug module), ensure that the macro
  // exists. On other devices or host without the debug module, ensure that
  // the macro doesn't exist.
  if (UCL::isDevice_host(device) &&
      UCL::hasDeviceExtensionSupport(device, "cl_codeplay_host_builtins")) {
    EXPECT_SUCCESS(
        clBuildProgram(program_def, 1, &device, nullptr, nullptr, nullptr));
    EXPECT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program_ndef, 1, &device, nullptr, nullptr, nullptr));
  } else {
    EXPECT_SUCCESS(
        clBuildProgram(program_ndef, 1, &device, nullptr, nullptr, nullptr));
    EXPECT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program_def, 1, &device, nullptr, nullptr, nullptr));
  }

  EXPECT_SUCCESS(clReleaseProgram(program_def));
  EXPECT_SUCCESS(clReleaseProgram(program_ndef));
}
