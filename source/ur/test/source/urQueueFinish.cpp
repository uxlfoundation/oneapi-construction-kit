// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <thread>

#include "uur/fixtures.h"

using urQueueFinishTest = uur::KernelTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urQueueFinishTest);

TEST_P(urQueueFinishTest, Success) {
  // Not strictly necessary for the simplest case but we might as well check
  // this isn't going to blow up with a wee bit of work in the pipes.
  const size_t nDimensions = 1, globalWorkOffset = 0, globalWorkSize = 1,
               localWorkSize = 1;
  ASSERT_SUCCESS(urEnqueueKernelLaunch(queue, kernel, nDimensions,
                                       &globalWorkOffset, &globalWorkSize,
                                       &localWorkSize, 0, nullptr, nullptr));
  EXPECT_SUCCESS(urQueueFinish(queue));
}

TEST_P(urQueueFinishTest, ConcurrentFinishes) {
  // This is stolen from the UnitCL test of the same name. That test was written
  // to detect a specific bug but it also seems like a good stress test for the
  // system in general.
  auto worker = [this]() {
    for (int i = 0; i < 1; i++) {
      const size_t nDimensions = 1, globalWorkOffset = 0, globalWorkSize = 1,
                   localWorkSize = 1;
      ASSERT_SUCCESS(urEnqueueKernelLaunch(
          queue, kernel, nDimensions, &globalWorkOffset, &globalWorkSize,
          &localWorkSize, 0, nullptr, nullptr));

      ASSERT_SUCCESS(urQueueFinish(queue));
    }
  };

  const size_t threads = 1;
  std::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}

TEST_P(urQueueFinishTest, InvalidNullHandleQueue) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE, urQueueFinish(nullptr));
}
