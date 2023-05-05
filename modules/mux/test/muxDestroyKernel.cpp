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

#include "common.h"

struct muxDestroyKernelTest : DeviceCompilerTest {
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

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyKernelTest);

TEST_P(muxDestroyKernelTest, Default) {
  mux_kernel_t kernel;

  ASSERT_SUCCESS(muxCreateKernel(device, executable, "parallel_copy",
                                 strlen("parallel_copy"), allocator, &kernel));

  muxDestroyKernel(device, kernel, allocator);
}
