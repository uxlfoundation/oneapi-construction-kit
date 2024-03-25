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

#include <cargo/string_algorithm.h>

#include "Common.h"

class clGetKernelWorkGroupInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const char *source = R"OCL(
      /* simple test case - no local memory */
      kernel void __attribute__((reqd_work_group_size(3, 2, 1)))
      simple_nolocal(global int *out, int val) { *out = val; }


      /* This kernel should be removed via dead code elim */
      kernel void __attribute__((reqd_work_group_size(3, 2, 1)))
      do_not_use(global int *out, int val) {
        __local int bar[2];

        bar[0] = val;
        bar[1] = val * 2;

        int final_val = bar[0] + bar[1];
        *out = final_val;
      }

      /* This kernel is called by 'foo' */
      kernel void __attribute__((reqd_work_group_size(3, 2, 1)))
      foo_dependency(global int *out, int val) {
        __local int bar[5];
        __constant int boo = 123;

        bar[0] = val;
        bar[1] = val * 2;

        int final_val = bar[0] + bar[1] + boo;
        *out = final_val;
      }

      /* Calling kernel with local memory */
      kernel void __attribute__((reqd_work_group_size(3, 2, 1)))
      foo(global int *out, int val) {
        __local int bar[5];
        __private int biz = 321;

        foo_dependency(out, val);

        bar[0] = val;
        bar[1] = val * 2;
        bar[2] = val * 3;

        int final_val = bar[0] + bar[1] + bar[2] + biz;
        *out = final_val;
      }
    )OCL";

    cl_int status;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);

    kernel_nolocal = clCreateKernel(program, "simple_nolocal", &status);
    EXPECT_TRUE(kernel_nolocal);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (kernel_nolocal) {
      ASSERT_SUCCESS(clReleaseKernel(kernel_nolocal));
    }
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_kernel kernel_nolocal = nullptr;
};

// Redmine #5117: Check CL_OUT_OF_RESOURCES
// Redmine #5114: Check CL_OUT_OF_HOST_MEMORY

// TODO: This is a ucl::MultiDeviceTest
TEST_F(clGetKernelWorkGroupInfoTest, DISABLED_InvalidDevice) {
  size_t global_work_size[3];
  if (UCL::getNumDevices() > 1) {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_DEVICE,
        clGetKernelWorkGroupInfo(kernel, nullptr, CL_KERNEL_WORK_GROUP_SIZE,
                                 sizeof(size_t *), global_work_size, nullptr));
  }
}

TEST_F(clGetKernelWorkGroupInfoTest, InvalidValueParamName) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(
          kernel, device,
          static_cast<cl_kernel_work_group_info>(CL_OUT_OF_RESOURCES), 0,
          nullptr, &size));
}

TEST_F(clGetKernelWorkGroupInfoTest, InvalidValueParamValueSize) {
  size_t global_work_size[3];
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_GLOBAL_WORK_SIZE, 0,
                               global_work_size, nullptr));
  size_t work_group_size;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, 0,
                               &work_group_size, nullptr));
  size_t compile_work_group_size[3];
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelWorkGroupInfo(
                        kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 0,
                        compile_work_group_size, nullptr));
  cl_ulong local_mem_size;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_LOCAL_MEM_SIZE, 0,
                               &local_mem_size, nullptr));
  size_t preferred_work_group_size_multiple;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device,
                               CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, 0,
                               &preferred_work_group_size_multiple, nullptr));
  cl_ulong private_mem_size;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE, 0,
                               &private_mem_size, nullptr));
}

TEST_F(clGetKernelWorkGroupInfoTest, InvalidKernel) {
  size_t work_group_size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clGetKernelWorkGroupInfo(nullptr, device, CL_KERNEL_WORK_GROUP_SIZE,
                               sizeof(size_t), &work_group_size, nullptr));
}

