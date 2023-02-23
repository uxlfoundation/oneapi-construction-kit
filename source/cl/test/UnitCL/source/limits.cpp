// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This file should contain tests that push "limits".  Some of which may not be
// queryable through the OpenCL API.

#include <limits>
#include <memory>

#include "Common.h"

template <typename T>
struct ReleaseHelper {
  ReleaseHelper(T t) : t(t) {}

  ~ReleaseHelper();

  operator T() const { return t; }

  T *data() { return std::addressof(t); }

 private:
  T t;
};

template <>
ReleaseHelper<cl_kernel>::~ReleaseHelper() {
  clReleaseKernel(t);
}

template <>
ReleaseHelper<cl_mem>::~ReleaseHelper() {
  clReleaseMemObject(t);
}

template <>
ReleaseHelper<cl_event>::~ReleaseHelper() {
  if (t) {
    clReleaseEvent(t);
  }
}

// The purpose of this test is to check that creating variables on the stack of
// a plausible size (a few k) either works success, or results in a compiler
// error.  Really this test is ensuring the the runtime doesn't just crash.
struct StackSizeTest : ucl::CommandQueueTest,
                       testing::WithParamInterface<unsigned> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int err;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &err);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  const char *source = R"(
__kernel void stack(const global char* input, global char* output) {
  char data[STACK_SIZE];
  size_t gid = get_global_id(0);

  output[gid] = 0;

  // Do some arbitrary calculation to ensure that the private array
  // can't be optimized away.  If in the future the private array does
  // get optimized away then it is safe to modify the arbitrary
  // calculation below to a different arbitrary calculation that keeps
  // private array around.

  for (int i = 0; i < STACK_SIZE; i++) {
    data[i] = (input[gid] * 2) % CHAR_MAX;
  }

  int tmp = 0;
  for (int j = 0; j < STACK_SIZE; j++) {
    tmp += data[j];
  }

  output[gid] = tmp % CHAR_MAX;
}
)";

  cl_program program = nullptr;
};

TEST_P(StackSizeTest, LargeStack) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  unsigned stack_size = GetParam();
  std::string stack_arg =
      std::string("-DSTACK_SIZE=").append(std::to_string(stack_size));

  cl_int err =
      clBuildProgram(program, 0, nullptr, stack_arg.c_str(), nullptr, nullptr);

  if (CL_SUCCESS != err) {
    printf(
        "  LIMIT WARNING: clBuildProgram error code (%d) for kernel with a\n"
        "  %u byte stack array.  This may be a hardware limitation.\n",
        err, stack_size);
    return;
  }

  ReleaseHelper<cl_kernel> kernel(clCreateKernel(program, "stack", &err));

  if (CL_SUCCESS != err) {
    printf(
        "  LIMIT WARNING: clCreateKernel error code (%d) for kernel with a\n"
        "  %u byte stack array.  This may be a hardware limitation.\n",
        err, stack_size);
    return;
  }

  size_t size = 256 * sizeof(cl_char);
  ReleaseHelper<cl_mem> mem_a(clCreateBuffer(context, 0, size, nullptr, &err));
  ReleaseHelper<cl_mem> mem_b(clCreateBuffer(context, 0, size, nullptr, &err));

  cl_char pattern = 1;
  ReleaseHelper<cl_event> fill_event(nullptr);
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, mem_a, &pattern, 1, 0, size,
                                     0, nullptr, fill_event.data()));

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(mem_a), &mem_a));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(mem_b), &mem_b));

  ReleaseHelper<cl_event> kernel_event(nullptr);
  err =
      clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &size, nullptr,
                             1, fill_event.data(), kernel_event.data());

  if (CL_SUCCESS != err) {
    printf(
        "  LIMIT WARNING: clEnqueueNDRangeKernel error code (%d) for kernel\n"
        "  with a %u byte stack array.  This may be a hardware limitation.\n",
        err, stack_size);
    return;
  }

  err = clWaitForEvents(1, kernel_event.data());

  // If something went wrong then kernel_event should have a negative status
  // and clWaitForEvents returns CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST.
  // But we'll just check for any error.
  if (CL_SUCCESS != err) {
    printf(
        "  LIMIT WARNING: clWaitForEvents error code (%d) for kernel with a\n"
        "  %u byte stack array.  This may be a hardware limitation.\n",
        err, stack_size);
    return;
  }

  // If we reached here then everything must have worked!  So check that correct
  // result was calculated.

  UCL::vector<cl_char> data(size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, mem_b, CL_TRUE, 0, size,
                                     data.data(), 0, nullptr, nullptr));

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<cl_char>((stack_size * 2) %
                                   std::numeric_limits<cl_char>::max()),
              data[i]);
  }
}

INSTANTIATE_TEST_CASE_P(Limits, StackSizeTest,
                        ::testing::Values(1u, 129u, 513u, 2049u, 4097u, 8193u));
