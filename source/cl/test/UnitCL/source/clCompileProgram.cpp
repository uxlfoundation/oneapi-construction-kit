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

class clCompileProgramGoodTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
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

class clCompileProgramBadTest : public ucl::ContextTest {
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

class clCompileProgramCompilerlessTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
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

TEST_F(clCompileProgramGoodTest, Default) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 1, &device, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, DefaultAllContextDevices) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, Callback) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
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

  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, Helper::callback, &userData));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_EQ(42, userData.data);

  ASSERT_SUCCESS(userData.status);

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clCompileProgramGoodTest, AttemptSecondCompile) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, UseEmbeddedHeaders) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection can't find dumped program.
  }
  const char *programSource = "#include <header>";

  cl_int errorcode = !CL_SUCCESS;
  cl_program otherProgram = clCreateProgramWithSource(
      context, 1, &programSource, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  const char *headerName = "header";

  ASSERT_SUCCESS(clCompileProgram(otherProgram, 0, nullptr, nullptr, 1,
                                  &program, &headerName, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clCompileProgramGoodTest, NullProgram) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clCompileProgram(nullptr, 0, nullptr, nullptr, 0, nullptr,
                                     nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, ManyDevicesNullDevice) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 1, nullptr, nullptr, 0, nullptr,
                                     nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, ZeroDeviesWithDevices) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 0, &device, nullptr, 0, nullptr,
                                     nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, ZeroHeadersNonNullHeaderNames) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 0, nullptr, nullptr, 0, &program,
                                     nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, ZeroHeadersNonNullHeaders) {
  const char *something = "something";
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                     &something, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, NonZeroHeadersWithNullHeader) {
  const char *something = "something";
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 0, nullptr, nullptr, 1, nullptr,
                                     &something, nullptr, nullptr));
}

TEST_F(clCompileProgramGoodTest, NonZeroHeadersWithNullHeadersNames) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCompileProgram(program, 0, nullptr, nullptr, 1, &program,
                                     nullptr, nullptr, nullptr));
}

// This test is testing behaviour that in not mandated by the OpenCL 1.2
// specification.  When clCompileProgram is given a non-null array of
// cl_program's as headers, but one or more of those programs is invalid, the
// specification does not provide a behaviour.  This test enforces the return
// of CL_INVALID_PROGRAM in such a case.
TEST_F(clCompileProgramGoodTest, UNSPECIFIED_InvalidHeader) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();
  }
  const char *programSource = "#include <header>\n#include <header2>";

  cl_int errorcode = !CL_SUCCESS;
  cl_program otherProgram = clCreateProgramWithSource(
      context, 1, &programSource, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  // Create an invalid cl_program by creating it with non-existent source.
  errorcode = !CL_SUCCESS;
  cl_program invalidHeader =
      clCreateProgramWithSource(context, 0, nullptr, nullptr, &errorcode);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);

  // Note that we are continuing to use invalidHeader despite an error code
  // that says we cannot.  This is to test what clCompileProgram does with an
  // invalid header.

  // headers[0] is valid, headers[1] is not.
  const char *headerNames[] = {"header", "header2"};
  const cl_program headers[] = {program, invalidHeader};

  // The OpenCL 1.2 specification does not say what clCompileProgram should
  // return in the event of a single header being invalid, but treating it the
  // same as the main program being invalid.
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clCompileProgram(otherProgram, 0, nullptr, nullptr, 2,
                                     headers, headerNames, nullptr, nullptr))
      << "This test is not required by the specification, use "
      << "--gtest_filter=-*UNSPECIFIED* to disable.";

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clCompileProgramGoodTest, DataWithoutCallback) {
  char something = 'a';
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr,
                       nullptr, static_cast<void *>(&something)));
}

TEST_F(clCompileProgramGoodTest, InvalidDevice) {
  UCL::vector<cl_device_id> devices(1, device);
  devices.push_back(nullptr);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_DEVICE,
      clCompileProgram(program, static_cast<cl_uint>(devices.size()),
                       devices.data(), nullptr, 0, nullptr, nullptr, nullptr,
                       nullptr));
}

TEST_F(clCompileProgramGoodTest, EmptySource) {
  const char *emptySource = "// This program contains no code!";

  cl_int errorcode = !CL_SUCCESS;
  cl_program emptyProgram =
      clCreateProgramWithSource(context, 1, &emptySource, nullptr, &errorcode);
  EXPECT_TRUE(emptyProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clCompileProgram(emptyProgram, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));

  cl_int status;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, "", 1, &emptyProgram, nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  EXPECT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));

  EXPECT_SUCCESS(clReleaseProgram(emptyProgram));
}

