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

#include <atomic>
#include <thread>

#include "Common.h"

class clBuildProgramGoodTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    cl_int errorcode;
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

TEST_F(clBuildProgramGoodTest, InvalidProgram) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_PROGRAM,
      clBuildProgram(nullptr, 1, &device, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, InvalidValueUserData) {
  char something = 'a';
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clBuildProgram(program, 1, &device, nullptr, nullptr,
                                   static_cast<void *>(&something)));
}

TEST_F(clBuildProgramGoodTest, InvalidValueNumDevices) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clBuildProgram(program, 1, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, InvalidValueDeviceList) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clBuildProgram(program, 0, &device, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, InvalidDevice) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::vector<cl_device_id> devices(1, nullptr);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_DEVICE,
      clBuildProgram(program, static_cast<cl_uint>(devices.size()),
                     devices.data(), nullptr, nullptr, nullptr));
}

// Redmine #5138: Check CL_INVALID_BINARY

// Redmine #5138: Check CL_INVALID_BUILD_OPTIONS

// Redmine #5138: Check CL_COMPILER_NOT_AVAILABLE

TEST_F(clBuildProgramGoodTest, AttemptSecondCompile) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, InvalidOperationKernelAttached) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_int status;
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  EXPECT_SUCCESS(status);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

// Redmine #5138: Check CL_INVALID_OPERATION program not created with
// clCreateProgramWithSource or clCreateProgramWithBinary

// Redmine #5117: Check CL_OUT_OF_RESOURCES

// Redmine #5114: Check CL_OUT_OF_HOST_MEMORY

TEST_F(clBuildProgramGoodTest, Default) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, DefaultAllContextDevices) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramGoodTest, Callback) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  struct UserData {
    int data;
    cl_event event;
    cl_program program;
    cl_int status;
    bool programMatches;
  };

  struct Helper {
    static void CL_CALLBACK callback(cl_program program, void *user_data) {
      UserData *const actualUserData = static_cast<UserData *>(user_data);
      actualUserData->data = 42;
      actualUserData->status =
          clSetUserEventStatus(actualUserData->event, CL_COMPLETE);
      actualUserData->programMatches = (actualUserData->program == program);
    }
  };

  cl_int userEventStatus = !CL_SUCCESS;
  cl_event event = clCreateUserEvent(context, &userEventStatus);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(userEventStatus);

  UserData userData;
  userData.data = 0;
  userData.event = event;
  userData.program = program;
  userData.status = !CL_SUCCESS;
  userData.programMatches = false;

  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, nullptr, Helper::callback,
                                &userData));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_EQ(42, userData.data);

  ASSERT_SUCCESS(userData.status);

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clBuildProgramGoodTest, DefaultUseProgram) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  cl_int status;
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  EXPECT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

TEST_F(clBuildProgramGoodTest, EmptySource) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *emptySource = "// This program contains no code!";

  cl_int errorcode = !CL_SUCCESS;
  cl_program emptyProgram =
      clCreateProgramWithSource(context, 1, &emptySource, nullptr, &errorcode);
  EXPECT_TRUE(emptyProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(emptyProgram, 0, nullptr, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(emptyProgram));
}

TEST_F(clBuildProgramGoodTest, EmptyProgram) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *emptySource = "";

  cl_int errorcode = !CL_SUCCESS;
  cl_program emptyProgram =
      clCreateProgramWithSource(context, 1, &emptySource, nullptr, &errorcode);
  EXPECT_TRUE(emptyProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(emptyProgram, 0, nullptr, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(emptyProgram));
}

