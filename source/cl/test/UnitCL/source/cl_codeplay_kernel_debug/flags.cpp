// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

struct cl_codeplay_kernel_debug : ucl::ContextTest {
  cl_program program = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!UCL::hasDeviceExtensionSupport(device, "cl_codeplay_kernel_debug")) {
      GTEST_SKIP();
    }
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &error);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }
};

TEST_F(cl_codeplay_kernel_debug, compileDebugInfoFlag) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "-g", 0, nullptr,
                                  nullptr, nullptr, nullptr));
}

TEST_F(cl_codeplay_kernel_debug, buildDebugInfoFlag) {
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-g", nullptr, nullptr));
}

TEST_F(cl_codeplay_kernel_debug, compileSourceFlag) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "-S /path/to/cl/source",
                                  0, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(cl_codeplay_kernel_debug, buildSourceFlag) {
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-S /path/to/cl/source",
                                nullptr, nullptr));
}

TEST_F(cl_codeplay_kernel_debug, compileSourceAndDebugInfoFlags) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr,
                                  "-S /path/to/cl/source -g", 0, nullptr,
                                  nullptr, nullptr, nullptr));
}

TEST_F(cl_codeplay_kernel_debug, buildSourceAndDebugInfoFlags) {
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-g -S /path/to/cl/source",
                                nullptr, nullptr));
}
