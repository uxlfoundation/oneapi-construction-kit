// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clLinkProgramGoodTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                    nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
};

TEST_F(clLinkProgramGoodTest, Default) {
  cl_int errorcode;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, nullptr, 1, &program, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

TEST_F(clLinkProgramGoodTest, DefaultUseProgram) {
  cl_int status;
  cl_program linkedProgram = clLinkProgram(context, 0, nullptr, nullptr, 1,
                                           &program, nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(status);

  cl_kernel kernel = clCreateKernel(linkedProgram, "foo", &status);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(status);

  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

TEST_F(clLinkProgramGoodTest, AllDevices) {
  cl_int errorcode;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, nullptr, 1, &program, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

TEST_F(clLinkProgramGoodTest, BadContext) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(nullptr, 0, nullptr, nullptr, 1, &program, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errorcode);
}

TEST_F(clLinkProgramGoodTest, NoDeviceListWithDevices) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 1, nullptr, nullptr, 1, &program, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, DeviceListWithNoDevices) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, &device, nullptr, 1, &program, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, NoProgramListWithNoPrograms) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 0, nullptr, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, NoProgramListWithPrograms) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 1, nullptr, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, ProgramListWithNoPrograms) {
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 0, &program, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, InvalidProgram) {
  cl_program programs[] = {program, nullptr};
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 2, programs, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM, errorcode);
}

TEST_F(clLinkProgramGoodTest, NullCallbackWithData) {
  const char *something = "foo";
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 1, &program, nullptr,
                             const_cast<char *>(something), &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

TEST_F(clLinkProgramGoodTest, InvalidDevice) {
  cl_device_id devices[] = {device, nullptr};
  cl_int errorcode;
  EXPECT_FALSE(clLinkProgram(context, 2, devices, nullptr, 1, &program, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_DEVICE, errorcode);
}

TEST_F(clLinkProgramGoodTest, UncompiledProgramInList) {
  cl_int errorcode;

  cl_program otherProgram =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);

  cl_program programs[] = {program, otherProgram};
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 2, programs, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, errorcode);

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clLinkProgramGoodTest, LinkFailureDuplicateKernels) {
  cl_int errorcode;

  cl_program otherProgram =
      clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clCompileProgram(otherProgram, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));

  cl_program programs[] = {program, otherProgram};
  EXPECT_FALSE(clLinkProgram(context, 0, nullptr, nullptr, 2, programs, nullptr,
                             nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_LINK_PROGRAM_FAILURE, errorcode);

  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clLinkProgramGoodTest, Callback) {
  struct UserData {
    int data;
    cl_event event;
    cl_int status;
    cl_program program;
  };

  struct Helper {
    static void CL_CALLBACK callback(cl_program program, void *user_data) {
      UserData *const actualUserData = static_cast<UserData *>(user_data);
      actualUserData->data = 42;
      actualUserData->status =
          clSetUserEventStatus(actualUserData->event, CL_COMPLETE);
      actualUserData->program = program;
    }
  };

  cl_int userEventStatus = !CL_SUCCESS;
  cl_event event = clCreateUserEvent(context, &userEventStatus);
  EXPECT_TRUE(event);
  ASSERT_SUCCESS(userEventStatus);

  UserData userData;
  userData.data = 0;
  userData.event = event;
  userData.status = !CL_SUCCESS;
  userData.program = program;

  cl_int linkProgramStatus = !CL_SUCCESS;
  cl_program linkProgram =
      clLinkProgram(context, 0, nullptr, nullptr, 1, &program, Helper::callback,
                    &userData, &linkProgramStatus);
  EXPECT_TRUE(linkProgram);
  ASSERT_SUCCESS(linkProgramStatus);

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_EQ(42, userData.data);

  ASSERT_SUCCESS(userData.status);

  ASSERT_EQ(linkProgram, userData.program);

  ASSERT_SUCCESS(clReleaseEvent(event));

  ASSERT_SUCCESS(clReleaseProgram(linkProgram));
}

TEST_F(clLinkProgramGoodTest, CreateLibraryThenGetBadKernel) {
  cl_int status;
  cl_program linkedProgram =
      clLinkProgram(context, 0, nullptr, "-create-library", 1, &program,
                    nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(status);

  EXPECT_FALSE(clCreateKernel(linkedProgram, "foo", &status));
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM_EXECUTABLE, status);

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

TEST_F(clLinkProgramGoodTest, CreateLibraryAndLinkAgainstIt) {
  cl_int status;
  cl_program linkedProgram =
      clLinkProgram(context, 0, nullptr, "-create-library", 1, &program,
                    nullptr, nullptr, &status);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(status);

  const char *otherSource = "int bar(int b) { return b; }";
  cl_program otherProgram =
      clCreateProgramWithSource(context, 1, &otherSource, nullptr, &status);
  EXPECT_TRUE(otherProgram);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clCompileProgram(otherProgram, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));

  cl_program programs[] = {otherProgram, linkedProgram};
  cl_program finalProgram = clLinkProgram(context, 0, nullptr, nullptr, 2,
                                          programs, nullptr, nullptr, &status);
  EXPECT_TRUE(finalProgram);
  ASSERT_SUCCESS(status);

  ASSERT_SUCCESS(clReleaseProgram(finalProgram));
  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
  ASSERT_SUCCESS(clReleaseProgram(otherProgram));
}

