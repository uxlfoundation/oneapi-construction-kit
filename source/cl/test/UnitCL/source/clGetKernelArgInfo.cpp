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

#include <utility>

#include "Common.h"

namespace {
void sourceToBinaryKernel(cl_device_id device, cl_context context,
                          cl_program program, const char *const kernelName,
                          cl_program &binary_program,
                          cl_kernel &binary_kernel) {
  size_t size;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                  sizeof(size_t), &size, nullptr));
  std::vector<uint8_t> binary(size);
  std::array<const uint8_t *, 1> binaries{{binary.data()}};
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARIES, size,
                                  static_cast<void *>(binaries.data()),
                                  nullptr));
  cl_int binary_status;
  cl_int status;
  binary_program = clCreateProgramWithBinary(
      context, 1, &device, &size, binaries.data(), &binary_status, &status);
  EXPECT_TRUE(binary_program);
  ASSERT_SUCCESS(binary_status);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(
      clBuildProgram(binary_program, 0, nullptr, nullptr, nullptr, nullptr));
  binary_kernel = clCreateKernel(program, kernelName, &status);
  EXPECT_TRUE(binary_kernel);
  ASSERT_SUCCESS(status);
}

}  // namespace

class clGetKernelArgInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source = R"(
        kernel void foo(global uint *a,
                        constant float4 *verbose_variable_name) {
          size_t i = get_global_id(0);
          a[i] = (int)verbose_variable_name[i].x;
        })";
    const size_t source_lens = strlen(source);
    cl_int status;
    auto program =
        clCreateProgramWithSource(context, 1, &source, &source_lens, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-cl-kernel-arg-info",
                                  nullptr, nullptr));
    programs[0] = program;
    auto kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);
    kernels[0] = kernel;
    sourceToBinaryKernel(device, context, program, "foo", programs[1],
                         kernels[1]);
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    for (auto program : programs) {
      if (program) {
        EXPECT_SUCCESS(clReleaseProgram(program));
      }
    }
    ContextTest::TearDown();
  }

  // first created from source, second from binaries
  std::array<cl_program, 2> programs = {};
  std::array<cl_kernel, 2> kernels = {};
};

TEST_F(clGetKernelArgInfoTest, InvalidArgIndex) {
  for (auto kernel : kernels) {
    size_t size;
    ASSERT_EQ(
        CL_INVALID_ARG_INDEX,
        clGetKernelArgInfo(kernel, 100, CL_KERNEL_ARG_NAME, 0, nullptr, &size));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueParamName) {
  for (auto kernel : kernels) {
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetKernelArgInfo(kernel, 0, CL_SUCCESS, 0, nullptr, nullptr));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueArgAddressQualifier) {
  for (auto kernel : kernels) {
    cl_kernel_arg_address_qualifier address_qualifier;
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_ADDRESS_QUALIFIER, 0,
                           &address_qualifier, nullptr));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueArgAccessQualifier) {
  for (auto kernel : kernels) {
    cl_kernel_arg_access_qualifier access_qualifier;
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_ACCESS_QUALIFIER, 0,
                           &access_qualifier, nullptr));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueArgTypeName) {
  for (auto kernel : kernels) {
    char name[1];
    ASSERT_EQ(CL_INVALID_VALUE,
              clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_NAME, 0, name,
                                 nullptr));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueArgTypeQualifier) {
  for (auto kernel : kernels) {
    cl_kernel_arg_type_qualifier type_qualifier;
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_QUALIFIER, 0,
                           &type_qualifier, nullptr));
  }
}

TEST_F(clGetKernelArgInfoTest, InvalidValueArgName) {
  for (auto kernel : kernels) {
    char name[1];
    ASSERT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_NAME, 0, name, nullptr));
  }
}

// Redmine #5137: Check TEST_F(clGetKernelArgInfoTest,
// KernelArgInfoNotAvailable)

TEST_F(clGetKernelArgInfoTest, InvalidKernel) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_KERNEL,
      clGetKernelArgInfo(nullptr, 0, CL_KERNEL_ARG_NAME, 0, nullptr, &size));
}

