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

namespace {
std::string attribute(const std::string &inner) {
  return "__attribute__((" + inner + "))";
}

std::string reqd_work_group_size(const std::array<size_t, 3> sizes) {
  return "reqd_work_group_size(" + std::to_string(sizes[0]) + "," +
         std::to_string(sizes[1]) + "," + std::to_string(sizes[2]) + ")";
}

std::string work_group_size_hint(const std::array<size_t, 3> sizes) {
  return "reqd_work_group_size(" + std::to_string(sizes[0]) + "," +
         std::to_string(sizes[1]) + "," + std::to_string(sizes[2]) + ")";
}
}  // namespace

class clGetKernelInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    cl_int errcode;
    size_t max_work_group_size;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                   sizeof(max_work_group_size),
                                   &max_work_group_size, nullptr));

    std::array<size_t, 3> max_work_item_sizes;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                   sizeof(max_work_item_sizes),
                                   max_work_item_sizes.data(), nullptr));

    work_group_size[0] = std::min(max_work_item_sizes[0], max_work_group_size);

    std::string source = "void kernel " +
                         attribute(reqd_work_group_size(work_group_size)) +
                         attribute(work_group_size_hint(work_group_size)) +
                         attribute("vec_type_hint(ulong4)") +
                         "foo(global int * a, global int * b) {*a = *b;}";
    const char *csource = source.data();
    const size_t source_length = source.size();
    program = clCreateProgramWithSource(context, 1, &csource, &source_length,
                                        &errcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errcode);
    EXPECT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &errcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() {
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  std::array<size_t, 3> work_group_size = {{1, 1, 1}};

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clGetKernelInfoTest, NullKernel) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clGetKernelInfo(nullptr, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size));
}

TEST_F(clGetKernelInfoTest, InvalidParamName) {
  size_t size;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetKernelInfo(kernel, CL_SUCCESS, 0, nullptr, &size));
}

TEST_F(clGetKernelInfoTest, KernelFunctionName) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size));
  ASSERT_EQ(strlen("foo") + 1, size);
  UCL::Buffer<char> kernelFunctionName(size);
  EXPECT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, size,
                                 kernelFunctionName, nullptr));
  ASSERT_TRUE(size ? size == strlen(kernelFunctionName) + 1 : true);
}

TEST_F(clGetKernelInfoTest, KernelNumArgs) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint numArgs;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, size, &numArgs, nullptr));
  ASSERT_EQ(2u, numArgs);
}

TEST_F(clGetKernelInfoTest, KernelReferenceCount) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint refCount;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_REFERENCE_COUNT, size,
                                 &refCount, nullptr));
  ASSERT_EQ(1u, refCount);
}

TEST_F(clGetKernelInfoTest, KernelContext) {
  cl_context kernelContext;
  size_t size;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_CONTEXT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context), size);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_CONTEXT, size,
                                 static_cast<void *>(&kernelContext), nullptr));
  ASSERT_EQ(context, kernelContext);
}

TEST_F(clGetKernelInfoTest, KernelProgram) {
  cl_program kernelProgram;
  size_t size;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_program), size);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, size,
                                 static_cast<void *>(&kernelProgram), nullptr));
  ASSERT_EQ(program, kernelProgram);
}

TEST_F(clGetKernelInfoTest, KernelAttributes) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection doesn't propogate kernel attributes.
  }
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, 0, nullptr, &size));

  UCL::Buffer<char> kernelAttribute(size);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, size,
                                 kernelAttribute, nullptr));
  ASSERT_TRUE(size == strlen(kernelAttribute) + 1);

  ASSERT_NE(nullptr, strstr(kernelAttribute, "vec_type_hint(ulong4)"))
      << kernelAttribute;
  ASSERT_NE(nullptr, strstr(kernelAttribute,
                            work_group_size_hint(work_group_size).c_str()))
      << kernelAttribute;
  ASSERT_NE(nullptr, strstr(kernelAttribute,
                            reqd_work_group_size(work_group_size).c_str()))
      << kernelAttribute;
}

class clGetKernelInfoTwoKernelsTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errcode;
    const char *source =
        "void __kernel foo(__global int * a, __global int * b) {*a = *b;}\n"
        "void __kernel __attribute__((vec_type_hint(ulong4)))\n"
        "              __attribute__((reqd_work_group_size(1, 1, 1)))\n"
        "  boo(__global float * a, __global float * b, __global float * c)\n"
        "    {*a = *c; *b = *c;}\n";
    const size_t source_length = strlen(source);
    program = clCreateProgramWithSource(context, 1, &source, &source_length,
                                        &errcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errcode);
    EXPECT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "boo", &errcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() override {
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_F(clGetKernelInfoTwoKernelsTest, KernelFunctionName) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size));
  ASSERT_EQ(strlen("boo") + 1, size);
  std::string kernelFunctionName(size, 0);
  EXPECT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, size,
                                 kernelFunctionName.data(), nullptr));
  ASSERT_EQ(0, strcmp("boo", kernelFunctionName.c_str()));
}