TEST_F(clBuildProgramGoodTest, CompilerUnavailable) {
  if (getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  ASSERT_EQ_ERRCODE(
      CL_COMPILER_NOT_AVAILABLE,
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
}

// This test exists because there used to be a datarace on initializing the
// compiler within a cl_context.  A key point of this test is that all the
// initial clBuildProgram's done within the context are in parallel, if one
// clBuildProgram could do enough initialization before the others start then
// there was no crash.
TEST_F(clBuildProgramGoodTest, ConcurrentBuild) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  const char *src = "kernel void k() {}";

  // This error code is only overwritten if a non-success code is seen, thus
  // serialization should be avoided when there are no errors.
  std::atomic<cl_int> error{CL_SUCCESS};
#define CHECK_ERROR(ERR_CODE)           \
  {                                     \
    const cl_int err_code = (ERR_CODE); \
    if (CL_SUCCESS != err_code) {       \
      error = err_code;                 \
    }                                   \
  }

  auto worker = [this, &src, &error]() {
    cl_int err = !CL_SUCCESS;
    cl_program program =
        clCreateProgramWithSource(context, 1, &src, nullptr, &err);
    CHECK_ERROR(err);
    CHECK_ERROR(clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    CHECK_ERROR(clReleaseProgram(program));
  };

  const size_t threads = 4;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  EXPECT_SUCCESS(error);
}

class clBuildProgramBadTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source = "bad kernel";
    cl_int errorcode;
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

TEST_F(clBuildProgramBadTest, BuildProgramFailure) {
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
}

TEST_F(clBuildProgramBadTest, InvalidOperationPreviousBuildFailed) {
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
}

typedef std::pair<cl_int, const char *> Pair;

struct BuildOptionsTest : ucl::ContextTest, testing::WithParamInterface<Pair> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int status;
    const char *source =
        "kernel void foo(global int *a, global int *b) { *a = *b; }";
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_P(BuildOptionsTest, CompileWithOption) {
  ASSERT_EQ_ERRCODE(
      GetParam().first,
      clBuildProgram(program, 0, nullptr, GetParam().second, nullptr, nullptr))
      << "options: " << GetParam().second;
}

INSTANTIATE_TEST_CASE_P(
    clBuildProgram, BuildOptionsTest,
    ::testing::Values(Pair(CL_SUCCESS, "-w"), Pair(CL_SUCCESS, "-Werror"),
                      Pair(CL_SUCCESS, "-cl-single-precision-constant"),
                      Pair(CL_SUCCESS, "-cl-opt-disable"),
                      Pair(CL_SUCCESS, "-cl-strict-aliasing"),
                      Pair(CL_SUCCESS, "-cl-mad-enable"),
                      Pair(CL_SUCCESS, "-cl-unsafe-math-optimizations"),
                      Pair(CL_SUCCESS, "-cl-finite-math-only"),
                      Pair(CL_SUCCESS, "-cl-fast-relaxed-math"),
                      Pair(CL_SUCCESS, "-cl-std=CL1.1"),
                      Pair(CL_SUCCESS, "-cl-std=CL1.2"),
                      Pair(CL_SUCCESS, "-cl-kernel-arg-info"),
                      Pair(CL_SUCCESS, "-cl-denorms-are-zero"),
                      Pair(CL_SUCCESS, "-cl-no-signed-zeros"),
                      Pair(CL_SUCCESS, "-cl-uniform-work-group-size"),
                      Pair(CL_SUCCESS, "-cl-no-subgroup-ifp")));

TEST_F(BuildOptionsTest, CompileWithOptionFP32CorrectlyRoundedDivideSqrt) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection erroneously succeeds.
  }
  const char *option = "-cl-fp32-correctly-rounded-divide-sqrt";
  if (UCL::hasCorrectlyRoundedDivideSqrtSupport(device)) {
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, option, nullptr, nullptr))
        << "option: " << option;
  } else {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_BUILD_OPTIONS,
        clBuildProgram(program, 0, nullptr, option, nullptr, nullptr))
        << "option: " << option;
  }
}

class clBuildProgramMacroTest : public ucl::CommandQueueTest {
 protected:
  enum { SIZE = sizeof(cl_int) };

  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "kernel void foo(global int *i)\n"
        "{\n"
        "#ifdef TEST\n"
        "#if TEST > 1\n"
        "  i[get_global_id(0)] = TEST;\n"
        "#else\n"
        "  i[get_global_id(0)] = TEST;\n"
        "#endif\n"
        "#else\n"
        "  i[get_global_id(0)] = 0;\n"
        "#endif\n"
        "}";
    cl_int status;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    macro_value =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, SIZE, nullptr, &status);
    EXPECT_TRUE(macro_value);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (macro_value) {
      EXPECT_SUCCESS(clReleaseMemObject(macro_value));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem macro_value = nullptr;
};

TEST_F(clBuildProgramMacroTest, NotDefined) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // being passed the ValueDefined program
  }
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));
  cl_int status;
  kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macro_value)));
  cl_event taskEvent;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macro_value, CL_TRUE, 0,
                                     sizeof(cl_int), &value, 1, &taskEvent,
                                     nullptr));
  EXPECT_EQ(0, value);  // macro TEST was not defined, kernel return 0
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