TEST_F(clGetKernelArgInfoTest, DefaultArgName) {
  for (auto kernel : kernels) {
    size_t size;
    ASSERT_SUCCESS(
        clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_NAME, 0, nullptr, &size));
    std::string arg_name(size, 0);
    EXPECT_SUCCESS(clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_NAME, size,
                                      arg_name.data(), nullptr));
    EXPECT_STREQ("a", arg_name.c_str());
    ASSERT_SUCCESS(
        clGetKernelArgInfo(kernel, 1, CL_KERNEL_ARG_NAME, 0, nullptr, &size));
    arg_name.resize(size);
    EXPECT_SUCCESS(clGetKernelArgInfo(kernel, 1, CL_KERNEL_ARG_NAME, size,
                                      arg_name.data(), nullptr));
    EXPECT_STREQ("verbose_variable_name", arg_name.c_str());
  }
}

// Redmine #5125: Check access qualifiers only apply to image objects which are
// not currently supported

struct TypeNameParam {
  const char *input;
  const char *expected;
};

static std::ostream &operator<<(std::ostream &out, const TypeNameParam &param) {
  out << "TypeNameParam{.input{\"" << param.input << "\"}, .expected{\""
      << param.expected << "\"}}";
  return out;
}

struct clGetKernelArgInfoTypeNameTest
    : ucl::ContextTest,
      testing::WithParamInterface<TypeNameParam> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const char *type_str = GetParam().input;
    const bool use_double =
        (0 == strncmp("double", type_str, strlen("double")));
    const bool use_half = (0 == strncmp("half", type_str, strlen("half")));

    if (use_double && !UCL::hasDoubleSupport(device)) {
      GTEST_SKIP();
    }
    if (use_half && !UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }

    std::string source("");
    if (use_half) {
      source.append("#pragma OPENCL EXTENSION cl_khr_fp16 : enable\n");
    }
    source.append("kernel void foo(");
    source.append(type_str);       // Value
    source.append(" a, global ");  // type
    source.append(type_str);       // Pointer
    source.append(" *out)");       // type
    source.append("{ out[get_global_id(0)] = a; }");

    const char *sources[] = {source.c_str()};
    size_t source_lens[] = {source.size()};
    cl_int status;
    auto program =
        clCreateProgramWithSource(context, 1, sources, source_lens, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-cl-kernel-arg-info",
                                  nullptr, nullptr));
    programs[0] = program;
    auto kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);
    kernels[0] = kernel;

    sourceToBinaryKernel(device, context, program, "foo", programs[1],
                         kernels[1]);
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    for (auto program : programs) {
      if (program) {
        EXPECT_SUCCESS(clReleaseProgram(program));
      }
    }
    ContextTest::TearDown();
  }

  // first created from source, second from binaries
  std::array<cl_program, 2> programs = {};
  std::array<cl_kernel, 2> kernels = {};
};

TEST_P(clGetKernelArgInfoTypeNameTest, Default) {
  for (auto kernel : kernels) {
    // Value type
    size_t size;
    ASSERT_SUCCESS(clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_NAME, 0,
                                      nullptr, &size));
    std::string type_name(size, 0);
    EXPECT_SUCCESS(clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_NAME, size,
                                      type_name.data(), nullptr));
    EXPECT_STREQ(GetParam().expected, type_name.c_str());

    // Pointer type
    ASSERT_SUCCESS(clGetKernelArgInfo(kernel, 1, CL_KERNEL_ARG_TYPE_NAME, 0,
                                      nullptr, &size));
    type_name.resize(size);
    EXPECT_SUCCESS(clGetKernelArgInfo(kernel, 1, CL_KERNEL_ARG_TYPE_NAME, size,
                                      type_name.data(), nullptr));
    std::string expected(GetParam().expected);
    expected.append("*");
    EXPECT_STREQ(expected.c_str(), type_name.c_str());
  }
}

