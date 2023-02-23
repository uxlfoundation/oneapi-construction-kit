// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class KernelFeaturesTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    const char *source = R"OpenCLC(
        void kernel foo(global int * a) {
          int foo[sizeof(long) == 8 ? 1 : -1]; *a = foo[0];
        })OpenCLC";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(KernelFeaturesTest, Default) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(this->program, 0, nullptr, nullptr, nullptr, nullptr));
}
