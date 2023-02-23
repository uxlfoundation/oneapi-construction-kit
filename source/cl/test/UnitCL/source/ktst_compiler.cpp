// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"
#include "kts/execution.h"

using namespace kts::ucl;

TEST_P(Execution, Attribute_01_reqd_work_group_size) {
  if (!this->BuildProgram()) {
    GTEST_SKIP();
  }

  cl_int error;
  constexpr size_t buffer_size = sizeof(cl_ulong) * 3;
  cl_mem buffer = clCreateBuffer(this->context, CL_MEM_WRITE_ONLY, buffer_size,
                                 nullptr, &error);
  ASSERT_SUCCESS(error);
  EXPECT_EQ_ERRCODE(CL_SUCCESS,
                    clSetKernelArg(this->kernel_, 0, sizeof(buffer), &buffer));

  constexpr size_t global_work_size[3] = {16, 8, 4};
  const auto max_work_items_sizes = this->getDeviceMaxWorkItemSizes();
  for (size_t i = 0; i < 3; i++) {
    if (global_work_size[i] > max_work_items_sizes[i]) {
      // Skip test as the max work group size requested is too small for this
      // device
      printf(
          "Work item size of %u not supported on this device for rank %zu "
          "(%u is max allowed), skipping test.\n",
          static_cast<unsigned int>(global_work_size[i]), i,
          static_cast<unsigned int>(max_work_items_sizes[i]));
      GTEST_SKIP();
    }
  }

  constexpr size_t work_group_size =
      global_work_size[0] * global_work_size[1] * global_work_size[2];
  const auto max_work_group_size = this->getDeviceMaxWorkGroupSize();
  if (work_group_size > max_work_group_size) {
    // Skip test as the max work group size requested is too small for this
    // device
    printf(
        "Work group size of %u not supported on this device (%u is max "
        "allowed), skipping test.\n",
        static_cast<unsigned int>(work_group_size),
        static_cast<unsigned int>(max_work_group_size));
    GTEST_SKIP();
  }

  cl_event ndRangeEvent = nullptr;
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS, clEnqueueNDRangeKernel(this->command_queue, this->kernel_, 3,
                                         nullptr, global_work_size, nullptr, 0,
                                         nullptr, &ndRangeEvent));

  cl_ulong reqd_work_group_size[3] = {};
  EXPECT_EQ_ERRCODE(
      CL_SUCCESS,
      clEnqueueReadBuffer(this->command_queue, buffer, CL_TRUE, 0, buffer_size,
                          reqd_work_group_size, 1, &ndRangeEvent, nullptr));

  EXPECT_EQ(16u, reqd_work_group_size[0]);
  EXPECT_EQ(8u, reqd_work_group_size[1]);
  EXPECT_EQ(4u, reqd_work_group_size[2]);

  EXPECT_SUCCESS(clReleaseEvent(ndRangeEvent));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_P(Execution, Goto_01_Noop) {
  kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Goto_02_Fake_If) {
  kts::Reference1D<cl_int> refOut = [](size_t x) { return x % 2; };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Goto_03_Fake_For) {
  kts::Reference1D<cl_int> refOut = [](size_t) { return 5; };

  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(static_cast<cl_int>(5));
  RunGeneric1D(kts::N);
}
