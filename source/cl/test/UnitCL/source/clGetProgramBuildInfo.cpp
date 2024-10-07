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

#include "Common.h"

class clGetProgramBuildInfoGoodTest : public ucl::ContextTest {
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
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

class clGetProgramBuildInfoBadTest : public ucl::ContextTest {
 protected:
  cl_program program = nullptr;
};

TEST_F(clGetProgramBuildInfoGoodTest, BadProgram) {
  ASSERT_EQ(CL_INVALID_PROGRAM,
            clGetProgramBuildInfo(nullptr, device, 0, 0, nullptr, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, BadDevice) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_DEVICE,
      clGetProgramBuildInfo(program, nullptr, 0, 0, nullptr, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, BadPointers) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramBuildInfo(program, device, 0, 0, nullptr, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildStatusSizeRet) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_build_status), size);
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildStatusBadSize) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS, 0,
                            &status, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildStatusSuccess) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_SUCCESS, status);
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildStatusBinarySuccess) {
  size_t binary_size = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                  sizeof(size_t), &binary_size, nullptr));

  const auto *program_binary = new unsigned char[binary_size];

  EXPECT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *),
                       static_cast<void *>(&program_binary), nullptr));

  cl_int binary_status = CL_INVALID_BINARY;
  cl_int error = CL_INVALID_BINARY;
  auto binary_program =
      clCreateProgramWithBinary(context, 1, &device, &binary_size,
                                &program_binary, &binary_status, &error);
  EXPECT_SUCCESS(binary_status);
  EXPECT_SUCCESS(error);

  delete[] program_binary;

  size_t size = 0;
  ASSERT_SUCCESS(clGetProgramBuildInfo(
      binary_program, device, CL_PROGRAM_BUILD_STATUS, 0, nullptr, &size));
  cl_build_status status = CL_BUILD_ERROR;
  ASSERT_SUCCESS(clGetProgramBuildInfo(
      binary_program, device, CL_PROGRAM_BUILD_STATUS, size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_SUCCESS, status);

  ASSERT_SUCCESS(clReleaseProgram(binary_program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBuildStatusNone) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, not source.
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_NONE, status);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBuildStatusIntermediate) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_SUCCESS, status);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBuildStatusSource) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, not source.
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_NONE, status);
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBuildStatusFailure) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source = "bad kernel";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_ERROR, status);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                       nullptr, &size));
  UCL::Buffer<char> log(size);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                       size, log, nullptr));
  ASSERT_TRUE(strstr(log, "error: unknown type name 'bad'"));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramMissingFunction) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "void some_func(global int * a, global int * b);"
      "void kernel foo(global int * a, global int * b) { some_func(a, b); }";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  ASSERT_EQ_ERRCODE(
      CL_BUILD_PROGRAM_FAILURE,
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       0, nullptr, &size));
  cl_build_status status;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_STATUS,
                                       size, &status, nullptr));
  ASSERT_EQ_ERRCODE(CL_BUILD_ERROR, status);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                       nullptr, &size));
  UCL::Buffer<char> log(size);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                       size, log, nullptr));
  ASSERT_TRUE(strstr(
      log, "Could not find a definition for external function 'some_func'"));
  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildOptionsSizeRet) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(
      program, device, CL_PROGRAM_BUILD_OPTIONS, 0, nullptr, &size));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildOptionsBadSize) {
  size_t size;
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, "-create-library", nullptr, nullptr));
  ASSERT_SUCCESS(clGetProgramBuildInfo(
      program, device, CL_PROGRAM_BUILD_OPTIONS, 0, nullptr, &size));
  UCL::Buffer<char> options(size);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_OPTIONS, 0,
                            options, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildOptionsDefault) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection doesn't propogate build options.
  }
  const char *input_options = "-cl-opt-disable -w";
  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, input_options, nullptr, nullptr));
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(
      program, device, CL_PROGRAM_BUILD_OPTIONS, 0, nullptr, &size));
  UCL::Buffer<char> output_options(size);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device,
                                       CL_PROGRAM_BUILD_OPTIONS, size,
                                       output_options, nullptr));
  // Ensure that the flags we pass in are the same as the ones we get out.
  ASSERT_TRUE(size ? size == strlen(output_options) + 1 : true);
  ASSERT_STREQ(input_options, output_options.data());
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildLogSizeRet) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                       nullptr, &size));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildLogBadSize) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                       nullptr, &size));
  UCL::Buffer<char> log(size);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                          0, log, nullptr));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBuildLogDefault) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                       nullptr, &size));
  UCL::Buffer<char> log(size);
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                       size, log, nullptr));
  ASSERT_TRUE(size ? size == strlen(log) + 1 : true);
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBinaryTypeSizeRet) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_program_binary_type), size);
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBinaryTypeBadSize) {
  cl_program_binary_type binaryType;
  ASSERT_EQ(CL_INVALID_VALUE,
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE, 0,
                                  &binaryType, nullptr));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBinaryTypeCompiled) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
  }
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                  nullptr, nullptr, nullptr));
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       0, nullptr, &size));
  cl_program_binary_type binaryType;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       size, &binaryType, nullptr));
  const cl_program_binary_type expect = CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
  ASSERT_EQ(expect, binaryType);

  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoBadTest, ProgramBinaryTypeNone) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs with executable status.
  }
  cl_int errorcode;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
  program = clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       0, nullptr, &size));
  cl_program_binary_type binaryType;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       size, &binaryType, nullptr));
  const cl_program_binary_type expect = CL_PROGRAM_BINARY_TYPE_NONE;
  ASSERT_EQ(expect, binaryType);

  ASSERT_SUCCESS(clReleaseProgram(program));
}

