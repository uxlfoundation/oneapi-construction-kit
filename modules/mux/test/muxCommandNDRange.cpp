// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxCommandNDRangeTest : DeviceCompilerTest {
  mux_command_buffer_t command_buffer = nullptr;
  mux_executable_t executable = nullptr;
  mux_kernel_t kernel = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());

    // Kernel source.
    const char *parallel_copy_opencl_c = "void kernel nop() {}";

    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(createMuxExecutable(parallel_copy_opencl_c, &executable));
    EXPECT_SUCCESS(muxCreateKernel(device, executable, "nop", strlen("nop"),
                                   allocator, &kernel));
  }

  void TearDown() override {
    if (nullptr != kernel) {
      muxDestroyKernel(device, kernel, allocator);
    }

    if (nullptr != executable) {
      muxDestroyExecutable(device, executable, allocator);
    }

    if (nullptr != command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }

    DeviceCompilerTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandNDRangeTest);

TEST_P(muxCommandNDRangeTest, Default) {
  size_t global_offset[3] = {1, 1, 1};
  size_t global_size[3] = {1, 1, 1};
  size_t local_size[3] = {1, 1, 1};
  size_t dimensions = 3;

  mux_ndrange_options_t nd_range_options{};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  nd_range_options.global_offset = &global_offset[0];
  nd_range_options.global_size = &global_size[0];
  nd_range_options.dimensions = dimensions;

  ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                                   nullptr, nullptr));
}

TEST_P(muxCommandNDRangeTest, InvalidKernel) {
  size_t global_offset[3] = {1, 1, 1};
  size_t global_size[3] = {1, 1, 1};
  size_t local_size[3] = {1, 1, 1};
  size_t dimensions = 3;

  mux_ndrange_options_t nd_range_options{};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  nd_range_options.global_offset = &global_offset[0];
  nd_range_options.global_size = &global_size[0];
  nd_range_options.dimensions = dimensions;

  mux_kernel_s invalid_kernel{};

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandNDRange(command_buffer, &invalid_kernel,
                                    nd_range_options, 0, nullptr, nullptr));
}

TEST_P(muxCommandNDRangeTest, InvalidWaitList) {
  size_t global_offset[3] = {1, 1, 1};
  size_t global_size[3] = {1, 1, 1};
  size_t local_size[3] = {1, 1, 1};
  size_t dimensions = 3;

  mux_ndrange_options_t nd_range_options{};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  nd_range_options.global_offset = &global_offset[0];
  nd_range_options.global_size = &global_size[0];
  nd_range_options.dimensions = dimensions;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandNDRange(command_buffer, kernel, nd_range_options, 1,
                                    nullptr, nullptr));

  mux_sync_point_t wait;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                                    &wait, nullptr));

  wait = reinterpret_cast<mux_sync_point_t>(kernel);
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandNDRange(command_buffer, kernel, nd_range_options, 1,
                                    &wait, nullptr));
}

TEST_P(muxCommandNDRangeTest, Sync) {
  size_t global_offset[3] = {1, 1, 1};
  size_t global_size[3] = {1, 1, 1};
  size_t local_size[3] = {1, 1, 1};
  size_t dimensions = 3;

  mux_ndrange_options_t nd_range_options{};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  nd_range_options.global_offset = &global_offset[0];
  nd_range_options.global_size = &global_size[0];
  nd_range_options.dimensions = dimensions;

  mux_sync_point_t wait = nullptr;
  ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                                   nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options, 1,
                                   &wait, nullptr));
}