// <input, expected>
// input: Type used in kernel argument.
// expected: How we expect CL_KERNEL_ARG_TYPE_NAME to describe the type.
//
// The rules are essentially:
//   * signed x --> x
//   * unsigned x --> ux
//   * <empty> x --> x
//
// However, 'signed char' remains 'signed char'.  Seemingly an inherited rule
// from C99, that was never actually necessary in OpenCL C.  This violates the
// spec saying that 'expected' will contain no whitespace, and is
// controversial.  It is possible that this behaviour will change in the spec,
// see https://github.com/KhronosGroup/OpenCL-Docs/pull/558
const TypeNameParam typeNameParams[] = {
    {"char", "char"},
#if CA_3424_RESOLVED
    {"signed char", "signed char"},  // Special case, see above.
#endif
    {"char2", "char2"},
    {"char3", "char3"},
    {"char4", "char4"},
    {"char8", "char8"},
    {"char16", "char16"},
    {"unsigned char", "uchar"},
    {"uchar", "uchar"},
    {"uchar2", "uchar2"},
    {"uchar3", "uchar3"},
    {"uchar4", "uchar4"},
    {"uchar8", "uchar8"},
    {"uchar16", "uchar16"},
    {"signed short", "short"},
    {"signed short int", "short"},
    {"short", "short"},
    {"short int", "short"},
    {"short2", "short2"},
    {"short3", "short3"},
    {"short4", "short4"},
    {"short8", "short8"},
    {"short16", "short16"},
    {"unsigned short", "ushort"},
    {"unsigned short int", "ushort"},
    {"ushort", "ushort"},
    {"ushort2", "ushort2"},
    {"ushort3", "ushort3"},
    {"ushort4", "ushort4"},
    {"ushort8", "ushort8"},
    {"ushort16", "ushort16"},
    {"signed int", "int"},
    {"int", "int"},
    {"int2", "int2"},
    {"int3", "int3"},
    {"int4", "int4"},
    {"int8", "int8"},
    {"int16", "int16"},
    {"unsigned int", "uint"},
    {"uint", "uint"},
    {"uint2", "uint2"},
    {"uint3", "uint3"},
    {"uint4", "uint4"},
    {"uint8", "uint8"},
    {"uint16", "uint16"},
    {"signed long", "long"},
    {"signed long int", "long"},
    {"long", "long"},
    {"long int", "long"},
    {"long2", "long2"},
    {"long3", "long3"},
    {"long4", "long4"},
    {"long8", "long8"},
    {"long16", "long16"},
    {"unsigned long", "ulong"},
    {"unsigned long int", "ulong"},
    {"ulong", "ulong"},
    {"ulong2", "ulong2"},
    {"ulong3", "ulong3"},
    {"ulong4", "ulong4"},
    {"ulong8", "ulong8"},
    {"ulong16", "ulong16"},
    {"float", "float"},
    {"float2", "float2"},
    {"float3", "float3"},
    {"float4", "float4"},
    {"float8", "float8"},
    {"float16", "float16"},
    {"double", "double"},
    {"double2", "double2"},
    {"double3", "double3"},
    {"double4", "double4"},
    {"double8", "double8"},
    {"double16", "double16"},
    {"half", "half"},
    {"half2", "half2"},
    {"half3", "half3"},
    {"half4", "half4"},
    {"half8", "half8"},
    {"half16", "half16"},
};

INSTANTIATE_TEST_CASE_P(Default, clGetKernelArgInfoTypeNameTest,
                        ::testing::ValuesIn(typeNameParams));

typedef std::pair<cl_kernel_arg_type_qualifier, const char *> type_qual_pair;

struct clGetKernelArgInfoTypeQualifierTest
    : ucl::ContextTest,
      testing::WithParamInterface<type_qual_pair> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    std::string source("kernel void foo(global int * ");
    source.append(GetParam().second);
    source.append(
        " a, global int * b){ size_t i = get_global_id(0); *b = * a; }");

    const char *sources[] = {source.c_str()};
    size_t source_lens[] = {source.size()};
    cl_int status;
    auto program =
        clCreateProgramWithSource(context, 1, sources, source_lens, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-cl-kernel-arg-info",
                                  nullptr, nullptr));
    programs[0] = program;

    auto kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);
    kernels[0] = kernel;

    sourceToBinaryKernel(device, context, program, "foo", programs[1],
                         kernels[1]);
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    for (auto program : programs) {
      if (program) {
        EXPECT_SUCCESS(clReleaseProgram(program));
      }
    }
    ContextTest::TearDown();
  }

  // first created from source, second from binaries
  std::array<cl_program, 2> programs = {};
  std::array<cl_kernel, 2> kernels = {};
};

