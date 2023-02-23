// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <thread>

#include "Common.h"

class clCreateKernelTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;} void "
        "kernel bar(global int * a, global int * b) {*a = *b;}";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
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

TEST_F(clCreateKernelTest, Default) {
  cl_int errorcode;
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

TEST_F(clCreateKernelTest, TwoKernels) {
  cl_int errorcode;
  cl_kernel foo = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(foo);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseKernel(foo));

  cl_kernel bar = clCreateKernel(program, "bar", &errorcode);
  EXPECT_TRUE(bar);
  EXPECT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseKernel(bar));
}

TEST_F(clCreateKernelTest, BadProgram) {
  cl_int errorcode;
  EXPECT_FALSE(clCreateKernel(nullptr, "foo", &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM, errorcode);
}

TEST_F(clCreateKernelTest, OnlyCreatedProgram) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection erroneously succeeds.
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  cl_program otherProgram =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  EXPECT_FALSE(clCreateKernel(otherProgram, "foo", &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM_EXECUTABLE, errorcode);

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clCreateKernelTest, OnlyCompiledProgram) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  cl_program otherProgram =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clCompileProgram(otherProgram, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));

  EXPECT_FALSE(clCreateKernel(otherProgram, "foo", &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM_EXECUTABLE, errorcode);

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clCreateKernelTest, InvalidKernelName) {
  cl_int errorcode;
  EXPECT_FALSE(clCreateKernel(program, "sus", &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL_NAME, errorcode);
}

TEST_F(clCreateKernelTest, NullKernelName) {
  cl_int errorcode;
  EXPECT_FALSE(clCreateKernel(program, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clCreateKernelTest, BuildProgramAfterCreateKernel) {
  cl_int errorcode;
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errorcode);

  // Redmine #5148: check CL_INVALID_OPERATION is the correct return code!
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

TEST_F(clCreateKernelTest, CompileProgramAfterCreateKernel) {
  cl_int errorcode;
  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errorcode);

  // Redmine #5148: check CL_INVALID_OPERATION is the correct return code!
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                     nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

// This test exists because we used to have a data-race on an LLVM global
// variable between `clBuildProgram` and `clCreateKernel`, even when there were
// operating on separate `cl_program` objects.
TEST_F(clCreateKernelTest, ConcurrentBuildAndCreate) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k() {}";

  auto worker = [this, &src]() {
    for (int i = 0; i < 32; i++) {
      cl_program program =
          clCreateProgramWithSource(context, 1, &src, nullptr, nullptr);
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
      cl_kernel kernel = clCreateKernel(program, "k", nullptr);

      clReleaseKernel(kernel);
      clReleaseProgram(program);
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
}
