// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>
#include <gtest/internal/gtest-internal.h>

#include <algorithm>

#include "common.h"
#include "mux/mux.h"

struct muxQueryLocalSizeForSubGroupCountBaseTest : DeviceCompilerTest {
  mux_executable_t executable = nullptr;
  mux_kernel_t kernel = nullptr;
  size_t sub_group_count = 4;
  size_t local_size_x = 0, local_size_y = 0, local_size_z = 0;

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
INSTANTIATE_DEVICE_TEST_SUITE_P(muxQueryLocalSizeForSubGroupCountBaseTest);

TEST_P(muxQueryLocalSizeForSubGroupCountBaseTest, Unsupported) {
  if (device->info->max_sub_group_count) {
    GTEST_SKIP();
  }
  ASSERT_ERROR_EQ(
      mux_error_feature_unsupported,
      muxQueryLocalSizeForSubGroupCount(kernel, sub_group_count, &local_size_x,
                                        &local_size_y, &local_size_z));
}

struct muxQueryLocalSizeForSubGroupCountTest
    : muxQueryLocalSizeForSubGroupCountBaseTest {
  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(muxQueryLocalSizeForSubGroupCountBaseTest::SetUp());
    if (!device->info->max_sub_group_count) {
      GTEST_SKIP();
    }
  }
};
INSTANTIATE_DEVICE_TEST_SUITE_P(muxQueryLocalSizeForSubGroupCountTest);

TEST_P(muxQueryLocalSizeForSubGroupCountTest, InvalidKernel) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxQueryLocalSizeForSubGroupCount(nullptr, sub_group_count, &local_size_x,
                                        &local_size_y, &local_size_z));
}

TEST_P(muxQueryLocalSizeForSubGroupCountTest, InvalidLocalSizeX) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxQueryLocalSizeForSubGroupCount(kernel, sub_group_count, nullptr,
                                        &local_size_y, &local_size_z));
}

TEST_P(muxQueryLocalSizeForSubGroupCountTest, InvalidLocalSizeY) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxQueryLocalSizeForSubGroupCount(kernel, sub_group_count, &local_size_x,
                                        nullptr, &local_size_z));
}

TEST_P(muxQueryLocalSizeForSubGroupCountTest, InvalidLocalSizeZ) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxQueryLocalSizeForSubGroupCount(kernel, sub_group_count, &local_size_x,
                                        &local_size_y, nullptr));
}

TEST_P(muxQueryLocalSizeForSubGroupCountTest, ValidateLocalSize) {
  ASSERT_SUCCESS(muxQueryLocalSizeForSubGroupCount(
      kernel, sub_group_count, &local_size_x, &local_size_y, &local_size_z));

  std::array<size_t, 3> local_sizes{local_size_x, local_size_y, local_size_z};

  const auto any_local_sizes_zero =
      std::any_of(std::begin(local_sizes), std::end(local_sizes),
                  [](size_t local_size) { return local_size == 0; });

  // If the local size is zero it indicates no local size would result in the
  // specified number of sub-groups.
  if (any_local_sizes_zero) {
    return;
  }

  // The local size must be 1D i.e. at least two of the local dimensions must
  // be 1.
  const auto one_dimensional_counts =
      std::count(std::begin(local_sizes), std::end(local_sizes), 1);
  ASSERT_EQ(one_dimensional_counts, 2);

  // The local size must be evenly divisible by the sub-group size with no
  // remainders.
  size_t sub_group_size;
  ASSERT_SUCCESS(muxQuerySubGroupSizeForLocalSize(
      kernel, local_size_x, local_size_y, local_size_z, &sub_group_size));
  ASSERT_EQ((local_size_x * local_size_y * local_size_z) % sub_group_size, 0);
}
