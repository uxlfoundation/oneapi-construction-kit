// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <gtest/gtest.h>

#include "common.h"
#include "mux/mux.h"

struct muxQuerySubGroupSizeForLocalSizeBaseTest : DeviceCompilerTest {
  mux_executable_t executable = nullptr;
  mux_kernel_t kernel = nullptr;
  size_t sub_group_size, local_size_x, local_size_y, local_size_z;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    ASSERT_SUCCESS(
        createMuxExecutable("void kernel sub_group_kernel() {}", &executable));
    EXPECT_SUCCESS(muxCreateKernel(device, executable, "sub_group_kernel",
                                   strlen("sub_group_kernel"), allocator,
                                   &kernel));
  }

  void TearDown() override {
    if (kernel) {
      muxDestroyKernel(device, kernel, allocator);
    }

    if (executable) {
      muxDestroyExecutable(device, executable, allocator);
    }

    DeviceCompilerTest::TearDown();
  }
};
INSTANTIATE_DEVICE_TEST_SUITE_P(muxQuerySubGroupSizeForLocalSizeBaseTest);

TEST_P(muxQuerySubGroupSizeForLocalSizeBaseTest, Unsupported) {
  if (device->info->max_sub_group_count) {
    GTEST_SKIP();
  }
  local_size_x = 4;
  local_size_y = 1;
  local_size_z = 1;

  ASSERT_ERROR_EQ(
      mux_error_feature_unsupported,
      muxQuerySubGroupSizeForLocalSize(kernel, local_size_x, local_size_y,
                                       local_size_z, &sub_group_size));
}

struct muxQuerySubGroupSizeForLocalSizeTest
    : muxQuerySubGroupSizeForLocalSizeBaseTest {
  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(muxQuerySubGroupSizeForLocalSizeBaseTest::SetUp());
    if (!device->info->max_sub_group_count) {
      GTEST_SKIP();
    }
  }
};
INSTANTIATE_DEVICE_TEST_SUITE_P(muxQuerySubGroupSizeForLocalSizeTest);

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, InvalidKernel) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxQuerySubGroupSizeForLocalSize(nullptr, local_size_x, local_size_y,
                                       local_size_z, &sub_group_size));
}

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeX) {
  local_size_x = 0;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxQuerySubGroupSizeForLocalSize(kernel, local_size_x, local_size_y,
                                       local_size_z, &sub_group_size));
}

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeY) {
  local_size_y = 0;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxQuerySubGroupSizeForLocalSize(kernel, local_size_x, local_size_y,
                                       local_size_z, &sub_group_size));
}

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, InvalidLocalSizeZ) {
  local_size_z = 0;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxQuerySubGroupSizeForLocalSize(kernel, local_size_x, local_size_y,
                                       local_size_z, &sub_group_size));
}

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, InvalidSubGroupSize) {
  local_size_x = 4;
  local_size_y = 1;
  local_size_z = 1;
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxQuerySubGroupSizeForLocalSize(kernel, local_size_x, local_size_y,
                                       local_size_z, nullptr));
}

TEST_P(muxQuerySubGroupSizeForLocalSizeTest, ValidateSubGroupSize) {
  local_size_x = 32;
  local_size_y = 1;
  local_size_z = 1;
  ASSERT_SUCCESS(muxQuerySubGroupSizeForLocalSize(
      kernel, local_size_x, local_size_y, local_size_z, &sub_group_size));
  ASSERT_GE(sub_group_size, 1);
}
