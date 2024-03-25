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
#include <gtest/gtest.h>

#include "Common.h"

using SubGroupsUnsupportedTest = ucl::ContextTest;

TEST_F(SubGroupsUnsupportedTest, clGetKernelSubGroupInfo) {
  // This test fixture assumes OpenCL 3.0 or later.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Check whether sub-groups are actually unsupported.
  cl_uint max_num_subgroups = 0;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                 sizeof(cl_uint), &max_num_subgroups, nullptr));
  if (max_num_subgroups) {
    GTEST_SKIP();
  }

  // Check whether we have a compiler to compile our kernel.
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }

  const char *kernel_source = R"OPENCL(
      kernel void sub_group_kernel(global int *in, global int *out) {
        uint gid = get_global_id(0);
        out[gid] = sub_group_reduce_add(in[gid]);
        }
      )OPENCL";
  const size_t kernel_source_length = std::strlen(kernel_source);

  cl_int error = CL_SUCCESS;
  auto program = clCreateProgramWithSource(context, 1, &kernel_source,
                                           &kernel_source_length, &error);
  ASSERT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, "-cl-std=CL3.0", nullptr, nullptr));
  auto kernel = clCreateKernel(program, "sub_group_kernel", &error);
  EXPECT_SUCCESS(error);

  size_t nd_range[] = {32, 1, 1};
  size_t sub_group_count;
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE,
                        sizeof(nd_range) / sizeof(nd_range[0]), &nd_range,
                        sizeof(sub_group_count), &sub_group_count, nullptr));

  size_t sub_group_size;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clGetKernelSubGroupInfo(
          kernel, device, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
          sizeof(nd_range) / sizeof(nd_range[0]), &nd_range,
          sizeof(sub_group_size), &sub_group_size, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clGetKernelSubGroupInfo(
          kernel, device, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
          sizeof(sub_group_count), &sub_group_count,
          sizeof(nd_range) / sizeof(nd_range[0]), &nd_range, nullptr));

  size_t max_num_sub_groups;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_MAX_NUM_SUB_GROUPS, 0,
                              nullptr, sizeof(max_num_sub_groups),
                              &max_num_sub_groups, nullptr));

  size_t compile_num_sub_groups;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_COMPILE_NUM_SUB_GROUPS,
                              0, nullptr, sizeof(compile_num_sub_groups),
                              &compile_num_sub_groups, nullptr));

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

class clGetKernelSubGroupInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());

    // This test fixture assumes OpenCL 3.0 or later.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    // Check whether sub-groups are actually supported.
    cl_uint max_num_subgroups = 0;
    EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                   sizeof(cl_uint), &max_num_subgroups,
                                   nullptr));
    if (0 == max_num_subgroups) {
      GTEST_SKIP();
    }

    // Check whether we have a compiler to compile our kernel.
    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    const char *kernel_source = R"OPENCL(
      kernel void sub_group_kernel(global int *in, global int *out) {
        uint gid = get_global_id(0);
        out[gid] = sub_group_reduce_add(in[gid]);
        }
      )OPENCL";
    const size_t kernel_source_length = std::strlen(kernel_source);

    cl_int error = CL_SUCCESS;
    program = clCreateProgramWithSource(context, 1, &kernel_source,
                                        &kernel_source_length, &error);
    ASSERT_SUCCESS(error);
    ASSERT_SUCCESS(
        clBuildProgram(program, 1, &device, "-cl-std=CL3.0", nullptr, nullptr));
    kernel = clCreateKernel(program, "sub_group_kernel", &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ucl::ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clGetKernelSubGroupInfoTest, OmitDeviceParameter) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, nullptr, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
      input_value_size, &input_value, output_value_size, &output_value,
      nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckSizeQuerySucceeds) {
  size_t output_value_size = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
      input_value_size, &input_value, 0, nullptr, &output_value_size));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckSizeQueryIsCorrect) {
  size_t output_value_size = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
      input_value_size, &input_value, 0, nullptr, &output_value_size));
  const size_t correct_output_value_size = sizeof(size_t);
  ASSERT_EQ(output_value_size, correct_output_value_size);
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckQuerySucceeds) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
      input_value_size, &input_value, output_value_size, &output_value,
      nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckIncorrectSizeQueryFails) {
  const size_t output_value_size = sizeof(size_t) - 1;
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelSubGroupInfo(kernel, device,
                              CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                              input_value_size, &input_value, output_value_size,
                              &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckNullInputValue) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelSubGroupInfo(kernel, device,
                              CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                              input_value_size, nullptr, output_value_size,
                              &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxSubGroupSizeForNDRangeCheckInvalidInputValueSize) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 0;
  const size_t input_value = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelSubGroupInfo(kernel, device,
                              CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                              input_value_size, &input_value, output_value_size,
                              &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       SubGroupCountForNDRangeCheckSizeQuerySucceeds) {
  size_t output_value_size = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE, input_value_size,
      &input_value, 0, nullptr, &output_value_size));
}