TEST_F(clLinkProgramGoodTest, LinkProgramThenTryCompile) {
  cl_int errorcode;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, nullptr, 1, &program, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCompileProgram(linkedProgram, 0, nullptr, nullptr, 0,
                                     nullptr, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

TEST_F(clLinkProgramGoodTest, LinkProgramThenTryBuild) {
  cl_int errorcode;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, nullptr, 1, &program, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(linkedProgram);
  ASSERT_SUCCESS(errorcode);

  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clBuildProgram(linkedProgram, 0, nullptr, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseProgram(linkedProgram));
}

class clLinkProgramCompilerlessTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
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
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
};

TEST_F(clLinkProgramCompilerlessTest, CompilerUnavailable) {
  cl_int errorcode;
  cl_program linkedProgram = clLinkProgram(
      context, 0, nullptr, nullptr, 1, &program, nullptr, nullptr, &errorcode);
  EXPECT_FALSE(linkedProgram);
  ASSERT_EQ_ERRCODE(CL_LINKER_NOT_AVAILABLE, errorcode);
}

// CL_LINK_PROGRAM_FAILURE if there is a failure to link the compiled binaries
// and/or libraries.

typedef std::pair<cl_int, const char *> Pair;

struct LinkOptionsTest : ucl::ContextTest, testing::WithParamInterface<Pair> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't link.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "kernel void foo(global int *a, global int *b) { *a = *b; }";
    cl_int status;
    source_program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clCompileProgram(source_program, 0, nullptr, "", 0, nullptr,
                                    nullptr, nullptr, nullptr));
  }

  void TearDown() {
    if (linked_program) {
      EXPECT_SUCCESS(clReleaseProgram(linked_program));
    }
    if (source_program) {
      EXPECT_SUCCESS(clReleaseProgram(source_program));
    }
    ContextTest::TearDown();
  }

  cl_program source_program = nullptr;
  cl_program linked_program = nullptr;
};

