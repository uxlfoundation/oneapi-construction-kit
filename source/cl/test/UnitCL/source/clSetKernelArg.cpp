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

class clSetKernelArgTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source =
        "void kernel foo(global int * a, global int * b, int c, local int * d) "
        "{*a = *b;}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(nullptr != program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(nullptr != kernel);
    ASSERT_SUCCESS(errorcode);
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

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clSetKernelArgTest, SetFirst) {
  cl_int errorcode;
  cl_mem buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clSetKernelArgTest, SetSecond) {
  cl_int errorcode;
  cl_mem buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clSetKernelArgTest, SetBoth) {
  cl_int errorcode;
  cl_mem buffer1 = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer1);
  ASSERT_SUCCESS(errorcode);

  cl_mem buffer2 = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer2);
  EXPECT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer1));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer2));

  ASSERT_SUCCESS(clReleaseMemObject(buffer1));
  ASSERT_SUCCESS(clReleaseMemObject(buffer2));
}

TEST_F(clSetKernelArgTest, SetBothReverseOrder) {
  cl_int errorcode;
  cl_mem buffer1 = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer1);
  ASSERT_SUCCESS(errorcode);

  cl_mem buffer2 = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer2);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer2));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer1));

  ASSERT_SUCCESS(clReleaseMemObject(buffer1));
  ASSERT_SUCCESS(clReleaseMemObject(buffer2));
}

TEST_F(clSetKernelArgTest, SetNonBuffer) {
  int payload = 1;
  ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(int), (void *)&payload));
}

TEST_F(clSetKernelArgTest, BadKernel) {
  cl_int errorcode;
  cl_mem buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clSetKernelArg(nullptr, 1, sizeof(cl_mem), (void *)&buffer));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clSetKernelArgTest, BadIndex) {
  cl_int errorcode;
  cl_mem buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_INDEX,
                    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&buffer));

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clSetKernelArgTest, BadSize) {
  char badPayload = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_SIZE,
                    clSetKernelArg(kernel, 0, 1, (void *)&badPayload));
}

TEST_F(clSetKernelArgTest, AddressOfNullArg) {
  cl_mem buffer = nullptr;
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer));
}

TEST_F(clSetKernelArgTest, NullArg) {
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), nullptr));
}

TEST_F(clSetKernelArgTest, InvalidLocalArgSize) {
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_SIZE, clSetKernelArg(kernel, 3, 0, nullptr));
}

TEST_F(clSetKernelArgTest, InvalidLocalArgValue) {
  cl_mem buffer = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_VALUE,
                    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&buffer));
}

#ifdef CL_VERSION_3_0
struct KernelArgParam {
  uint32_t global_buf_size;
  uint32_t local_buf_size;
};

