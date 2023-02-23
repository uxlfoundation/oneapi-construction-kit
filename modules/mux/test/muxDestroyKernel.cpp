// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
