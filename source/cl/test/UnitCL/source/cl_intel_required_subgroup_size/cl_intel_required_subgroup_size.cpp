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

#include <CL/cl_ext_codeplay.h>

#include "Common.h"
#include "kts/sub_group_helpers.h"

using namespace kts::ucl;

struct cl_intel_required_subgroup_size_Test : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_intel_required_subgroup_size")) {
      GTEST_SKIP();
    }
  }

  void TearDown() override { ContextTest::TearDown(); }
};

// Helper method to return the sub-group sizes reported by the device.
static std::vector<size_t> getSubgroupSizes(cl_device_id device) {
  size_t sub_groups_num_bytes = 0;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL, 0,
                                 nullptr, &sub_groups_num_bytes));

  const size_t num_sub_groups = sub_groups_num_bytes / sizeof(size_t);
  std::vector<size_t> sub_groups(num_sub_groups);
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL,
                                 sub_groups_num_bytes, sub_groups.data(),
                                 nullptr));
  return sub_groups;
}

TEST_F(cl_intel_required_subgroup_size_Test, DeviceInfo) {
  getSubgroupSizes(device);
}

TEST_F(cl_intel_required_subgroup_size_Test, DeviceInfoBadParamValue) {
  size_t sub_groups_num_bytes = 0;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL, 0,
                                 nullptr, &sub_groups_num_bytes));

  const size_t num_sub_groups = sub_groups_num_bytes / sizeof(size_t);
  std::vector<size_t> sub_groups(num_sub_groups);

  // It's valid to pass in *more* bytes than required.
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL,
                                 sub_groups_num_bytes + sizeof(size_t),
                                 sub_groups.data(), nullptr));

  if (num_sub_groups != 0) {
    // If we don't pass in a number of bytes large enough to cover all of the
    // sub-group sizes reported by the device, the API should return
    // CL_INVALID_VALUE.
    EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                      clGetDeviceInfo(device, CL_DEVICE_SUB_GROUP_SIZES_INTEL,
                                      0, sub_groups.data(), nullptr));
  }
}

struct cl_intel_required_subgroup_size_KernelTest : public kts::ucl::Execution {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    if (!isDeviceExtensionSupported("cl_intel_required_subgroup_size")) {
      GTEST_SKIP();
    }

    // Subgroups are a 3.0 feature.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    // Some of these tests run small local sizes, which we don't vectorize.
    // This is too coarse-grained, as there are some NDRanges which we can
    // vectorize.
    fail_if_not_vectorized_ = false;

    // clGetDeviceInfo may return 0, indicating that the device does not support
    // subgroups.
    cl_uint max_num_subgroups;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                   sizeof(cl_uint), &max_num_subgroups,
                                   nullptr));
    if (0 == max_num_subgroups) {
      GTEST_SKIP() << "Device does not support sub-groups, skipping test.\n";
    }

    AddBuildOption("-cl-std=CL3.0");

    // These kernels may not be supported, so enable soft fail mode.
    fail_if_build_program_failed = false;
  }
};

TEST_P(cl_intel_required_subgroup_size_KernelTest, Ext_Reqd_Subgroup_01_Size8) {
  constexpr size_t Size = 8;
  auto sub_groups = getSubgroupSizes(device);
  if (std::find(sub_groups.begin(), sub_groups.end(), Size) ==
      sub_groups.end()) {
    printf(
        "Required sub-group size of %zu not supported on this device, skipping "
        "test.\n",
        Size);
    GTEST_SKIP();
  }

  // We need to force a compilation to ensure we have a kernel to query
  // kernel information from. This may fail, in which case we must skip the
  // test.
  if (!BuildProgram()) {
    printf("Could not build the program, skipping test.\n");
    GTEST_SKIP();
  }

  // Check that CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL returns the sub-group
  // size we encoded at compile time.
  size_t val_size;
  size_t sub_group_size;
  EXPECT_SUCCESS(clGetKernelSubGroupInfo(
      kernel_, device, CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL,
      sizeof(sub_group_size), nullptr, sizeof(sub_group_size), &sub_group_size,
      &val_size));
  EXPECT_EQ(val_size, sizeof(sub_group_size));
  EXPECT_EQ(sub_group_size, Size);

  // Check CL_KERNEL_SPILL_MEM_SIZE_INTEL doesn't error and returns something
  // sensible.
  cl_ulong mem_size;
  EXPECT_SUCCESS(
      clGetKernelWorkGroupInfo(kernel_, device, CL_KERNEL_SPILL_MEM_SIZE_INTEL,
                               sizeof(mem_size), &mem_size, &val_size));
  // Without any constraints on the what the spill size means (and none
  // supported by the oneAPI Construction Kit) it's hard to test any more than
  // this.
  EXPECT_EQ(val_size, sizeof(mem_size));
}

static const kts::ucl::SourceType source_types[] = {
    kts::ucl::OPENCL_C, kts::ucl::OFFLINE, kts::ucl::SPIRV,
    kts::ucl::OFFLINESPIRV};

UCL_EXECUTION_TEST_SUITE(cl_intel_required_subgroup_size_KernelTest,
                         testing::ValuesIn(source_types))
