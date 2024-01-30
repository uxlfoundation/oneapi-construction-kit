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

#include <cargo/string_view.h>

#include "common.h"

struct muxCreateKernelTest : DeviceCompilerTest {
  mux_executable_t executable = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());

    const char *parallel_copy_opencl_c = R"(
      void kernel parallel_copy(global int* a, global int* b) {
        const size_t gid = get_global_id(0);
        a[gid] = b[gid];
    })";

    ASSERT_SUCCESS(createMuxExecutable(parallel_copy_opencl_c, &executable));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyExecutable(device, executable, allocator);
    }
    DeviceCompilerTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateKernelTest);

TEST_P(muxCreateKernelTest, Default) {
  mux_kernel_t kernel;

  ASSERT_SUCCESS(muxCreateKernel(device, executable, "parallel_copy",
                                 strlen("parallel_copy"), allocator, &kernel));

  ASSERT_GE(kernel->preferred_local_size_x, 1u);
  ASSERT_GE(kernel->preferred_local_size_y, 1u);
  ASSERT_GE(kernel->preferred_local_size_z, 1u);

  ASSERT_LE(kernel->preferred_local_size_x,
            device->info->max_work_group_size_x);
  ASSERT_LE(kernel->preferred_local_size_y,
            device->info->max_work_group_size_y);
  ASSERT_LE(kernel->preferred_local_size_z,
            device->info->max_work_group_size_z);

  muxDestroyKernel(device, kernel, allocator);
}

TEST_P(muxCreateKernelTest, NullName) {
  mux_kernel_t kernel;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateKernel(device, executable, 0,
                                  strlen("parallel_copy"), allocator, &kernel));
}

TEST_P(muxCreateKernelTest, InvalidName) {
  // TODO(CA-3368): Don't skip this test once the riscv target passes it.
  const cargo::string_view device_name{device->info->device_name};
  if (device_name.find("RISC-V") != cargo::string_view::npos) {
    GTEST_SKIP();
  }
  mux_kernel_t kernel;

  ASSERT_ERROR_EQ(mux_error_missing_kernel,
                  muxCreateKernel(device, executable, "some_bad_name",
                                  strlen("some_bad_name"), allocator, &kernel));
}

TEST_P(muxCreateKernelTest, InvalidNameLength) {
  mux_kernel_t kernel;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateKernel(device, executable, "parallel_copy", 0,
                                  allocator, &kernel));
}

TEST_P(muxCreateKernelTest, InvalidOutKernel) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateKernel(device, executable, "parallel_copy",
                                  strlen("parallel_copy"), allocator, 0));
}