// To run this query on a regular (not custom) CL device we need to use a built
// in kernel, so don't bother with the clGetKernelWorkGroupInfoTest fixture.
using clGetKernelWorkGroupInfoTestBuiltInKernel = ucl::DeviceTest;
TEST_F(clGetKernelWorkGroupInfoTestBuiltInKernel, GlobalWorkSize) {
  cl_device_type device_type;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type),
                                 &device_type, nullptr));

  // This test will only pass if we aren't running on a custom device.
  if (device_type == CL_DEVICE_TYPE_CUSTOM) {
    GTEST_SKIP();
  }

  // To query CL_KERNEL_GLOBAL_WORK_SIZE on a non-custom device type we must
  // have a builtin kernel to query from.
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  UCL::Buffer<char> built_in_kernels(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 built_in_kernels, nullptr));

  const auto name_list = cargo::split_all(
      cargo::string_view{static_cast<const char *>(built_in_kernels.data())},
      ";");

  if (name_list.size() == 0) {
    GTEST_SKIP();
  }

  const std::string built_in_kernel =
      std::string(std::begin(name_list[0]), std::end(name_list[0]));

  cl_int status = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(status);

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, built_in_kernel.data(), &status);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(status);

  cl_kernel kernel = clCreateKernel(program, built_in_kernel.data(), &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_GLOBAL_WORK_SIZE, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clGetKernelWorkGroupInfoTestBuiltInKernel, GlobalWorkSizeInvalidDevice) {
  cl_device_type device_type;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type),
                                 &device_type, nullptr));

  // This test checks we get invalid value when we try querying a built in
  // kernel on a custom device
  if (device_type != CL_DEVICE_TYPE_CUSTOM) {
    GTEST_SKIP();
  }

  size_t size = 0;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  UCL::Buffer<char> built_in_kernels(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 built_in_kernels, nullptr));

  const auto name_list = cargo::split_all(
      cargo::string_view{static_cast<const char *>(built_in_kernels.data())},
      ";");

  if (name_list.size() == 0) {
    GTEST_SKIP();
  }

  const std::string built_in_kernel =
      std::string(std::begin(name_list[0]), std::end(name_list[0]));

  cl_int status = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(status);

  cl_program program = clCreateProgramWithBuiltInKernels(
      context, 1, &device, built_in_kernel.data(), &status);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(status);

  cl_kernel kernel = clCreateKernel(program, built_in_kernel.data(), &status);
  ASSERT_SUCCESS(status);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_GLOBAL_WORK_SIZE, 0,
                               nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clGetKernelWorkGroupInfoTest, GlobalWorkSizeCustomDevice) {
  // If this is a custom device we're allowed to query
  // CL_KERNEL_GLOBAL_WORK_SIZE for a regular kernel, but not a builtin kernel.
  cl_device_type device_type;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type),
                                 &device_type, nullptr));

  if (device_type != CL_DEVICE_TYPE_CUSTOM) {
    GTEST_SKIP();
  }

  // Check that we can succesfully query KERNEL_GLOBAL_WORK_SIZE from a regular
  // kernel on a custom device.
  size_t global_work_size[3];
  ASSERT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_GLOBAL_WORK_SIZE,
                               sizeof(size_t *), global_work_size, nullptr));
}

TEST_F(clGetKernelWorkGroupInfoTest, GlobalWorkSizeInvalidKernel) {
  cl_device_type device_type;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type),
                                 &device_type, nullptr));

  if (device_type == CL_DEVICE_TYPE_CUSTOM) {
    GTEST_SKIP();
  }

  // Check that we can't query GLOBAL_WORK_SIZE for a regular kernel on a
  // non-custom device
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_GLOBAL_WORK_SIZE, 0,
                               nullptr, nullptr));
}

TEST_F(clGetKernelWorkGroupInfoTest, WorkGroupSizeParamSize) {
  size_t size;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_WORK_GROUP_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
}

TEST_F(clGetKernelWorkGroupInfoTest, WorkGroupSizeValue) {
  size_t work_group_size;
  ASSERT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE,
                               sizeof(size_t), &work_group_size, nullptr));
  ASSERT_LE(1u, work_group_size);
}

TEST_F(clGetKernelWorkGroupInfoTest, CompileWorkGroupSizeParamSize) {
  size_t size;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t) * 3, size);
}

TEST_F(clGetKernelWorkGroupInfoTest, CompileWorkGroupSize) {
  size_t compile_work_group_size[3];
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(size_t) * 3,
      compile_work_group_size, nullptr));
  ASSERT_EQ(3u, compile_work_group_size[0]);
  ASSERT_EQ(2u, compile_work_group_size[1]);
  ASSERT_EQ(1u, compile_work_group_size[2]);
}

TEST_F(clGetKernelWorkGroupInfoTest, LocalMemSizeParamSize) {
  size_t size;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_LOCAL_MEM_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
}

// Disabled because the implementation is also allowed to use some of the local
// memory and include that in calculations, thus it is hard to know the correct
// answer.  See CA-666.
TEST_F(clGetKernelWorkGroupInfoTest, DISABLED_LocalMemSizeValue) {
  cl_ulong local_mem_size;
  ASSERT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_LOCAL_MEM_SIZE,
                               sizeof(cl_ulong), &local_mem_size, nullptr));

  const cl_int expected[10]{};
  ASSERT_EQ(sizeof(expected), local_mem_size);
}

TEST_F(clGetKernelWorkGroupInfoTest, LocalMemSizeValueEmpty) {
  cl_ulong local_mem_size;
  ASSERT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel_nolocal, device, CL_KERNEL_LOCAL_MEM_SIZE,
                               sizeof(cl_ulong), &local_mem_size, nullptr));

  ASSERT_EQ(0u, local_mem_size);
}

TEST_F(clGetKernelWorkGroupInfoTest, PreferredWorkGroupSizeMulipleParamSize) {
  size_t size;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(size_t), size);
}

TEST_F(clGetKernelWorkGroupInfoTest, PreferredWorkGroupSizeMuliple) {
  size_t preferred_work_group_size_multiple;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
      sizeof(size_t), &preferred_work_group_size_multiple, nullptr));
  ASSERT_LE(1u, preferred_work_group_size_multiple);
}

TEST_F(clGetKernelWorkGroupInfoTest, PrivateMemSizeParamSize) {
  size_t size;
  ASSERT_SUCCESS(clGetKernelWorkGroupInfo(
      kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
}

TEST_F(clGetKernelWorkGroupInfoTest, PrivateMemSizeValue) {
  cl_ulong private_mem_size = std::numeric_limits<cl_ulong>::max();
  ASSERT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE,
                               sizeof(cl_ulong), &private_mem_size, nullptr));
  // The best we can do is ensure `private_mem_size` is actually set to
  // something other than the maximum possible value set above since the actual
  // private memory size for any given kernel is implementation defined.
  ASSERT_NE(std::numeric_limits<cl_ulong>::max(), private_mem_size);
}
