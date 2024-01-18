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
#include <gtest/gtest.h>

#include <algorithm>

#include "common.h"

/// @file This file contains all tests for the compiler::Kernel object.

/// @brief Test fixture for testing behaviour of the
/// compiler::Module::getKernelTest API.
struct GetKernelTest : OpenCLCModuleTest {};

TEST_P(GetKernelTest, GetKernel) {
  auto *kernel = module->getKernel("nop");
  if (optional_device) {
    ASSERT_NE(nullptr, kernel);
  } else {
    ASSERT_EQ(nullptr, kernel);
  }
}

TEST_P(GetKernelTest, PreferredLocalSize) {
  if (!optional_device) {
    GTEST_SKIP();
  }

  auto *kernel = module->getKernel("nop");
  ASSERT_NE(nullptr, kernel);

  EXPECT_GE(kernel->preferred_local_size_x, 1u);
  EXPECT_GE(kernel->preferred_local_size_y, 1u);
  EXPECT_GE(kernel->preferred_local_size_z, 1u);

  auto device = *optional_device;
  EXPECT_LE(kernel->preferred_local_size_x,
            device->info->max_work_group_size_x);
  EXPECT_LE(kernel->preferred_local_size_y,
            device->info->max_work_group_size_y);
  EXPECT_LE(kernel->preferred_local_size_z,
            device->info->max_work_group_size_z);
}

TEST_P(GetKernelTest, InvalidName) {
  auto *kernel = module->getKernel("some_bad_name");
  ASSERT_EQ(nullptr, kernel);
}

INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(GetKernelTest);

/// @brief Test fixture for testing behvaiour of the
/// compiler::Kernel::precacheLocalSize API.
struct PrecacheLocalSizeTest : CompilerKernelTest {};

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSize) {
  ASSERT_EQ(compiler::Result::SUCCESS, kernel->precacheLocalSize(1, 1, 1));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeMaxX) {
  ASSERT_EQ(
      compiler::Result::SUCCESS,
      kernel->precacheLocalSize(device->info->max_work_group_size_x, 1, 1));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeMaxY) {
  ASSERT_EQ(
      compiler::Result::SUCCESS,
      kernel->precacheLocalSize(1, device->info->max_work_group_size_y, 1));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeMaxZ) {
  ASSERT_EQ(
      compiler::Result::SUCCESS,
      kernel->precacheLocalSize(1, 1, device->info->max_work_group_size_z));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeInvalidX) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->precacheLocalSize(0, 1, 1));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeInvalidY) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->precacheLocalSize(1, 0, 1));
}

TEST_P(PrecacheLocalSizeTest, PrecacheLocalSizeInvalidZ) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->precacheLocalSize(1, 1, 0));
}

INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(PrecacheLocalSizeTest);

/// @brief Test fixture for testing behvaiour of the
/// compiler::Kernel::getDynamicWorkWidth API.
struct GetDynamicWorkWidthTest : CompilerKernelTest {};

TEST_P(GetDynamicWorkWidthTest, GetDynamicWorkWidth) {
  auto dynamic_work_width = kernel->getDynamicWorkWidth(1, 1, 1);
  ASSERT_TRUE(dynamic_work_width);
  ASSERT_EQ(1, *dynamic_work_width);
}

TEST_P(GetDynamicWorkWidthTest, GetDynamicWorkWidthMaxX) {
  auto dynamic_work_width =
      kernel->getDynamicWorkWidth(device->info->max_work_group_size_x, 1, 1);
  ASSERT_TRUE(dynamic_work_width);
  EXPECT_GE(*dynamic_work_width, 1u);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_width);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_group_size_x);
}

TEST_P(GetDynamicWorkWidthTest, GetDynamicWorkWidthMaxY) {
  auto dynamic_work_width =
      kernel->getDynamicWorkWidth(1, device->info->max_work_group_size_y, 1);
  ASSERT_TRUE(dynamic_work_width);
  EXPECT_GE(*dynamic_work_width, 1u);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_width);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_group_size_y);
}

