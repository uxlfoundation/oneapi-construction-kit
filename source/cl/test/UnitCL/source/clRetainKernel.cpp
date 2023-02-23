// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clRetainKernelTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clRetainKernelTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_KERNEL, clRetainKernel(nullptr));
  ASSERT_SUCCESS(clRetainKernel(kernel));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}
