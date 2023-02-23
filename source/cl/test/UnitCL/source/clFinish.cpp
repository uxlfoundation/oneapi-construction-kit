// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <thread>

#include "Common.h"

using clFinishTest = ucl::CommandQueueTest;

TEST_F(clFinishTest, InvalidCommandQueue) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clFinish(nullptr));
}

TEST_F(clFinishTest, Default) { ASSERT_SUCCESS(clFinish(command_queue)); }

// This test can essentially only fail under a thread-sanitizer build as it
// doesn't "do" anything, so it will never get the wrong result.  The original
// issue being tracked down was a rare crash though, not an incorrect result.
//
// It is aiming to cause enqueuing-work and flushes to be happening
// concurrently on a single cl_command_queue.
//
// See also `TEST_F(clFlushTest, ConcurrentFlushes).
TEST_F(clFinishTest, ConcurrentFinishes) {
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

      ASSERT_SUCCESS(clFinish(command_queue));
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

  clReleaseKernel(kernel);
  clReleaseProgram(program);
}