TEST_F(clGetKernelSubGroupInfoTest,
       SubGroupCountForNDRangeCheckSizeQueryIsCorrect) {
  size_t output_value_size = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE, input_value_size,
      &input_value, 0, nullptr, &output_value_size));
  const size_t correct_output_value_size = sizeof(size_t);
  ASSERT_EQ(output_value_size, correct_output_value_size);
}

TEST_F(clGetKernelSubGroupInfoTest, SubGroupCountForNDRangeCheckQuerySucceeds) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE, input_value_size,
      &input_value, output_value_size, &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       SubGroupCountForNDRangeCheckIncorrectSizeQueryFails) {
  const size_t output_value_size = sizeof(size_t) - 1;
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  const size_t input_value = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE,
                        input_value_size, &input_value, output_value_size,
                        &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       SubGroupCountForNDRangeCheckNullInputValue) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 1 * sizeof(size_t);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE,
                        input_value_size, nullptr, output_value_size,
                        &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       SubGroupCountForNDRangeCheckInvalidInputValueSize) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  const size_t input_value_size = 0;
  const size_t input_value = 1;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE,
                        input_value_size, &input_value, output_value_size,
                        &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       LocalSizeForSubGroupCountCheckSizeQuerySucceeds) {
  size_t output_value_size = 0;
  const size_t input_value_size = sizeof(size_t);
  const size_t input_value = 4;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
      input_value_size, &input_value, 0, nullptr, &output_value_size));
}

TEST_F(clGetKernelSubGroupInfoTest,
       LocalSizeForSubGroupCountCheckQuerySucceeds) {
  size_t output_value_size = 0;
  const size_t input_value_size = sizeof(size_t);
  const size_t input_value = 4;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
      input_value_size, &input_value, 0, nullptr, &output_value_size));

  std::vector<size_t> value(output_value_size);
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
      input_value_size, &input_value, output_value_size, value.data(),
      nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       LocalSizeForSubGroupCountCheckIncorrectSizeQueryFails) {
  size_t output_value_size = 0;
  const size_t input_value_size = sizeof(size_t);
  const size_t input_value = 4;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
      input_value_size, &input_value, 0, nullptr, &output_value_size));

  std::vector<size_t> value(output_value_size);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetKernelSubGroupInfo(kernel, device,
                              CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
                              input_value_size, &input_value,
                              output_value_size + 1, value.data(), nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest, MaxNumSubGroupsCheckSizeQuerySucceeds) {
  size_t output_value_size = 0;
  ASSERT_SUCCESS(
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_MAX_NUM_SUB_GROUPS, 0,
                              nullptr, 0, nullptr, &output_value_size));
}

TEST_F(clGetKernelSubGroupInfoTest, MaxNumSubGroupsCheckSizeQueryIsCorrect) {
  size_t output_value_size = 0;
  ASSERT_SUCCESS(
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_MAX_NUM_SUB_GROUPS, 0,
                              nullptr, 0, nullptr, &output_value_size));
  const size_t correct_output_value_size = sizeof(size_t);
  ASSERT_EQ(output_value_size, correct_output_value_size);
}

TEST_F(clGetKernelSubGroupInfoTest, MaxNumSubGroupsCheckQuerySucceeds) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_MAX_NUM_SUB_GROUPS, 0, nullptr,
      output_value_size, &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       MaxNumSubGroupsCheckIncorrectSizeQueryFails) {
  const size_t output_value_size = sizeof(size_t) - 1;
  size_t output_value = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_MAX_NUM_SUB_GROUPS, 0,
                        nullptr, output_value_size, &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest, CompileNumSubGroupsCheckSizeQuerySucceeds) {
  size_t output_value_size = 0;
  ASSERT_SUCCESS(
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_COMPILE_NUM_SUB_GROUPS,
                              0, nullptr, 0, nullptr, &output_value_size));
}

TEST_F(clGetKernelSubGroupInfoTest,
       CompileNumSubGroupsCheckSizeQueryIsCorrect) {
  size_t output_value_size = 0;
  ASSERT_SUCCESS(
      clGetKernelSubGroupInfo(kernel, device, CL_KERNEL_COMPILE_NUM_SUB_GROUPS,
                              0, nullptr, 0, nullptr, &output_value_size));
  const size_t correct_output_value_size = sizeof(size_t);
  ASSERT_EQ(output_value_size, correct_output_value_size);
}

TEST_F(clGetKernelSubGroupInfoTest, CompileNumSubGroupsCheckQuerySucceeds) {
  const size_t output_value_size = sizeof(size_t);
  size_t output_value = 0;
  ASSERT_SUCCESS(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_COMPILE_NUM_SUB_GROUPS, 0, nullptr,
      output_value_size, &output_value, nullptr));
}

TEST_F(clGetKernelSubGroupInfoTest,
       CompileNumSubGroupsCheckIncorrectSizeQueryFails) {
  const size_t output_value_size = sizeof(size_t) - 1;
  size_t output_value = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelSubGroupInfo(
                        kernel, device, CL_KERNEL_COMPILE_NUM_SUB_GROUPS, 0,
                        nullptr, output_value_size, &output_value, nullptr));
}
