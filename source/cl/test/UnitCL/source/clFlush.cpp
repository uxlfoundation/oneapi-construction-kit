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

#include "Common.h"

using clFlushTest = ucl::CommandQueueTest;

TEST_F(clFlushTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clFlush(nullptr));
}

TEST_F(clFlushTest, Default) { ASSERT_SUCCESS(clFlush(command_queue)); }

// This test can essentially only fail under a thread-sanitizer build as it
// doesn't "do" anything, so it will never get the wrong result.  The original
// issue being tracked down was a rare crash though, not an incorrect result.
//
// It is aiming to cause enqueuing-work and flushes to be happening
// concurrently on a single cl_command_queue.
//
// See also `TEST_F(clFinishTest, ConcurrentFinishes).
TEST_F(clFlushTest, ConcurrentFlushes) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *src = "kernel void k() {}";
  const size_t range = 1;

  cl_int errcode = !CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &src, nullptr, &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "k", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);

  auto worker = [this, &kernel, &range]() {
    for (int i = 0; i < 32; i++) {
      ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                            &range, nullptr, 0, nullptr,
                                            nullptr));

      ASSERT_SUCCESS(clFlush(command_queue));
    }
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  // This clFinish is here to operate concurrently with the worker thread
  // clFlush operations, this has been known to causing issues in the past.
  ASSERT_SUCCESS(clFinish(command_queue));

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // This clFinish is here to ensure all work is complete before we start
  // releasing the kernel etc.
  ASSERT_SUCCESS(clFinish(command_queue));

  clReleaseKernel(kernel);
  clReleaseProgram(program);
}

// This test can't fail, but at the time it was written it would deadlock
// within 10-20 runs for a system under load.  It never seemed to deadlock
// without load, it never triggered any TSAN warnings under any conditions.
//
// The test is aiming to cause flushes and finishes to be happening
// concurrently on a single cl_command_queue.  This also requires enqueueing
// work as we go so that the flushes and finishes are not just no-ops.
TEST_F(clFlushTest, ConcurrentFlushFinish) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *src = "kernel void k() {}";
  const size_t range = 1;

  cl_int errcode = !CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &src, nullptr, &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "k", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);

  // Note: Flush threads run 4x as many interations as finish threads because
  // we expect finish threads to progress more slowly.

  auto worker_flush = [this, &kernel, &range]() {
    for (int i = 0; i < 64; i++) {
      ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                            &range, nullptr, 0, nullptr,
                                            nullptr));

      ASSERT_SUCCESS(clFlush(command_queue));
    }
  };

  auto worker_finish = [this, &kernel, &range]() {
    for (int i = 0; i < 16; i++) {
      ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                            &range, nullptr, 0, nullptr,
                                            nullptr));

      ASSERT_SUCCESS(clFinish(command_queue));
    }
  };

  // Need at least 8 threads to have enough of each worker type to quickly
  // trigger the deadlock being tested (under load, without load even setting
  // this to a huge number like 1024 did not trigger the issue).
  const size_t threads = 8;
  UCL::vector<std::thread> workers(threads);

  // Half the threads are created with the 'flush' worker, half with the
  // 'finish' worker.
  for (size_t i = 0; i < threads; i++) {
    if ((i % 2) == 0) {
      workers[i] = std::thread(worker_flush);
    } else {
      workers[i] = std::thread(worker_finish);
    }
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // Ensure that the work that the threads that were 'flush' enqueued has
  // actually finished before we start releasing resources.
  ASSERT_SUCCESS(clFinish(command_queue));

  clReleaseKernel(kernel);
  clReleaseProgram(program);
}

// We had a deadlock that occurred when an event call back was used to enqueue
// more work, and then call clFlush so this test does exactly that from a few
// threads at once. This is very similar to clSetEventCallback.EnqueueCallback
// but it also calls clFlush within the callback as these were two separate
// deadlocks.
static void CL_CALLBACK EnqueueFlushCallback(cl_event, cl_int, void *s) {
  auto state = static_cast<std::pair<cl_command_queue, cl_kernel> *>(s);
  const size_t range = 1;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(state->first, state->second, 1, nullptr,
                                        &range, nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(clFlush(state->first));
}

TEST_F(clFlushTest, EnqueueFlushCallback) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k() {}";
  const size_t range = 1;

  cl_int errcode = !CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &src, nullptr, &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "k", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);

  std::pair<cl_command_queue, cl_kernel> state(command_queue, kernel);

  auto worker = [this, &kernel, &range, &state]() {
    for (int i = 0; i < 32; i++) {
      cl_event event;

      ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                            &range, nullptr, 0, nullptr,
                                            &event));
      clSetEventCallback(event, CL_COMPLETE, EnqueueFlushCallback, &state);
      ASSERT_SUCCESS(clWaitForEvents(1, &event));
      ASSERT_SUCCESS(clReleaseEvent(event));
    }
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // Just make sure that all work has actually finished before we start
  // releasing resources.
  ASSERT_SUCCESS(clFinish(command_queue));

  clReleaseKernel(kernel);
  clReleaseProgram(program);
}
