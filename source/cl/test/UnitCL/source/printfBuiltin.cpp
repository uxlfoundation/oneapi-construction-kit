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

struct printfBuiltinTest : ucl::CommandQueueTest,
                           testing::WithParamInterface<const char *> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source = GetParam();
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    build_error =
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    EXPECT_SUCCESS(build_error);
    if (build_error) {
      // NOTE: The program failed to build therefore we can't not run the test,
      // instead display the build log.
      size_t log_size;
      ASSERT_SUCCESS(clGetProgramBuildInfo(
          program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size));
      if (log_size) {
        UCL::vector<char> log(log_size);
        ASSERT_SUCCESS(clGetProgramBuildInfo(program, device,
                                             CL_PROGRAM_BUILD_LOG, log.size(),
                                             log.data(), nullptr));
        printf("build log: %s\n", log.data());
      } else {
        printf("build log is empty\n");
      }
      FAIL();
      return;
    }
    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);
    inMem = clCreateBuffer(context, 0, sizeof(cl_int), nullptr, &errorcode);
    ASSERT_TRUE(inMem);
    ASSERT_SUCCESS(errorcode);
    outMem = clCreateBuffer(context, 0, sizeof(cl_int), nullptr, &errorcode);
    ASSERT_TRUE(outMem);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outMem));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inMem));
    buffer = 42;
  }

  void TearDown() override {
    if (outMem) {
      EXPECT_SUCCESS(clReleaseMemObject(outMem));
    }
    if (inMem) {
      EXPECT_SUCCESS(clReleaseMemObject(inMem));
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
  cl_int build_error = 0;
  cl_kernel kernel = nullptr;
  cl_mem inMem = nullptr;
  cl_mem outMem = nullptr;
  cl_int buffer = 0;
};

class printfBuiltinValidTest : public printfBuiltinTest {};

class printfBuiltinInvalidTest : public printfBuiltinTest {};

static const char *valid_kernels[] = {
    "void kernel foo(global int * a, global int * b)"
    "{"
    " *a = printf(\"0x%08x\\n\", *b);"
    "}",
    "void kernel foo(global int * a, global int * b)"
    "{"
    " constant char * format = \"0x%08x\\n\";"
    " *a = printf(format, *b);"
    "}",
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    uchar2 tmp = (uchar2)(0xFA, 0xFB);\n"
    "    *a = printf(\"%#v2hhx\\n\", tmp);\n"
    "}\n",
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    float8 tmp = (float8)(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, "
    "8.0f);\n"
    "    *a = printf(\"%v8hlf\\n\", tmp);\n"
    "}\n",
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    float4 tmp = (float4)(1.0f, 2.0f, 3.0f, 4.0f);\n"
    "    *a = printf(\"%v4hla\\n\", tmp);\n"
    "}\n",
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%s\\n\", \"hello\");\n"
    "}\n",
};

static const char *invalid_kernels[] = {
    // 'l' length modifier must not be used with 'c'.
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%lc\\n\", 'x');\n"
    "}\n",
    // 'l' length modifier must not be used with 's'.
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%ls\\n\", \"hello\");\n"
    "}\n",
    // 'n' specifier is reserved by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%n\\n\", a);\n"
    "}\n",
    // 'll' length modifier is not supported by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%llx\\n\", *b);\n"
    "}\n",
    // 'L' length modifier is not supported by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    float tmp = 4.0f;\n"
    "    *a = printf(\"%Lf\\n\", tmp);\n"
    "}\n",
    // 'j' length modifier is not supported by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%jx\\n\", *b);\n"
    "}\n",
    // 'z' length modifier is not supported by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%zx\\n\", *b);\n"
    "}\n",
    // 't' length modifier is not supported by OpenCL
    "void kernel foo(global int * a, global int * b)"
    "{\n"
    "    *a = printf(\"%tx\\n\", *b);\n"
    "}\n"};

TEST_P(printfBuiltinValidTest, ValidKernels) {
  cl_event writeEvent, ndRangeEvent, readEvent;

  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, false, 0,
                                      sizeof(cl_int), &buffer, 0, nullptr,
                                      &writeEvent));

  const size_t global_size = 1;

  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 1, &writeEvent,
                                        &ndRangeEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, false, 0,
                                     sizeof(cl_int), &buffer, 1, &ndRangeEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  EXPECT_EQ(0, buffer);

  EXPECT_SUCCESS(clReleaseEvent(writeEvent));
  EXPECT_SUCCESS(clReleaseEvent(ndRangeEvent));
  EXPECT_SUCCESS(clReleaseEvent(readEvent));
}

TEST_P(printfBuiltinInvalidTest, InvalidKernels) {
  if (build_error) {
    // NOTE: The program failed to build so we can not run the kernel.
    return;
  }

  cl_event writeEvent, ndRangeEvent, readEvent;

  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, inMem, false, 0,
                                      sizeof(cl_int), &buffer, 0, nullptr,
                                      &writeEvent));

  const size_t global_size = 1;

  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 1, &writeEvent,
                                        &ndRangeEvent));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, outMem, false, 0,
                                     sizeof(cl_int), &buffer, 1, &ndRangeEvent,
                                     &readEvent));

  EXPECT_SUCCESS(clWaitForEvents(1, &readEvent));

  EXPECT_EQ(-1, buffer);

  EXPECT_SUCCESS(clReleaseEvent(writeEvent));
  EXPECT_SUCCESS(clReleaseEvent(ndRangeEvent));
  EXPECT_SUCCESS(clReleaseEvent(readEvent));
}

INSTANTIATE_TEST_CASE_P(ValidKernels, printfBuiltinValidTest,
                        ::testing::ValuesIn(valid_kernels));

INSTANTIATE_TEST_CASE_P(InvalidKernels, printfBuiltinInvalidTest,
                        ::testing::ValuesIn(invalid_kernels));