TEST_P(clGetKernelArgInfoTypeQualifierTest, Default) {
  for (auto kernel : kernels) {
    cl_kernel_arg_type_qualifier typeQual;
    ASSERT_SUCCESS(clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_QUALIFIER,
                                      sizeof(cl_kernel_arg_type_qualifier),
                                      &typeQual, nullptr));
  }
}

static type_qual_pair typeQualParams[] = {
    type_qual_pair(CL_KERNEL_ARG_TYPE_NONE, ""),
    type_qual_pair(CL_KERNEL_ARG_TYPE_CONST, "const"),
    type_qual_pair(CL_KERNEL_ARG_TYPE_VOLATILE, "volatile"),
    type_qual_pair(CL_KERNEL_ARG_TYPE_RESTRICT, "restrict"),
};

INSTANTIATE_TEST_CASE_P(Default, clGetKernelArgInfoTypeQualifierTest,
                        ::testing::ValuesIn(typeQualParams));

struct clGetKernelArgInfoConstTypeQualifierTest
    : ucl::ContextTest,
      testing::WithParamInterface<type_qual_pair> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    std::string source("kernel void foo(__constant int * ");
    source.append(GetParam().second);
    source.append(" a){ size_t i = get_global_id(0); }");
    const char *sources[] = {source.c_str()};
    size_t source_lens[] = {source.size()};
    cl_int status;
    auto program =
        clCreateProgramWithSource(context, 1, sources, source_lens, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);
    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "-cl-kernel-arg-info",
                                  nullptr, nullptr));
    programs[0] = program;
    auto kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);
    kernels[0] = kernel;
    sourceToBinaryKernel(device, context, program, "foo", programs[1],
                         kernels[1]);
  }

  void TearDown() override {
    for (auto kernel : kernels) {
      if (kernel) {
        EXPECT_SUCCESS(clReleaseKernel(kernel));
      }
    }
    for (auto program : programs) {
      if (program) {
        EXPECT_SUCCESS(clReleaseProgram(program));
      }
    }
    ContextTest::TearDown();
  }

  // first created from source, second from binaries
  std::array<cl_program, 2> programs = {};
  std::array<cl_kernel, 2> kernels = {};
};

TEST_P(clGetKernelArgInfoConstTypeQualifierTest, Default) {
  for (auto kernel : kernels) {
    cl_kernel_arg_type_qualifier typeQual;
    ASSERT_SUCCESS(clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_TYPE_QUALIFIER,
                                      sizeof(cl_kernel_arg_type_qualifier),
                                      &typeQual, nullptr));
  }
}

static type_qual_pair typeConstQualParams[] = {
    type_qual_pair(CL_KERNEL_ARG_TYPE_RESTRICT, "restrict"),
};

INSTANTIATE_TEST_CASE_P(Default, clGetKernelArgInfoConstTypeQualifierTest,
                        ::testing::ValuesIn(typeConstQualParams));

using clGetKernelArgInfo2Test = ucl::ContextTest;
TEST_F(clGetKernelArgInfo2Test, InfoNotAvailable) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *source = "kernel void foo(global int *i) {}";
  cl_int status;
  cl_program program =
      clCreateProgramWithSource(context, 1, &source, nullptr, &status);
  EXPECT_SUCCESS(status);
  EXPECT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  EXPECT_SUCCESS(status);
  cl_program binary_program = nullptr;
  cl_kernel binary_kernel = nullptr;
  sourceToBinaryKernel(device, context, program, "foo", binary_program,
                       binary_kernel);
  size_t size;
  EXPECT_EQ_ERRCODE(
      CL_KERNEL_ARG_INFO_NOT_AVAILABLE,
      clGetKernelArgInfo(kernel, 0, CL_KERNEL_ARG_NAME, 0, nullptr, &size));
  EXPECT_EQ_ERRCODE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE,
                    clGetKernelArgInfo(binary_kernel, 0, CL_KERNEL_ARG_NAME, 0,
                                       nullptr, &size));
  EXPECT_SUCCESS(clReleaseProgram(binary_program));
  EXPECT_SUCCESS(clReleaseKernel(binary_kernel));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}
