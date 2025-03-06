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

#include <algorithm>
#include <cstring>
#include <sstream>
#include <type_traits>

#include "Common.h"

template <typename T>
class KernelArgumentTypesTest : public ucl::CommandQueueTest {
 public:
  using TestType = T;

  // We override operator new to correctly align Heap allocations of this
  // class. The alignof(T) may be up to 64 bytes, but C++ does not have
  // primitive types that large and thus the default new does not handle
  // alignments as large as 64 bytes.
  void *operator new(size_t size) {
#ifdef _MSC_VER
    // MSVC has not implemented alignof, but they have the exact same
    // functionality with __alignof, so use that instead.  Also, MSVC is not
    // able to instantiate KernelArgumentTypesTest<T> here, so we just use the
    // alignment of T or cl_ulong, whichever is greater.
    uint32_t align = (uint32_t)std::max(__alignof(cl_ulong), __alignof(T));
#else
    const uint32_t align = alignof(KernelArgumentTypesTest<T>);
#endif
    return UCL::aligned_alloc(align, size);
  }

  // We override operator delete to match the override of operator new.
  void operator delete(void *p) { UCL::aligned_free(p); }

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    doubleFPConfig = getDeviceDoubleFpConfig();
  }

  bool isDoubleType() const {
    return std::is_same_v<typename TestType::value_type, cl_double>;
  }

  cl_device_fp_config doubleFPConfig = 0;
  TestType in = {};
  TestType out = {};
};

#if !defined(__clang_analyzer__)
typedef ::testing::Types<
    ucl::Char, ucl::Char2, ucl::Char3, ucl::Char4, ucl::Char8, ucl::Char16,
    ucl::UChar, ucl::UChar2, ucl::UChar3, ucl::UChar4, ucl::UChar8,
    ucl::UChar16, ucl::Short, ucl::Short2, ucl::Short3, ucl::Short4,
    ucl::Short8, ucl::Short16, ucl::UShort, ucl::UShort2, ucl::UShort3,
    ucl::UShort4, ucl::UShort8, ucl::UShort16, ucl::Int, ucl::Int2, ucl::Int3,
    ucl::Int4, ucl::Int8, ucl::Int16, ucl::UInt, ucl::UInt2, ucl::UInt3,
    ucl::UInt4, ucl::UInt8, ucl::UInt16, ucl::Long, ucl::Long2, ucl::Long3,
    ucl::Long4, ucl::Long8, ucl::Long16, ucl::ULong, ucl::ULong2, ucl::ULong3,
    ucl::ULong4, ucl::ULong8, ucl::ULong16, ucl::Float, ucl::Float2,
    ucl::Float3, ucl::Float4, ucl::Float8, ucl::Float16, ucl::Double,
    ucl::Double2, ucl::Double3, ucl::Double4, ucl::Double8, ucl::Double16>
    TestTypes;
#else
// Reduce the number of types to test if running clang analyzer (or
// clang-tidy), they'll all result in basically the same code but it takes a
// long time to analyze all of them.
typedef ::testing::Types<ucl::Int> TestTypes;
#endif

TYPED_TEST_SUITE(KernelArgumentTypesTest, TestTypes, );

TYPED_TEST(KernelArgumentTypesTest, ByGlobalPointer) {
  if (TestFixture::isDoubleType() && TestFixture::doubleFPConfig == 0) {
    GTEST_SKIP();
  }
  if (!this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int errorcode;

  cl_mem inMem =
      clCreateBuffer(this->context, 0, sizeof(typename TestFixture::TestType),
                     nullptr, &errorcode);
  ASSERT_TRUE(inMem);
  ASSERT_SUCCESS(errorcode);
  cl_mem outMem =
      clCreateBuffer(this->context, 0, sizeof(typename TestFixture::TestType),
                     nullptr, &errorcode);
  ASSERT_TRUE(outMem);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clEnqueueWriteBuffer(this->command_queue, inMem, CL_TRUE, 0,
                                      sizeof(typename TestFixture::TestType),
                                      &(this->in), 0, nullptr, nullptr));

  const std::string typeName = TestFixture::TestType::source_name();
  const char *kernelString[5] = {"void kernel foo(global ", typeName.c_str(),
                                 " * a, global ", typeName.c_str(),
                                 " * b) {*a = *b;}"};

  cl_program program = clCreateProgramWithSource(this->context, 5, kernelString,
                                                 nullptr, &errorcode);
  ASSERT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errorcode);

  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&outMem)));
  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 1, sizeof(cl_mem), static_cast<void *>(&inMem)));

  EXPECT_SUCCESS(
      clEnqueueTask(this->command_queue, kernel, 0, nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(this->command_queue, outMem, CL_TRUE, 0,
                                     sizeof(typename TestFixture::TestType),
                                     &(this->out), 0, nullptr, nullptr));

  EXPECT_EQ(this->in, this->out);

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));

  EXPECT_SUCCESS(clReleaseMemObject(inMem));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
}

