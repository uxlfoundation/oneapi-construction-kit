// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clCreateKernelsInProgramGoodTest : public ucl::ContextTest {
 protected:
  enum { NUM_KERNELS = 10 };

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;} void "
        "kernel bar(global int * a, global int * b) {*a = *b;}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernels[NUM_KERNELS] = {};
};

class clCreateKernelsInProgramBadTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source = "bad kernel";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clCreateKernelsInProgramGoodTest, Default) {
  ASSERT_SUCCESS(clCreateKernelsInProgram(program, 2, kernels, nullptr));
}

TEST_F(clCreateKernelsInProgramGoodTest, NumKernelsRet) {
  cl_uint num_kernels;
  ASSERT_SUCCESS(clCreateKernelsInProgram(program, 2, nullptr, &num_kernels));
  ASSERT_EQ(2u, num_kernels);
}

TEST_F(clCreateKernelsInProgramGoodTest, GoodProgramNumKernelsTooSmall) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCreateKernelsInProgram(program, 0, kernels, nullptr));
}

TEST_F(clCreateKernelsInProgramBadTest, BadProgram) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clCreateKernelsInProgram(nullptr, 0, nullptr, nullptr));
}

TEST_F(clCreateKernelsInProgramBadTest, BadProgramExecutable) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM_EXECUTABLE,
                    clCreateKernelsInProgram(program, 1, nullptr, nullptr));
}
