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

#include <thread>

#include "uur/fixtures.h"

using urQueueFlushTest = uur::KernelTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urQueueFlushTest);

TEST_P(urQueueFlushTest, Success) {
  // Not strictly necessary for the simplest case but we might as well check
  // this isn't going to blow up with a wee bit of work in the pipes.
  const size_t nDimensions = 1, globalWorkOffset = 0, globalWorkSize = 1,
               localWorkSize = 1;
  ur_event_handle_t event;
  ASSERT_SUCCESS(urEnqueueKernelLaunch(queue, kernel, nDimensions,
                                       &globalWorkOffset, &globalWorkSize,
                                       &localWorkSize, 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  // Can't let the teardown happen until our queue is clear.
  EXPECT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));
}

TEST_P(urQueueFlushTest, ConcurrentFlushes) {
  // This is stolen from the UnitCL test of the same name. That test was written
  // to detect a specific bug but it also seems like a good stress test for the
  // system in general.
  auto worker = [this]() {
    for (int i = 0; i < 32; i++) {
      const size_t nDimensions = 1, globalWorkOffset = 0, globalWorkSize = 1,
                   localWorkSize = 1;
      ASSERT_SUCCESS(urEnqueueKernelLaunch(
          queue, kernel, nDimensions, &globalWorkOffset, &globalWorkSize,
          &localWorkSize, 0, nullptr, nullptr));

      ASSERT_SUCCESS(urQueueFlush(queue));
    }
  };

  const size_t threads = 4;
  std::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  // This finish is here to operate concurrently with the worker thread flush
  // operations, just to increase stress on the system a little.
  ASSERT_SUCCESS(urQueueFinish(queue));

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // This finish is here to ensure all work is complete before we start
  // releasing the kernel etc.
  ASSERT_SUCCESS(urQueueFinish(queue));
}

TEST_P(urQueueFlushTest, InvalidNullHandleQueue) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE, urQueueFlush(nullptr));
}
