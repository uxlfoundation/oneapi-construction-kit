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

#include "Common.h"

class clCreateProgramWithILKHRTest : public ucl::ContextTest {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_khr_il_program")) {
      GTEST_SKIP();
    }
    clCreateProgramWithILKHR = reinterpret_cast<clCreateProgramWithILKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clCreateProgramWithILKHR"));
    ASSERT_NE(nullptr, clCreateProgramWithILKHR);

    uint32_t addr_model = getDeviceAddressBits() == 64 ? 0x2 : 0x1;

    // kernel void foo() {}
    //
    //         OpCapability Addresses
    //         OpCapability Kernel
    //    %1 = OpExtInstImport "OpenCL.std"
    //         OpMemoryModel Physical32/64 OpenCL ; varies by arch
    //         OpEntryPoint Kernel %4 "foo"
    //         OpSource OpenCL_C 102000
    // %void = OpTypeVoid
    //    %3 = OpTypeFunction %void
    //    %4 = OpFunction %void Pure %3
    //    %5 = OpLabel
    //         OpReturn
    //         OpFunctionEnd
    spirv = {{
        0x07230203, 0x00010000, 0x0006000e, 0x00000006, 0x00000000,
        0x00020011,                          // OpCapability
        0x00000004,                          //   Addresses
        0x00020011,                          // OpCapability
        0x00000006,                          //   Kernel
        0x0005000b,                          // OpExtInstImport
        0x00000001,                          // %1
        0x6e65704f, 0x732e4c43, 0x00006474,  //   "OpenCL.std"
        0x0003000e,                          // OpMemoryModel
        addr_model,                          //   Physical32/64
        0x00000002,                          //   OpenCL
        0x0004000f,                          // OpEntryPoint
        0x00000006,                          //   Kernel
        0x00000004,                          //   %4
        0x006f6f66,                          //   "foo"
        0x00030003,                          // OpSource
        0x00000003,                          //   OpenCL_C
        0x00018e70,                          //   102000
        0x00020013,                          // OpTypeVoid
        0x00000002,                          //   %void
        0x00030021,                          // OpTypeFunction
        0x00000003,                          // %3
        0x00000002,                          //   %void
        0x00050036,                          //   OpFunction
        0x00000002,                          //   %void
        0x00000004,                          //   Pure
        0x00000004,                          //   %4
        0x00000003,                          //   %3
        0x000200f8,                          // OpLabel
        0x00000005,                          // %5
        0x000100fd,                          // OpReturn
        0x00010038,                          // OpFunctionEnd
    }};
  }

  clCreateProgramWithILKHR_fn clCreateProgramWithILKHR = nullptr;
  std::array<uint32_t, 38> spirv;
};

TEST_F(clCreateProgramWithILKHRTest, Default) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IL_VERSION_KHR, 0, nullptr, &size));
  std::string il_version(size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IL_VERSION_KHR, size,
                                 il_version.data(), nullptr));
  ASSERT_STREQ("SPIR-V_1.0", il_version.c_str());

  cl_int error;
  cl_program program = clCreateProgramWithILKHR(
      context, spirv.data(), spirv.size() * sizeof(spirv[0]), &error);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_IL_KHR, 0, nullptr, &size));
  ASSERT_EQ(spirv.size(), size / sizeof(uint32_t));
  std::vector<uint32_t> il(size);
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_IL_KHR, size, il.data(), nullptr));
  ASSERT_TRUE(std::equal(spirv.begin(), spirv.end(), il.begin()));

  ASSERT_SUCCESS(UCL::buildProgram(program, device, ""));

  cl_kernel kernel = clCreateKernel(program, "foo", &error);
  ASSERT_SUCCESS(error);

  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  ASSERT_SUCCESS(error);
  constexpr size_t work_dim = 1;
  const std::array<size_t, work_dim> global_work_size{{1}};

  cl_event event;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, work_dim,
                                        nullptr, global_work_size.data(),
                                        nullptr, 0, nullptr, &event));
  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(clReleaseEvent(event));
  ASSERT_SUCCESS(clReleaseCommandQueue(command_queue));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clCreateProgramWithILKHRTest, InvalidContext) {
  cl_int error;
  ASSERT_EQ(nullptr,
            clCreateProgramWithILKHR(nullptr, spirv.data(),
                                     spirv.size() * sizeof(spirv[0]), &error));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, error);
}

TEST_F(clCreateProgramWithILKHRTest, InvalidValue) {
  cl_int error;
  ASSERT_EQ(nullptr,
            clCreateProgramWithILKHR(context, nullptr,
                                     spirv.size() * sizeof(spirv[0]), &error));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  ASSERT_EQ(nullptr,
            clCreateProgramWithILKHR(context, spirv.data(), 0, &error));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
  std::array<uint32_t, 32> invalid{{}};
  ASSERT_EQ(nullptr, clCreateProgramWithILKHR(context, invalid.data(),
                                              invalid.size() * sizeof(spirv[0]),
                                              &error));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, error);
}