// This test exists because there used to be a datarace on initializing the
// compiler within a cl_context.  A key point of this test is that all the
// initial clCompileProgram's done within the context are in parallel, if one
// clBuildProgram could do enough initialization before the others start then
// there was no crash.
TEST_F(clCompileProgramGoodTest, ConcurrentCompile) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
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
    CHECK_ERROR(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                 nullptr, nullptr, nullptr));
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

TEST_F(clCompileProgramCompilerlessTest, CompilerUnavailable) {
  ASSERT_EQ_ERRCODE(CL_COMPILER_NOT_AVAILABLE,
                    clCompileProgram(program, 1, &device, nullptr, 0, nullptr,
                                     nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramBadTest, BadSource) {
  ASSERT_EQ(CL_COMPILE_PROGRAM_FAILURE,
            clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr,
                             nullptr, nullptr));
}

TEST_F(clCompileProgramBadTest, UseBadEmbeddedHeaders) {
  const char *programSource = "#include <header>";

  cl_int errorcode = !CL_SUCCESS;
  cl_program otherProgram = clCreateProgramWithSource(
      context, 1, &programSource, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  const char *headerName = "header";

  ASSERT_EQ_ERRCODE(CL_COMPILE_PROGRAM_FAILURE,
                    clCompileProgram(otherProgram, 0, nullptr, nullptr, 1,
                                     &program, &headerName, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

typedef std::pair<cl_int, const char *> Pair;

struct CompileOptionsTest : ucl::ContextTest,
                            testing::WithParamInterface<Pair> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "kernel void foo(global int *a, global int *b) { *a = *b; }";
    cl_int status;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
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

TEST_P(CompileOptionsTest, CompileWithOption) {
  ASSERT_EQ_ERRCODE(GetParam().first,
                    clCompileProgram(program, 0, nullptr, GetParam().second, 0,
                                     nullptr, nullptr, nullptr, nullptr))
      << "options: " << GetParam().second;
}

INSTANTIATE_TEST_CASE_P(
    clCompileProgram, CompileOptionsTest,
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
                      Pair(CL_SUCCESS, "-cl-no-signed-zeros")));

TEST_F(CompileOptionsTest, CompileWithOptionFP32CorrectlyRoundedDivideSqrt) {
  const char *option = "-cl-fp32-correctly-rounded-divide-sqrt";
  if (UCL::hasCorrectlyRoundedDivideSqrtSupport(device)) {
    ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, option, 0, nullptr,
                                    nullptr, nullptr, nullptr))
        << "option: " << option;
  } else {
    ASSERT_EQ_ERRCODE(CL_INVALID_COMPILER_OPTIONS,
                      clCompileProgram(program, 0, nullptr, option, 0, nullptr,
                                       nullptr, nullptr, nullptr))
        << "option: " << option;
  }
}

class clCompileProgramMacroTest : public ucl::CommandQueueTest {
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
    ASSERT_SUCCESS(status);
    macroValue =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, SIZE, nullptr, &status);
    ASSERT_SUCCESS(status);
  }

  void TearDown() {
    if (macroValue) {
      EXPECT_SUCCESS(clReleaseMemObject(macroValue));
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
  cl_mem macroValue = nullptr;
};

TEST_F(clCompileProgramMacroTest, NotDefined) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "", 0, nullptr, nullptr,
                                  nullptr, nullptr));
  cl_int status;
  cl_program linkedProgram = clLinkProgram(context, 0, nullptr, "", 1, &program,
                                           nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  EXPECT_SUCCESS(status);
  kernel = clCreateKernel(linkedProgram, "foo", &status);
  EXPECT_SUCCESS(status);
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macroValue)));
  cl_event taskEvent;
  EXPECT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macroValue, CL_TRUE, 0,
                                     SIZE, &value, 1, &taskEvent, nullptr));
  EXPECT_EQ(0, value);  // macro TEST was not defined, kernel returns 0

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

TEST_F(clCompileProgramMacroTest, DefaultDefined) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "-DTEST", 0, nullptr,
                                  nullptr, nullptr, nullptr));
  cl_int status;
  cl_program linkedProgram = clLinkProgram(context, 0, nullptr, "", 1, &program,
                                           nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  EXPECT_SUCCESS(status);
  kernel = clCreateKernel(linkedProgram, "foo", &status);
  EXPECT_SUCCESS(status);
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macroValue)));
  cl_event taskEvent;
  EXPECT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macroValue, CL_TRUE, 0,
                                     SIZE, &value, 1, &taskEvent, nullptr));
  ASSERT_EQ(1, value);  // macro TEST was defined with the default value 1

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