class clSetKernelArgTestFromIL
    : public ucl::ContextTest,
      public testing::WithParamInterface<KernelArgParam> {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    if (!UCL::isDeviceVersionAtLeast(ucl::Version{3, 0})) {
      GTEST_SKIP();
    }
    {
      size_t size;
      ASSERT_SUCCESS(
          clGetDeviceInfo(device, CL_DEVICE_IL_VERSION, 0, nullptr, &size));
      std::string il_version(size, '\0');
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IL_VERSION, size,
                                     il_version.data(), nullptr));
      if (std::strcmp(il_version.c_str(), "SPIR-V_1.0") != 0) {
        GTEST_SKIP();
      }
    }

    // === OpenCL C ===
    // void kernel foo(global int * a, local int * b){
    //   *a = *b;
    // }
    //
    //
    // === SPIRV ===
    //                  OpCapability Addresses
    //                  OpCapability Kernel
    //             %1 = OpExtInstImport "OpenCL.std"
    //                  OpMemoryModel Physical64 OpenCL
    //                  OpEntryPoint Kernel %7 "foo"
    //            %18 = OpString "kernel_arg_type.foo.int*,int*,"
    //                  OpSource OpenCL_C 100000
    //                  OpDecorate %8 MaxByteOffset { global offset}
    //                  OpDecorate %9 MaxByteOffset { local offset }
    //                  OpDecorate %12 Alignment 8
    //                  OpDecorate %14 Alignment 8
    //          %uint = OpTypeInt 32 0
    //          %void = OpTypeVoid
    //   %_ptr_CrossWorkgroup_uint = OpTypePointer CrossWorkgroup %uint
    //   %_ptr_Workgroup_uint = OpTypePointer Workgroup %uint
    //             %6 = OpTypeFunction %void %_ptr_CrossWorkgroup_uint
    //             %_ptr_Workgroup_uint
    //   %_ptr_Function__ptr_CrossWorkgroup_uint = OpTypePointer Function
    //   %_ptr_CrossWorkgroup_uint
    //   %_ptr_Function__ptr_Workgroup_uint = OpTypePointer Function
    //   %_ptr_Workgroup_uint
    //             %7 = OpFunction %void DontInline %6
    //             %8 = OpFunctionParameter %_ptr_CrossWorkgroup_uint
    //             %9 = OpFunctionParameter %_ptr_Workgroup_uint
    //            %10 = OpLabel
    //            %12 = OpVariable %_ptr_Function__ptr_CrossWorkgroup_uint
    //            Function %14 = OpVariable %_ptr_Function__ptr_Workgroup_uint
    //            Function
    //                  OpStore %12 %8 Aligned 8
    //                  OpStore %14 %9 Aligned 8
    //            %15 = OpLoad %_ptr_Workgroup_uint %14 Aligned 8
    //            %16 = OpLoad %uint %15 Aligned 4
    //            %17 = OpLoad %_ptr_CrossWorkgroup_uint %12 Aligned 8
    //                  OpStore %17 %16 Aligned 4
    //                  OpReturn
    //                  OpFunctionEnd
    //

    const uint32_t addr_model = getDeviceAddressBits() == 64 ? 0x2 : 0x1;

    auto global_size = GetParam().global_buf_size;
    auto local_size = GetParam().local_buf_size;
    spirv = {{0x07230203, 0x00010600, 0x00070000,  0x00000013, 0x00000000,
              0x00020011, 0x00000004, 0x00020011,  0x00000006, 0x0005000b,
              0x00000001, 0x6e65704f, 0x732e4c43,  0x00006474, 0x0003000e,
              addr_model, 0x00000002, 0x0004000f,  0x00000006, 0x00000002,
              0x006f6f66, 0x000a0007, 0x00000003,  0x6e72656b, 0x615f6c65,
              0x745f6772, 0x2e657079, 0x2e6f6f66,  0x2a746e69, 0x746e692c,
              0x00002c2a, 0x00030003, 0x00000003,  0x000186a0, 0x00040047,
              0x00000004, 0x0000002d, global_size, 0x00040047, 0x00000005,
              0x0000002d, local_size, 0x00040047,  0x00000006, 0x0000002c,
              0x00000008, 0x00040047, 0x00000007,  0x0000002c, 0x00000008,
              0x00040015, 0x00000008, 0x00000020,  0x00000000, 0x00020013,
              0x00000009, 0x00040020, 0x0000000a,  0x00000005, 0x00000008,
              0x00040020, 0x0000000b, 0x00000004,  0x00000008, 0x00050021,
              0x0000000c, 0x00000009, 0x0000000a,  0x0000000b, 0x00040020,
              0x0000000d, 0x00000007, 0x0000000a,  0x00040020, 0x0000000e,
              0x00000007, 0x0000000b, 0x00050036,  0x00000009, 0x00000002,
              0x00000002, 0x0000000c, 0x00030037,  0x0000000a, 0x00000004,
              0x00030037, 0x0000000b, 0x00000005,  0x000200f8, 0x0000000f,
              0x0004003b, 0x0000000d, 0x00000006,  0x00000007, 0x0004003b,
              0x0000000e, 0x00000007, 0x00000007,  0x0005003e, 0x00000006,
              0x00000004, 0x00000002, 0x00000008,  0x0005003e, 0x00000007,
              0x00000005, 0x00000002, 0x00000008,  0x0006003d, 0x0000000b,
              0x00000010, 0x00000007, 0x00000002,  0x00000008, 0x0006003d,
              0x00000008, 0x00000011, 0x00000010,  0x00000002, 0x00000004,
              0x0006003d, 0x0000000a, 0x00000012,  0x00000006, 0x00000002,
              0x00000008, 0x0005003e, 0x00000012,  0x00000011, 0x00000002,
              0x00000004, 0x000100fd, 0x00010038}};
    cl_int errorcode;
    program = clCreateProgramWithIL(
        context, spirv.data(), spirv.size() * sizeof(spirv[0]), &errorcode);
    ASSERT_TRUE(nullptr != program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "foo", &errorcode);
    ASSERT_TRUE(nullptr != kernel);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  std::array<uint32_t, 133> spirv;
  cl_mem buffer = nullptr;
};

TEST_P(clSetKernelArgTestFromIL, GlobalParameter) {
  cl_int errorcode;
  const auto param = GetParam();
  buffer =
      clCreateBuffer(context, 0, param.global_buf_size, nullptr, &errorcode);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer));
}

TEST_P(clSetKernelArgTestFromIL, LocalParameter) {
  const auto param = GetParam();
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, param.local_buf_size, nullptr));
}

TEST_P(clSetKernelArgTestFromIL, MaxSizeRestrictionExceededGlobal) {
  cl_int errorcode;
  const auto param = GetParam();
  buffer = clCreateBuffer(context, 0, param.global_buf_size + 1, nullptr,
                          &errorcode);
  ASSERT_SUCCESS(errorcode);
  ASSERT_EQ_ERRCODE(CL_MAX_SIZE_RESTRICTION_EXCEEDED,
                    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer));
}

TEST_P(clSetKernelArgTestFromIL, MaxSizeRestrictionExceededLocal) {
  const auto param = GetParam();
  ASSERT_EQ_ERRCODE(
      CL_MAX_SIZE_RESTRICTION_EXCEEDED,
      clSetKernelArg(kernel, 1, param.local_buf_size + 1, nullptr));
}

INSTANTIATE_TEST_SUITE_P(clSetKernelArg, clSetKernelArgTestFromIL,
                         ::testing::Values(KernelArgParam{1, 1},
                                           KernelArgParam{256, 128},
                                           KernelArgParam{512, 2048},
                                           KernelArgParam{4096, 4096}));
#endif

/* Redmine #5128:
CL_INVALID_ARG_VALUE if arg_value specified is not a valid value.
CL_INVALID_ARG_SIZE if arg_size does not match the size of the data type for an
argument that is not a memory object or if the argument is a memory object and
arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared
with the __local qualifier or if the argument is a sampler and arg_size !=
sizeof(cl_sampler).
CL_INVALID_ARG_VALUE if the argument is an image declared with the read_only
qualifier and arg_value refers to an image object created with cl_mem_flags of
CL_MEM_WRITE or if the image argument is declared with the write_only qualifier
and arg_value refers to an image object created with cl_mem_flags of
CL_MEM_READ.
*/