TEST_F(clGetProgramBuildInfoGoodTest, ProgramBinaryTypeExecutable) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       0, nullptr, &size));
  cl_program_binary_type binaryType;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       size, &binaryType, nullptr));
  const cl_program_binary_type expect = CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
  ASSERT_EQ(expect, binaryType);
}
#if defined(CL_VERSION_3_0)
class clGetProgramBuildInfoTestScalarQueryOpenCL30
    : public clGetProgramBuildInfoGoodTest,
      public testing::WithParamInterface<std::tuple<size_t, int>> {
 protected:
  void SetUp() override {
    clGetProgramBuildInfoGoodTest::SetUp();
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetProgramBuildInfoTestScalarQueryOpenCL30, CheckSizeQuerySucceeds) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(clGetProgramBuildInfo(program, device, query_enum_value, 0,
                                       nullptr, &size));
}

TEST_P(clGetProgramBuildInfoTestScalarQueryOpenCL30, CheckSizeQueryIsCorrect) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, query_enum_value, 0,
                                       nullptr, &size));
  // Get the correct size of the query.
  auto value_size_in_bytes = std::get<0>(GetParam());
  // Check the queried value is correct.
  EXPECT_EQ(size, value_size_in_bytes);
}

TEST_P(clGetProgramBuildInfoTestScalarQueryOpenCL30, CheckQuerySucceeds) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetProgramBuildInfo(program, device, query_enum_value,
                                       value_buffer.size(), value_buffer.data(),
                                       nullptr));
}

TEST_P(clGetProgramBuildInfoTestScalarQueryOpenCL30,
       CheckIncorrectSizeQueryFails) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramBuildInfo(program, device, query_enum_value,
                                          value_buffer.size() - 1,
                                          value_buffer.data(), nullptr));
}

INSTANTIATE_TEST_CASE_P(
    BuildProgramQuery, clGetProgramBuildInfoTestScalarQueryOpenCL30,
    testing::Values(std::make_tuple(
        sizeof(size_t), CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE)),
    [](const testing::TestParamInfo<
        clGetProgramBuildInfoTestScalarQueryOpenCL30::ParamType> &info) {
      return UCL::programBuildQueryToString(std::get<1>(info.param));
    });
#endif