TEST_F(clCompileProgramMacroTest, ValueDefined) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, "-DTEST=42", 0, nullptr,
                                  nullptr, nullptr, nullptr));
  cl_int status;
  cl_program linkedProgram = clLinkProgram(context, 0, nullptr, "", 1, &program,
                                           nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  EXPECT_SUCCESS(status);
  kernel = clCreateKernel(linkedProgram, "foo", &status);
  EXPECT_SUCCESS(status);
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&macroValue)));
  cl_event taskEvent;
  EXPECT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  cl_int value;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, macroValue, CL_TRUE, 0,
                                     SIZE, &value, 1, &taskEvent, nullptr));
  EXPECT_EQ(42, value);  // macro TEST was defined with the value 42

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  ASSERT_SUCCESS(clReleaseEvent(taskEvent));
}

class clCompileProgramIncludePathTest : public ucl::ContextTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "#include \"test_include.h\"\n"
        "kernel void foo(global int *answer) { *answer = ultimate_question(); "
        "}\n";
    cl_int status;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    UCL::checkTestIncludePath();
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  void TestGoodPath(const std::string &option, const std::string &path) {
    const std::string options = option + path;
    ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, options.c_str(), 0,
                                    nullptr, nullptr, nullptr, nullptr));

    cl_int status;
    cl_program linkedProgram = clLinkProgram(
        context, 0, nullptr, "", 1, &program, nullptr, nullptr, &status);
    EXPECT_TRUE(linkedProgram);
    EXPECT_SUCCESS(status);
    ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  }

  cl_program program = nullptr;
};

TEST_F(clCompileProgramIncludePathTest, GoodPathWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePath());
}

TEST_F(clCompileProgramIncludePathTest,
       DISABLED_GoodQuotedSpacesPathWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePathWithQuotedSpaces());
}

TEST_F(clCompileProgramIncludePathTest,
       DISABLED_GoodPathBackslashedSpaceWithSpace) {
  TestGoodPath("-I ", UCL::getTestIncludePathWithBackslashedSpaces());
}

TEST_F(clCompileProgramIncludePathTest, GoodPathNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePath());
}

TEST_F(clCompileProgramIncludePathTest, DISABLED_GoodQuotedSpacesPathNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePathWithQuotedSpaces());
}

TEST_F(clCompileProgramIncludePathTest,
       DISABLED_GoodPathBackslashedSpaceNoSpace) {
  TestGoodPath("-I", UCL::getTestIncludePathWithBackslashedSpaces());
}

TEST_F(clCompileProgramIncludePathTest, MissingHeader) {
  const char *source = "#include \"header_does_not_exist.h\"\n";

  cl_int status = !CL_SUCCESS;
  cl_program missing_header =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  EXPECT_TRUE(missing_header);
  ASSERT_SUCCESS(status);

  const std::string option = "-I ";
  const std::string options = option + UCL::getTestIncludePath();
  ASSERT_EQ_ERRCODE(
      CL_COMPILE_PROGRAM_FAILURE,
      clCompileProgram(missing_header, 0, nullptr, options.c_str(), 0, nullptr,
                       nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(missing_header));
}

// Successful extern const use in clLinkProgramTest.ExternConstantDecl
TEST_F(clCompileProgramIncludePathTest, MissingExternConstant) {
  // The test_empty_include header does not declare constant meaning_of_life.
  const char *source =
      "#include \"test_empty_include.h\"\n"
      "kernel void foo(global int *answer) { *answer = meaning_of_life; "
      "}\n";

  cl_int status = !CL_SUCCESS;
  cl_program missing_dec =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  EXPECT_TRUE(missing_dec);
  ASSERT_SUCCESS(status);

  const std::string option = "-I ";
  const std::string options = option + UCL::getTestIncludePath();
  ASSERT_EQ_ERRCODE(CL_COMPILE_PROGRAM_FAILURE,
                    clCompileProgram(missing_dec, 0, nullptr, options.c_str(),
                                     0, nullptr, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseProgram(missing_dec));
}

TEST_F(clCompileProgramIncludePathTest, BadPathWithSpace) {
  ASSERT_EQ_ERRCODE(CL_COMPILE_PROGRAM_FAILURE,
                    clCompileProgram(program, 0, nullptr, "-I /bad/path", 0,
                                     nullptr, nullptr, nullptr, nullptr));
}

TEST_F(clCompileProgramIncludePathTest, BadPathNoSpace) {
  ASSERT_EQ_ERRCODE(CL_COMPILE_PROGRAM_FAILURE,
                    clCompileProgram(program, 0, nullptr, "-I/bad/path", 0,
                                     nullptr, nullptr, nullptr, nullptr));
}

class clCompileLinkEmbeddedHeader : public ucl::ContextTest {
 protected:
  clCompileLinkEmbeddedHeader(const char *source, const char *header_source)
      : source(source), header_source(header_source) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clCompileProgram(program, 1, &device, nullptr, 0, nullptr,
                                    nullptr, nullptr, nullptr));

    header = clCreateProgramWithSource(context, 1, &header_source, nullptr,
                                       &errorcode);
    EXPECT_TRUE(header);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clCompileProgram(header, 1, &device, nullptr, 0, nullptr,
                                    nullptr, nullptr, nullptr));

    const char *header_name = "test";
    const char *program_source = "#include <test>";
    program_with_header = clCreateProgramWithSource(context, 1, &program_source,
                                                    nullptr, &errorcode);
    EXPECT_TRUE(program_with_header);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clCompileProgram(program_with_header, 1, &device, nullptr, 1,
                                    &header, &header_name, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (header) {
      EXPECT_SUCCESS(clReleaseProgram(header));
    }
    if (program_with_header) {
      EXPECT_SUCCESS(clReleaseProgram(program_with_header));
    }
    ContextTest::TearDown();
  }

  const char *source = nullptr;
  const char *header_source = nullptr;
  cl_program program = nullptr;
  cl_program header = nullptr;
  cl_program program_with_header = nullptr;
};