TEST_F(clBuildProgramMacroTest, DefaultDefined) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // being passed the ValueDefined program
  }
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-DTEST", nullptr, nullptr));
  cl_int status;
  kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macro_value)));
  cl_event taskEvent;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macro_value, CL_TRUE, 0,
                                     sizeof(cl_int), &value, 1, &taskEvent,
                                     nullptr));
  EXPECT_EQ(1, value);  // macro TEST was defined with the default value 1
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

TEST_F(clBuildProgramMacroTest, ValueDefined) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-DTEST=42", nullptr, nullptr));
  cl_int status;
  kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macro_value)));
  cl_event taskEvent;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macro_value, CL_TRUE, 0,
                                     sizeof(cl_int), &value, 1, &taskEvent,
                                     nullptr));
  EXPECT_EQ(42, value);  // macro TEST was defined with the value 42
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

TEST_F(clBuildProgramMacroTest, ValueDefinedThenSpace) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-DTEST=42 ", nullptr, nullptr));
  cl_int status;
  kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macro_value)));
  cl_event taskEvent;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macro_value, CL_TRUE, 0,
                                     sizeof(cl_int), &value, 1, &taskEvent,
                                     nullptr));
  EXPECT_EQ(42, value);  // macro TEST was defined with the value 42
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

class clBuildProgramTwiceTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "kernel void foo(global int *i)\n"
        "{\n"
        "  i[get_global_id(0)] = TEST;\n"
        "}";
    cl_int status;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    macro_value = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int),
                                 nullptr, &status);
    EXPECT_TRUE(macro_value);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (macro_value) {
      EXPECT_SUCCESS(clReleaseMemObject(macro_value));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  void RunAndGetResult(cl_program program, cl_int *result, bool release_early,
                       bool release_late) {
    ASSERT_FALSE(release_early && release_late);  // Can only release once.
    cl_int status;
    kernel = clCreateKernel(program, "foo", &status);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&macro_value)));
    cl_event taskEvent;
    ASSERT_SUCCESS(
        clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
    if (release_early) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macro_value, CL_TRUE, 0,
                                       sizeof(cl_int), result, 1, &taskEvent,
                                       nullptr));
    ASSERT_SUCCESS(clReleaseEvent(taskEvent));
    if (release_late) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem macro_value = nullptr;
};

TEST_F(clBuildProgramTwiceTest, RedefineMacro) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // This test was written to narrow down a timing failure (sometimes
  // clBuildProgram would return CL_INVALID_OPERATION from a second
  // clBuildProgram call), so run this test in a loop to increase the chance
  // of triggering the timing issue.  Early vs late kernel release doesn't
  // really matter, this is just for variation.
  for (unsigned i = 0; i < 10u; i++) {
    cl_int result1 = -1, result2 = -1;
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, "-DTEST=42", nullptr, nullptr));
    RunAndGetResult(program, &result1, false, true);  // Release kernel late.
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, "-DTEST=43", nullptr, nullptr));
    RunAndGetResult(program, &result2, true, false);  // Release kernel early.
    EXPECT_EQ(result1, 42);
    EXPECT_EQ(result2, 43);
  }
}

TEST_F(clBuildProgramTwiceTest, RetainKernel) {
  // We build and run the program, but don't release the kernel yet.  Then
  // we try to rebuild the program and thus expect an error due to the
  // still attached kernel.  Finally, we release the kernel.
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-DTEST=42", nullptr, nullptr));
  cl_int result = -1;
  RunAndGetResult(program, &result, false, false);
  EXPECT_EQ(result, 42);
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clBuildProgram(program, 0, nullptr, "-DTEST=43", nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

class clBuildProgramIncludePathTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int status;
    const char *source =
        "#include \"test_include.h\"\n"
        "kernel void foo(global int *i) { *i = ultimate_question(); }";
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);

    UCL::checkTestIncludePath();
  }

  void TestGoodPath(const std::string &option, const std::string &path) {
    const std::string options = option + path;
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, options.c_str(), nullptr, nullptr));
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clBuildProgramIncludePathTest, GoodPathWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePath());
}

TEST_F(clBuildProgramIncludePathTest, DISABLED_GoodQuotedSpacesPathWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePathWithQuotedSpaces());
}

TEST_F(clBuildProgramIncludePathTest,
       DISABLED_GoodBackslashedSpacesPathWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePathWithBackslashedSpaces());
}

TEST_F(clBuildProgramIncludePathTest, GoodPathNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePath());
}

TEST_F(clBuildProgramIncludePathTest, DISABLED_GoodQuotedSpacesPathNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePathWithQuotedSpaces());
}