TYPED_TEST(KernelArgumentTypesTest, ByConstantPointer) {
  if (TestFixture::isDoubleType() && TestFixture::doubleFPConfig == 0) {
    GTEST_SKIP();
  }
  if (!this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int errorcode;

  cl_mem inMem =
      clCreateBuffer(this->context, 0, sizeof(typename TestFixture::TestType),
                     nullptr, &errorcode);
  ASSERT_TRUE(inMem);
  ASSERT_SUCCESS(errorcode);

  cl_mem outMem =
      clCreateBuffer(this->context, 0, sizeof(typename TestFixture::TestType),
                     nullptr, &errorcode);
  ASSERT_TRUE(outMem);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clEnqueueWriteBuffer(this->command_queue, inMem, CL_TRUE, 0,
                                      sizeof(typename TestFixture::TestType),
                                      &(this->in), 0, nullptr, nullptr));

  const std::string typeName = TestFixture::TestType::source_name();
  const char *kernelString[5] = {"void kernel foo(global ", typeName.c_str(),
                                 " * a, constant ", typeName.c_str(),
                                 " * b) {*a = *b;}"};

  cl_program program = clCreateProgramWithSource(this->context, 5, kernelString,
                                                 nullptr, &errorcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);

  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&outMem)));
  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 1, sizeof(cl_mem), static_cast<void *>(&inMem)));

  EXPECT_SUCCESS(
      clEnqueueTask(this->command_queue, kernel, 0, nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(this->command_queue, outMem, CL_TRUE, 0,
                                     sizeof(typename TestFixture::TestType),
                                     &(this->out), 0, nullptr, nullptr));

  EXPECT_EQ(this->in, this->out);

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));

  EXPECT_SUCCESS(clReleaseMemObject(inMem));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
}

TYPED_TEST(KernelArgumentTypesTest, ByValue) {
  if (TestFixture::isDoubleType() && TestFixture::doubleFPConfig == 0) {
    GTEST_SKIP();
  }
  if (!this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int status;

  typedef typename TestFixture::TestType TestType;

  // create input & output buffers
  cl_mem outBuffer =
      clCreateBuffer(this->context, 0, sizeof(TestType), nullptr, &status);
  EXPECT_TRUE(outBuffer);
  ASSERT_SUCCESS(status);

  // create program
  const std::string typeName = TestType::source_name();
  const char *source[] = {"kernel void foo(global ", typeName.c_str(),
                          " *out, ", typeName.c_str(),
                          " value) { *out = value; }"};
  cl_program program =
      clCreateProgramWithSource(this->context, 5, source, nullptr, &status);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(status);

  // build program
  EXPECT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  // create kernel
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(status);

  // set kernel args
  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&outBuffer)));

  // Redmine #5143: set this to some actual value
  static_assert(std::is_trivial_v<TestType>, "TestType must be a trivial type");
  TestType value(42);

  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(TestType), &value));

  // enqueue task
  EXPECT_SUCCESS(
      clEnqueueTask(this->command_queue, kernel, 0, nullptr, nullptr));

  // read output buffer
  TestType result;
  EXPECT_SUCCESS(clEnqueueReadBuffer(this->command_queue, outBuffer, CL_TRUE, 0,
                                     sizeof(TestType), &result, 0, nullptr,
                                     nullptr));

  EXPECT_EQ(value, result);

  // release resources
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseMemObject(outBuffer));
}

TYPED_TEST(KernelArgumentTypesTest, NullByValue) {
  if (TestFixture::isDoubleType() && TestFixture::doubleFPConfig == 0) {
    GTEST_SKIP();
  }
  if (!this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int status;

  typedef typename TestFixture::TestType TestType;

  // create program
  const std::string typeName = TestType::source_name();
  const char *source[] = {"kernel void foo(global ", typeName.c_str(),
                          " *out, ", typeName.c_str(),
                          " value) { *out = value; }"};
  cl_program program =
      clCreateProgramWithSource(this->context, 5, source, nullptr, &status);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(status);

  // build program
  EXPECT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  // create kernel
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(status);

  EXPECT_EQ_ERRCODE(CL_INVALID_ARG_VALUE,
                    clSetKernelArg(kernel, 1, sizeof(TestType), nullptr));

  // release resources
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

TYPED_TEST(KernelArgumentTypesTest, ByLocalPointer) {
  if (TestFixture::isDoubleType() && TestFixture::doubleFPConfig == 0) {
    GTEST_SKIP();
  }
  if (!this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int errorcode;

  cl_mem outMem =
      clCreateBuffer(this->context, 0, sizeof(typename TestFixture::TestType),
                     nullptr, &errorcode);
  EXPECT_TRUE(outMem);
  ASSERT_SUCCESS(errorcode);

  const std::string typeName = TestFixture::TestType::source_name();
  const char *kernelString[5] = {"void kernel foo(global ", typeName.c_str(),
                                 " * a, local ", typeName.c_str(),
                                 " * b) {*a = *b;}"};

  cl_program program = clCreateProgramWithSource(this->context, 5, kernelString,
                                                 nullptr, &errorcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);
  EXPECT_TRUE(kernel);
  EXPECT_SUCCESS(errorcode);

  EXPECT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&outMem)));
  EXPECT_EQ(CL_SUCCESS,
            clSetKernelArg(kernel, 1, sizeof(typename TestFixture::TestType),
                           nullptr));

  EXPECT_SUCCESS(
      clEnqueueTask(this->command_queue, kernel, 0, nullptr, nullptr));

  EXPECT_SUCCESS(clEnqueueReadBuffer(this->command_queue, outMem, CL_TRUE, 0,
                                     sizeof(typename TestFixture::TestType),
                                     &(this->out), 0, nullptr, nullptr));

  // can't actually verify that local went ok as we cannot assume anything about
  // the local memory!

  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
  EXPECT_SUCCESS(clReleaseMemObject(outMem));
}