class clCompileLinkEmbeddedHeaderPrototype
    : public clCompileLinkEmbeddedHeader {
 protected:
  clCompileLinkEmbeddedHeaderPrototype()
      : clCompileLinkEmbeddedHeader(
            "extern int test(void);\n"
            "void kernel foo(global int * a) {*a = test();}",
            "int test(void) { return 42; }") {}
};

TEST_F(clCompileLinkEmbeddedHeaderPrototype, Default) {
  cl_int errorcode = CL_SUCCESS;
  cl_program link_input[] = {program, program_with_header};
  cl_program linked = clLinkProgram(context, 1, &device, nullptr, 2, link_input,
                                    nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linked);
  EXPECT_SUCCESS(errorcode);
  EXPECT_SUCCESS(clReleaseProgram(linked));
}

// Note that this test differs from clCompileLinkEmbeddedHeaderPrototype
// because the 'test' function declaration is not a prototype, i.e. it does not
// completely define function i.e. could have zero or more arguments (in
// clCompileLinkEmbeddedHeaderPrototype it has exactly zero arguments).  This
// is a trickier test for OpenCL implementations because it is necessary to get
// the calling convention correct even when we don't know the function
// prototype.
class clCompileLinkEmbeddedHeaderDeclaration
    : public clCompileLinkEmbeddedHeader {
 protected:
  clCompileLinkEmbeddedHeaderDeclaration()
      : clCompileLinkEmbeddedHeader(
            "extern int test();\n"
            "void kernel foo(global int * a) {*a = test();}",
            "int test() { return 42; }") {}
};

// Disabled as this currently causes issues w.r.t. calling conventions, see
// Redmine issue #5295.
TEST_F(clCompileLinkEmbeddedHeaderDeclaration, DISABLED_Default) {
  cl_int errorcode = CL_SUCCESS;
  cl_program link_input[] = {program, program_with_header};
  cl_program linked = clLinkProgram(context, 1, &device, nullptr, 2, link_input,
                                    nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linked);
  EXPECT_SUCCESS(errorcode);
  EXPECT_SUCCESS(clReleaseProgram(linked));
}

struct clCompileAndLinkImageKernelGoodTest
    : ucl::ContextTest,
      testing::WithParamInterface<const char *> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!(getDeviceImageSupport() && getDeviceCompilerAvailable())) {
      GTEST_SKIP();
    }
  }

  void SetUpProgram(const char *source) {
    cl_int errorcode = CL_SUCCESS;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override { ContextTest::TearDown(); }

  cl_program program = nullptr;
};

TEST_P(clCompileAndLinkImageKernelGoodTest, ImageArgument) {
  this->SetUpProgram(GetParam());
  ASSERT_SUCCESS(clCompileProgram(program, 1, &device, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
  cl_int status = CL_SUCCESS;
  cl_program linkedProgram = clLinkProgram(context, 0, nullptr, "", 1, &program,
                                           nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  EXPECT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

INSTANTIATE_TEST_CASE_P(
    clCompileAndLinkImageKernel, clCompileAndLinkImageKernelGoodTest,
    ::testing::Values(
        "void __kernel image_test(__read_only image2d_t input, __write_only "
        "                         image2d_t output, __global int* buffer) "
        "{}\n",

        "void __kernel image_test(__read_only image3d_t input, __write_only "
        "                         image3d_t output, __global int* buffer) "
        "{}\n",

        "void __kernel image_test(__read_only image2d_array_t input, "
        "          __write_only image2d_array_t output, __global int* buffer) "
        "{}\n",

        "void __kernel image_test(__read_only image1d_t input, __write_only "
        "                         image1d_t output, __global int* buffer) "
        "{}\n",

        "void __kernel image_test(__read_only image1d_buffer_t input, "
        "         __write_only image1d_buffer_t output, __global int* buffer) "
        "{}\n",

        "void __kernel image_test(__read_only image2d_t input, __write_only "
        "                         image2d_t output, sampler_t sampler) "
        "{}\n"));