TEST_F(clBuildProgramIncludePathTest,
       DISABLED_GoodBackslashedSpacesPathNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePathWithQuotedSpaces());
}

TEST_F(clBuildProgramIncludePathTest, MissingHeader) {
  const char *source = "#include \"header_does_not_exist.h\"\n";

  cl_int status = !CL_SUCCESS;
  cl_program missing_header =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  EXPECT_TRUE(missing_header);
  ASSERT_SUCCESS(status);

  const std::string option = "-I ";
  const std::string options = option + UCL::getTestIncludePath();

  ASSERT_EQ_ERRCODE(CL_BUILD_PROGRAM_FAILURE,
                    clBuildProgram(missing_header, 0, nullptr, options.c_str(),
                                   nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(missing_header));
}

TEST_F(clBuildProgramIncludePathTest, MissingDeclaration) {
  // The test_empty_include header does not declare ultimate_question().
  const char *source =
      "#include \"test_empty_include.h\"\n"
      "kernel void foo(global int *answer) { *answer = ultimate_question(); "
      "}\n";
  cl_int status = !CL_SUCCESS;
  cl_program missing_dec =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  EXPECT_TRUE(missing_dec);
  ASSERT_SUCCESS(status);

  const std::string option = "-I ";
  const std::string options = option + UCL::getTestIncludePath();

  ASSERT_EQ_ERRCODE(CL_BUILD_PROGRAM_FAILURE,
                    clBuildProgram(missing_dec, 0, nullptr, options.c_str(),
                                   nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(missing_dec));
}

TEST_F(clBuildProgramIncludePathTest, BadPathWithSpace) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection erroneously succeeds.
  }
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, "-I /bad/path", nullptr, nullptr));
}

TEST_F(clBuildProgramIncludePathTest, BadPathNoSpace) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection erroneously succeeds.
  }
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, "-I/bad/path", nullptr, nullptr));
}

using clBuildProgramTest = ucl::DeviceTest;

TEST_F(clBuildProgramTest, ReleaseInReverseOrder) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(errorcode);

  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseContext(context));

  ASSERT_SUCCESS(clReleaseProgram(program));
}

// clBuildProgramBadKernelTest tests are ones that are expected to pass
// clCreateProgramWithSource, but result in CL_BUILD_PROGRAM_FAILURE from
// clBuildProgram.  This is not a parameterized test simply so that it is easy
// to give each test a descriptive name.
class clBuildProgramBadKernelTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }

  void Build(const char *source) {
    cl_int errorcode = !CL_SUCCESS;
    cl_program program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);

    ASSERT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    ASSERT_SUCCESS(clReleaseProgram(program));
  }
};

TEST_F(clBuildProgramBadKernelTest, UnresolvedExternal) {
  const char *source = R"cl(
int bar(int x, int y);
void kernel foo(global int * a, global int * b) { *a = bar(*a, *b); }
)cl";
  Build(source);
}

TEST_F(clBuildProgramBadKernelTest, RecursiveKernel1) {
  const char *source = R"cl(
kernel void call1(global int *out, global int *i);
kernel void call2(global int *out, int gid, global int *i) {
  if (*i++ < 10) { call1(out, i); }
  out[gid]++;
}
kernel void call1(global int *out, global int *i) {
  size_t gid = get_global_id(0);
  call2(out, gid, i);
  out[gid]++;
}
)cl";
  Build(source);
}

// For this test to work this must not be tail recursive, or Clang
// optimizations may turn the recursion into a loop (this test expects the
// code to not compile due to recursion).
TEST_F(clBuildProgramBadKernelTest, RecursiveKernel2) {
  const char *source = R"cl(
kernel void rec(global int *out, int n) {
  size_t gid = get_global_id(0);
  if (n == 0) { return; }
  rec(out, n - 1);
  if (gid % 4) { out[gid] = n; }
}
kernel void call(global int *out, int n) {
  size_t gid = get_global_id(0);
  if (gid % 2) { out[gid] = n; }
  rec(out, n);
}
)cl";
  Build(source);
}

// This test exists because one of our debug support classes would segfault
// when trying to process the call to the function without a declaration.
TEST_F(clBuildProgramBadKernelTest, MissingCalledFunction) {
  const char *source = R"cl(
kernel void f(global float *param_must_exist_for_crash) {
  function_that_does_not_exist(param_must_exist_for_crash);
}
)cl";
  Build(source);
}