TEST_F(clGetKernelInfoTwoKernelsTest, KernelNumArgs) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint numArgs;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, size, &numArgs, nullptr));
  ASSERT_EQ(3u, numArgs);
}

TEST_F(clGetKernelInfoTwoKernelsTest, KernelReferenceCount) {
  size_t size;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint refCount;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_REFERENCE_COUNT, size,
                                 &refCount, nullptr));
  ASSERT_EQ(1u, refCount);
}

TEST_F(clGetKernelInfoTwoKernelsTest, KernelContext) {
  cl_context kernelContext;
  size_t size;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_CONTEXT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context), size);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_CONTEXT, size,
                                 static_cast<void *>(&kernelContext), nullptr));
  ASSERT_EQ(context, kernelContext);
}

TEST_F(clGetKernelInfoTwoKernelsTest, KernelProgram) {
  cl_program kernelProgram;
  size_t size;
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_program), size);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, size,
                                 static_cast<void *>(&kernelProgram), nullptr));
  ASSERT_EQ(program, kernelProgram);
}

TEST_F(clGetKernelInfoTwoKernelsTest, KernelAttributes) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection doesn't propogate kernel attributes.
  }
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, 0, nullptr, &size));

  std::string kernelAttribute(size, 0);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, size,
                                 kernelAttribute.data(), nullptr));

  ASSERT_NE(nullptr, strstr(kernelAttribute.c_str(), "vec_type_hint(ulong4)"));
  ASSERT_NE(nullptr,
            strstr(kernelAttribute.c_str(), "reqd_work_group_size(1,1,1)"));
}

typedef std::pair<const char *, std::vector<const char *>> InputPair;

struct clGetKernelInfoAttributeTest : ucl::ContextTest,
                                      testing::WithParamInterface<InputPair> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection doesn't propogate kernel attributes.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *strings = GetParam().first;
    const size_t source_length = strlen(GetParam().first);
    cl_int errcode;
    program = clCreateProgramWithSource(context, 1, &strings, &source_length,
                                        &errcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errcode);
    EXPECT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
    kernel = clCreateKernel(program, "foo", &errcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() {
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_P(clGetKernelInfoAttributeTest, Default) {
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, 0, nullptr, &size));

  std::string kernelAttribute(size, 0);
  ASSERT_SUCCESS(clGetKernelInfo(kernel, CL_KERNEL_ATTRIBUTES, size,
                                 kernelAttribute.data(), nullptr));

  for (auto kernelAttributeName : GetParam().second) {
    ASSERT_NE(nullptr, strstr(kernelAttribute.c_str(), kernelAttributeName));
  }
}

static InputPair AttributeParams[] = {
    {
        "void kernel "
        "__attribute__((reqd_work_group_size(1, 1, 1)))"
        "__attribute__((work_group_size_hint(1, 1, 1)))"
        "__attribute__((vec_type_hint(ulong4)))"
        "  foo(global int * a, global int * b) {*a = *b;}",
        {
            "vec_type_hint(ulong4)",
            "work_group_size_hint(1,1,1)",
            "reqd_work_group_size(1,1,1)",
        },
    },
    {
        "void kernel "
        "__attribute__((vec_type_hint(int)))"
        "  foo(global int * a, global int * b) {*a = *b;}",
        {
            "vec_type_hint(int)",
        },
    },
    {
        "void kernel "
        "__attribute__((vec_type_hint(float)))"
        "  foo(global int * a, global int * b) {*a = *b;}",
        {
            "vec_type_hint(float)",
        },
    },
    {
        "void kernel "
        "__attribute__((reqd_work_group_size(1, 1, 1)))"
        "  foo(global int * a, global int * b) {*a = *b;}",
        {
            "reqd_work_group_size(1,1,1)",
        },
    },
    {
        "void kernel "
        "__attribute__((work_group_size_hint(1, 1, 1)))"
        "  foo(global int * a, global int * b) {*a = *b;}",
        {
            "work_group_size_hint(1,1,1)",
        },
    },
};

INSTANTIATE_TEST_CASE_P(Default, clGetKernelInfoAttributeTest,
                        ::testing::ValuesIn(AttributeParams));