TEST_P(GetDynamicWorkWidthTest, GetDynamicWorkWidthMaxZ) {
  auto dynamic_work_width =
      kernel->getDynamicWorkWidth(1, 1, device->info->max_work_group_size_z);
  ASSERT_TRUE(dynamic_work_width);
  EXPECT_GE(*dynamic_work_width, 1u);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_width);
  EXPECT_LE(*dynamic_work_width, device->info->max_work_group_size_z);
}

INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(GetDynamicWorkWidthTest);

/// @brief Test fixture for testing behaviour of the
/// compiler::Kernel::createSpecializedKernel API.
struct CreateSpecializedKernelTest : CompilerKernelTest {};

TEST_P(CreateSpecializedKernelTest, CreateSpecializedKernel) {
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_TRUE(specialized_kernel);
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidDescriptorsNull) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_ndrange_options_t::descriptors is
  // null and mux_ndrange_options_t::descriptors_length is non-zero.
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 1;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel.has_value());
  ASSERT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidDescriptorsLength) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_ndrange_options_t::descriptors is
  // non-null and mux_ndrange_options_t::descriptors_length is zero.
  mux_ndrange_options_t nd_range_options{};
  mux_descriptor_info_t descriptors;
  nd_range_options.descriptors = &descriptors;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel.has_value());
  ASSERT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidLocalSize) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if any of the elements in
  // mux_nd_range_options_t::local_size is zero.
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  // Generate all permutations where at least one local_size element is 0.
  size_t local_size[]{0, 0, 0};
  for (unsigned i = 0; i < 2; ++i) {
    for (unsigned j = 0; j < 2; ++j) {
      for (unsigned k = 0; k < 2; ++k) {
        // Skip the case [1, 1, 1] since this is valid.
        if (i == 1 && j == 1 && k == 1) {
          break;
        }
        std::memcpy(nd_range_options.local_size, local_size,
                    sizeof(local_size));
        auto specialized_kernel =
            kernel->createSpecializedKernel(nd_range_options);
        ASSERT_FALSE(specialized_kernel.has_value());
        EXPECT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
        local_size[2] = !local_size[2];
      }
      local_size[1] = !local_size[1];
    }
    local_size[0] = !local_size[0];
  }
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidGlobalOffsetNull) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_nd_range_options_t::global_offset is
  // null.
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  nd_range_options.global_offset = nullptr;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel.has_value());
  ASSERT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidGlobalSizeNull) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_nd_range_options_t::global_size is
  // null.
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  nd_range_options.global_size = nullptr;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel.has_value());
  ASSERT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
}

TEST_P(CreateSpecializedKernelTest, NDRangeOptionsInvalidGlobalDimensions) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_nd_range_options_t::dimensions is
  // 0 or greater than 3.
  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 0;

  auto specialized_kernel_zero_dim =
      kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel_zero_dim.has_value());
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            specialized_kernel_zero_dim.error());

  nd_range_options.dimensions = 4;

  auto specialized_kernel_four_dim =
      kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel_four_dim.has_value());
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            specialized_kernel_four_dim.error());
}

TEST_P(CreateSpecializedKernelTest,
       NDRangeOptionsInvalidDescriptorCustomBuffer) {
  // compiler::kernel::createSpecializedKernel must return
  // compiler::Result::INVALID_VALUE if mux_nd_range_options_t::descriptors
  // contains an element where the mux_descriptor_info_type_e::type field is
  // mux_descriptor_info_type_e::mux_descriptor_info_type_custom_buffer and the
  // device being targeted has
  // mux_device_t::device_info_t::custom_buffer_capabilities is 0.

  // We can only run this test for devices not supporting custom buffer
  // capabilities.
  if (device->info->custom_buffer_capabilities) {
    GTEST_SKIP();
  }
  mux_ndrange_options_t nd_range_options{};
  mux_descriptor_info_t descriptor;
  descriptor.type =
      mux_descriptor_info_type_e::mux_descriptor_info_type_custom_buffer;
  nd_range_options.descriptors = &descriptor;
  nd_range_options.descriptors_length = 1;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 0;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  auto specialized_kernel = kernel->createSpecializedKernel(nd_range_options);
  ASSERT_FALSE(specialized_kernel.has_value());
  EXPECT_EQ(compiler::Result::INVALID_VALUE, specialized_kernel.error());
}

INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(
    CreateSpecializedKernelTest);

struct SubGroupUnsupportedTest : CompilerKernelTest {
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerKernelTest::SetUp());
    if (device->info->max_sub_group_count) {
      GTEST_SKIP();
    }
  }
};
INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(SubGroupUnsupportedTest);

TEST_P(SubGroupUnsupportedTest, QuerySubGroupSizeForLocalSize) {
  ASSERT_EQ(compiler::Result::FEATURE_UNSUPPORTED,
            kernel->querySubGroupSizeForLocalSize(4, 1, 1).error());
}

TEST_P(SubGroupUnsupportedTest, QueryLocalSizeForSubGroupCount) {
  ASSERT_EQ(compiler::Result::FEATURE_UNSUPPORTED,
            kernel->queryLocalSizeForSubGroupCount(4).error());
}

TEST_P(SubGroupUnsupportedTest, QueryMaxSubGroupCount) {
  ASSERT_EQ(compiler::Result::FEATURE_UNSUPPORTED,
            kernel->queryMaxSubGroupCount().error());
}

struct SubGroupSupportedTest : CompilerKernelTest {
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerKernelTest::SetUp());
    if (!device->info->max_sub_group_count) {
      GTEST_SKIP();
    }
  }
};

using QuerySubGroupSizeForLocalSizeTest = SubGroupUnsupportedTest;
INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(
    QuerySubGroupSizeForLocalSizeTest);

TEST_P(QuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeX) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->querySubGroupSizeForLocalSize(0, 1, 1).error());
}

TEST_P(QuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeY) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->querySubGroupSizeForLocalSize(1, 0, 1).error());
}

TEST_P(QuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeZ) {
  ASSERT_EQ(compiler::Result::INVALID_VALUE,
            kernel->querySubGroupSizeForLocalSize(1, 1, 0).error());
}

TEST_P(QuerySubGroupSizeForLocalSizeTest, ValidateSubGroupSize) {
  ASSERT_GT(kernel->querySubGroupSizeForLocalSize(4, 1, 1).value(),
            (uint32_t)0);
}

using QueryLocalSizeForSubGroupCount = SubGroupUnsupportedTest;
INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(
    QueryLocalSizeForSubGroupCount);

TEST_P(QueryLocalSizeForSubGroupCount, ValidateLocalSize) {
  auto local_size = kernel->queryLocalSizeForSubGroupCount(4).value();
  // The returned local size must be 1 dimensional, i.e. two of the dimensions
  // must be 1.
  const auto one_dimensional_counts =
      std::count(std::begin(local_size), std::end(local_size), 1);
  ASSERT_EQ(one_dimensional_counts, 2);
  // The returned local size must be evenely divisible by the sub-group size
  // that would result from enqueuing the kernel with the given size i.e. no
  // remainder sub-groups.
  auto sub_group_size = kernel
                            ->querySubGroupSizeForLocalSize(
                                local_size[0], local_size[1], local_size[2])
                            .value();
  ASSERT_EQ((local_size[0] * local_size[1] * local_size[2]) % sub_group_size,
            (uint32_t)0);
}

using QueryMaxSubGroupCount = SubGroupUnsupportedTest;
INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(QueryMaxSubGroupCount);

TEST_P(QueryMaxSubGroupCount, ValidateSubGroupCount) {
  ASSERT_GT(kernel->queryMaxSubGroupCount().value(), (size_t)0);
}