TEST_P(LinkOptionsTest, LinkWithOption) {
  cl_int status;
  linked_program = clLinkProgram(context, 0, nullptr, GetParam().second, 1,
                                 &source_program, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(GetParam().first, status)
      << "options: " << GetParam().second;
}

INSTANTIATE_TEST_CASE_P(
    clLinkProgram, LinkOptionsTest,
    ::testing::Values(
        Pair(CL_SUCCESS, "-create-library"),
        Pair(CL_SUCCESS, "-create-library -enable-link-options"),
        Pair(CL_SUCCESS, "-enable-link-options -create-library"),
        Pair(CL_INVALID_LINKER_OPTIONS, "-enable-link-options"),
        Pair(CL_SUCCESS, "-cl-denorms-are-zero"),
        Pair(CL_SUCCESS, "-cl-no-signed-zeros"),
        Pair(CL_SUCCESS, "-cl-unsafe-math-optimizations"),
        Pair(CL_SUCCESS, "-cl-finite-math-only"),
        Pair(CL_SUCCESS, "-cl-fast-relaxed-math"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-denorms-are-zero"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-no-signed-zeros"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-unsafe-math-optimizations"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-finite-math-only"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-fast-relaxed-math"),
        Pair(CL_INVALID_LINKER_OPTIONS, "-create-library -cl-denorms-are-zero"),
        Pair(CL_INVALID_LINKER_OPTIONS, "-create-library -cl-no-signed-zeros"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-create-library -cl-unsafe-math-optimizations"),
        Pair(CL_INVALID_LINKER_OPTIONS, "-create-library -cl-finite-math-only"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-create-library -cl-fast-relaxed-math")));

struct LinkLibraryOptionsTest : ucl::ContextTest,
                                testing::WithParamInterface<Pair> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't link.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int status;
    const char *source =
        "kernel void foo(global int *a, global int *b) { *a = *b; }";
    source_program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &status);
    ASSERT_SUCCESS(clCompileProgram(source_program, 0, nullptr, "", 0, nullptr,
                                    nullptr, nullptr, nullptr));
  }

  void TearDown() {
    if (linked_program) {
      EXPECT_SUCCESS(clReleaseProgram(linked_program));
    }
    if (library_program) {
      EXPECT_SUCCESS(clReleaseProgram(library_program));
    }
    if (source_program) {
      EXPECT_SUCCESS(clReleaseProgram(source_program));
    }
    ContextTest::TearDown();
  }

  cl_program source_program = nullptr;
  cl_program library_program = nullptr;
  cl_program linked_program = nullptr;
};

TEST_P(LinkLibraryOptionsTest, LinkLibraryWithGoodOption) {
  cl_int status;
  library_program =
      clLinkProgram(context, 0, nullptr, "-create-library -enable-link-options",
                    1, &source_program, nullptr, nullptr, &status);
  ASSERT_EQ(CL_SUCCESS, status);
  linked_program = clLinkProgram(context, 0, nullptr, GetParam().second, 1,
                                 &library_program, nullptr, nullptr, &status);
  ASSERT_EQ(GetParam().first, status);
}

INSTANTIATE_TEST_CASE_P(
    clLinkProgram, LinkLibraryOptionsTest,
    ::testing::Values(
        Pair(CL_INVALID_LINKER_OPTIONS, "-enable-link-options"),
        Pair(CL_SUCCESS, "-cl-denorms-are-zero"),
        Pair(CL_SUCCESS, "-cl-no-signed-zeros"),
        Pair(CL_SUCCESS, "-cl-unsafe-math-optimizations"),
        Pair(CL_SUCCESS, "-cl-finite-math-only"),
        Pair(CL_SUCCESS, "-cl-fast-relaxed-math"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-denorms-are-zero"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-no-signed-zeros"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-unsafe-math-optimizations"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-finite-math-only"),
        Pair(CL_INVALID_LINKER_OPTIONS,
             "-enable-link-options -cl-fast-relaxed-math")));

using clLinkProgramTest = ucl::ContextTest;
TEST_F(clLinkProgramTest, ExternConstantDecl) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int error;
  const char *source_extern_constant_use = R"(
extern constant int extern_constant_int;
void kernel foo(global int *buf) {
  int i = get_global_id(0);
  buf[i] = extern_constant_int;
}
)";
  const size_t source_extern_constant_use_size =
      strlen(source_extern_constant_use);
  cl_program program_extern_constant_use =
      clCreateProgramWithSource(context, 1, &source_extern_constant_use,
                                &source_extern_constant_use_size, &error);
  error = clCompileProgram(program_extern_constant_use, 1, &device, "", 0,
                           nullptr, nullptr, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  const char *source_extern_constant_def = R"(
constant int extern_constant_int = 42;
)";
  const size_t source_extern_constant_def_size =
      strlen(source_extern_constant_def);
  cl_program program_extern_constant_def =
      clCreateProgramWithSource(context, 1, &source_extern_constant_def,
                                &source_extern_constant_def_size, &error);
  error = clCompileProgram(program_extern_constant_def, 1, &device, "", 0,
                           nullptr, nullptr, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  cl_program programs[2] = {program_extern_constant_def,
                            program_extern_constant_use};
  cl_program linked_program = clLinkProgram(context, 1, &device, "", 2,
                                            programs, nullptr, nullptr, &error);
  EXPECT_NE(nullptr, linked_program);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseProgram(program_extern_constant_use));
  ASSERT_SUCCESS(clReleaseProgram(program_extern_constant_def));
  ASSERT_SUCCESS(clReleaseProgram(linked_program));
}

TEST_F(clLinkProgramTest, ExternFunctionPrototype) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int error;
  const char *source_extern_function_use = R"(
extern int extern_function_int(void);
void kernel foo(global int *buf) {
  int i = get_global_id(0);
  buf[i] = extern_function_int();
}
)";
  const size_t source_extern_function_use_size =
      strlen(source_extern_function_use);
  cl_program program_extern_function_use =
      clCreateProgramWithSource(context, 1, &source_extern_function_use,
                                &source_extern_function_use_size, &error);
  error = clCompileProgram(program_extern_function_use, 1, &device, "", 0,
                           nullptr, nullptr, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  const char *source_extern_function_def = R"(
int extern_function_int() { return 42;})";
  const size_t source_extern_function_def_size =
      strlen(source_extern_function_def);
  cl_program program_extern_function_def =
      clCreateProgramWithSource(context, 1, &source_extern_function_def,
                                &source_extern_function_def_size, &error);
  error = clCompileProgram(program_extern_function_def, 1, &device, "", 0,
                           nullptr, nullptr, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  cl_program programs[2] = {program_extern_function_def,
                            program_extern_function_use};
  cl_program linked_program = clLinkProgram(context, 1, &device, "", 2,
                                            programs, nullptr, nullptr, &error);
  EXPECT_NE(nullptr, linked_program);
  ASSERT_SUCCESS(error);
  ASSERT_SUCCESS(clReleaseProgram(program_extern_function_use));
  ASSERT_SUCCESS(clReleaseProgram(program_extern_function_def));
  ASSERT_SUCCESS(clReleaseProgram(linked_program));
}
