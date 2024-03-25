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

#include <cstdio>

#include "Common.h"
#include "EventWaitList.h"
#include "kts/precision.h"

using namespace kts::ucl;

// Abstract class for testing the Codeplay softmath extension by building two
// programs for each test compiled from identical source. One program with
// '-codeplay-soft-math' as a compilation option, to enable the extension,
// and one program without the option.
struct SoftMathTest : ucl::CommandQueueTest,
                      testing::WithParamInterface<const char *> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      // Injection can't differentiate between different fixture instances,
      // uses same program binary for all tests, causes crashs and validation
      // failures.
      GTEST_SKIP();
    }
    if (!(isPlatformExtensionSupported("cl_codeplay_soft_math") &&
          getDeviceCompilerAvailable())) {
      GTEST_SKIP();
    }
  }

  void TearDown() override {
    if (normal_program) {
      EXPECT_SUCCESS(clReleaseProgram(normal_program));
    }
    if (soft_math_program) {
      EXPECT_SUCCESS(clReleaseProgram(soft_math_program));
    }
    if (normal_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(normal_kernel));
    }
    if (soft_math_kernel) {
      EXPECT_SUCCESS(clReleaseKernel(soft_math_kernel));
    }
    CommandQueueTest::TearDown();
  }

  cl_program createProgram(const char *source, const char *options) const {
    cl_int errorcode = CL_SUCCESS;
    cl_program program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    EXPECT_SUCCESS(errorcode);
    EXPECT_SUCCESS(
        clBuildProgram(program, 0, nullptr, options, nullptr, nullptr));
    return program;
  }

  cl_kernel createKernel(cl_program program, const char *name) const {
    cl_int errorcode = CL_SUCCESS;
    cl_kernel kernel = clCreateKernel(program, name, &errorcode);
    EXPECT_TRUE(kernel);
    EXPECT_SUCCESS(errorcode);
    return kernel;
  }

  cl_mem createBuffer(const size_t size) const {
    cl_int errorcode = CL_SUCCESS;
    cl_mem mem = clCreateBuffer(context, 0, size, nullptr, &errorcode);
    EXPECT_TRUE(mem);
    EXPECT_SUCCESS(errorcode);
    char *const mapped = static_cast<char *>(
        mapBuffer(mem, CL_MAP_WRITE_INVALIDATE_REGION, size));
    for (size_t i = 0; i < size; i++) {
      mapped[i] = static_cast<char>(rand());
    }
    unmapBuffer(mem, mapped);
    return mem;
  }

  void setMemArg(cl_kernel kernel, cl_uint i, cl_mem mem) const {
    const cl_int errorcode =
        clSetKernelArg(kernel, i, sizeof(mem), (void *)&mem);
    ASSERT_SUCCESS(errorcode);
  }

  void *mapBuffer(cl_mem mem, cl_map_flags flags, size_t bytes) const {
    cl_int errorcode;
    void *buffer = clEnqueueMapBuffer(command_queue, mem, true, flags, 0, bytes,
                                      0, nullptr, nullptr, &errorcode);
    EXPECT_SUCCESS(errorcode);
    return buffer;
  }

  void unmapBuffer(cl_mem mem, void *const ptr) const {
    ASSERT_SUCCESS(
        clEnqueueUnmapMemObject(command_queue, mem, ptr, 0, nullptr, nullptr));
  }

  // Generates softmath & reference kernels from identical source, substituting
  // in the builtin name to test.
  void BuildKernels(const char *source, const std::string &builtin) {
    // If we are doing a vec3, the kernel will use vload3/store3 to access
    // a scalar pointer.
    std::string param = GetParam();
    const bool vec3 = '3' == param.back();
    if (vec3) {
      param.pop_back();
    }
    // Set the type to test via a macro
    std::string options("-DTYPE=");
    options.append(param);
    // Set the builtin name to test via a macro
    options.append(" -DBUILTIN=");
    options.append(builtin);
    normal_program = createProgram(source, options.c_str());
    // Enable softmath extension via compilation flag
    options.append(" -codeplay-soft-math");
    soft_math_program = createProgram(source, options.c_str());
    // Kernel has name 'f'
    normal_kernel = createKernel(normal_program, "f");
    soft_math_kernel = createKernel(soft_math_program, "f");
  }

  void RunKernels(const size_t global_size) {
    ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, normal_kernel, 1,
                                          nullptr, &global_size, nullptr, 0,
                                          nullptr, nullptr));
    ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, soft_math_kernel, 1,
                                          nullptr, &global_size, nullptr, 0,
                                          nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));
  }

  // Pure virtual function to implement test code for the parameterized builtin
  // function name
  virtual void TestBuiltin(const std::string &builtin) = 0;

  cl_program normal_program = nullptr;
  cl_program soft_math_program = nullptr;
  cl_kernel normal_kernel = nullptr;
  cl_kernel soft_math_kernel = nullptr;
};

// Class for testing the softmath extension for builtins with a single
// parameter. Setting 'verify' template parameter to true will verify softmath
// kernel results are byte identical to the values returned by the reference
// kernel.
template <bool verify>
struct SoftMathTestOneArg : public SoftMathTest {
  static const char *program_str;
  static const char *program_vec3_str;
  static constexpr size_t bytes = 128L * 1024L * 3L;

  virtual void SetUp() override {
    SoftMathTest::SetUp();

    normal_mem = createBuffer(bytes);
    soft_math_mem = createBuffer(bytes);
    input_mem = createBuffer(bytes);
  }

  virtual void TearDown() override {
    SoftMathTest::TearDown();

    EXPECT_SUCCESS(clReleaseMemObject(normal_mem));
    EXPECT_SUCCESS(clReleaseMemObject(soft_math_mem));
    EXPECT_SUCCESS(clReleaseMemObject(input_mem));
  }

  void TestBuiltin(const std::string &builtin) override {
    const std::string param = GetParam();
    const bool vec3 = '3' == param.back();
    const char *source = vec3 ? program_vec3_str : program_str;
    BuildKernels(source, builtin);

    setMemArg(normal_kernel, 0, normal_mem);
    setMemArg(normal_kernel, 1, input_mem);

    setMemArg(soft_math_kernel, 0, soft_math_mem);
    setMemArg(soft_math_kernel, 1, input_mem);

    const size_t global_size = bytes / UCL::getTypeSize(param.c_str());
    RunKernels(global_size);

    if (verify) {
      char *normal =
          static_cast<char *>(mapBuffer(normal_mem, CL_MAP_READ, bytes));
      char *soft_math =
          static_cast<char *>(mapBuffer(soft_math_mem, CL_MAP_READ, bytes));

      for (size_t i = 0; i < bytes; i++) {
        ASSERT_EQ(normal[i], soft_math[i])
            << "expected " << normal[i] << ", actual " << soft_math[i];
      }

      unmapBuffer(normal_mem, normal);
      unmapBuffer(soft_math_mem, soft_math);
    }
  }

  cl_mem normal_mem;
  cl_mem soft_math_mem;
  cl_mem input_mem;
};

template <bool verify>
const char *SoftMathTestOneArg<verify>::program_str =
    "void kernel f(global TYPE *o,\n"
    "  global TYPE *a) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  o[gid] = BUILTIN(a[gid]);\n"
    "}";

template <bool verify>
const char *SoftMathTestOneArg<verify>::program_vec3_str =
    "void kernel f(global TYPE *o,\n"
    "  global TYPE *a) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  vstore3(BUILTIN(vload3(gid, a)), gid, o);\n"
    "}";

// Class for testing the softmath extension for builtins which take two
// parameters. Setting 'verify' template parameter to true will verify softmath
// kernel results are byte identical to the values returned by the reference
// kernel.
template <bool verify>
struct SoftMathTestTwoArg : public SoftMathTest {
  static const char *program_str;
  static const char *program_vec3_str;
  static constexpr size_t bytes = 128L * 1024L * 3L;

  virtual void SetUp() override {
    SoftMathTest::SetUp();

    normal_mem = createBuffer(bytes);
    soft_math_mem = createBuffer(bytes);
    inputA_mem = createBuffer(bytes);
    inputB_mem = createBuffer(bytes);
  }

  virtual void TearDown() override {
    SoftMathTest::TearDown();

    EXPECT_SUCCESS(clReleaseMemObject(normal_mem));
    EXPECT_SUCCESS(clReleaseMemObject(soft_math_mem));
    EXPECT_SUCCESS(clReleaseMemObject(inputA_mem));
    EXPECT_SUCCESS(clReleaseMemObject(inputB_mem));
  }

  void TestBuiltin(const std::string &builtin) override {
    const std::string param = GetParam();
    const bool vec3 = '3' == param.back();
    const char *source = vec3 ? program_vec3_str : program_str;
    BuildKernels(source, builtin);

    setMemArg(normal_kernel, 0, normal_mem);
    setMemArg(normal_kernel, 1, inputA_mem);
    setMemArg(normal_kernel, 2, inputB_mem);

    setMemArg(soft_math_kernel, 0, soft_math_mem);
    setMemArg(soft_math_kernel, 1, inputA_mem);
    setMemArg(soft_math_kernel, 2, inputB_mem);

    const size_t global_size = bytes / UCL::getTypeSize(param.c_str());
    RunKernels(global_size);

    if (verify) {
      char *normal =
          static_cast<char *>(mapBuffer(normal_mem, CL_MAP_READ, bytes));
      char *soft_math =
          static_cast<char *>(mapBuffer(soft_math_mem, CL_MAP_READ, bytes));

      for (size_t i = 0; i < bytes; i++) {
        ASSERT_EQ(normal[i], soft_math[i])
            << "expected " << normal[i] << ", actual " << soft_math[i];
      }

      unmapBuffer(normal_mem, normal);
      unmapBuffer(soft_math_mem, soft_math);
    }
  }

  cl_mem normal_mem;
  cl_mem soft_math_mem;
  cl_mem inputA_mem;
  cl_mem inputB_mem;
};

template <bool verify>
const char *SoftMathTestTwoArg<verify>::program_str =
    "void kernel f(global TYPE *o,\n"
    "  global TYPE *a, global TYPE *b) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  o[gid] = BUILTIN(a[gid], b[gid]);\n"
    "}";

template <bool verify>
const char *SoftMathTestTwoArg<verify>::program_vec3_str =
    "void kernel f(global TYPE *o,\n"
    "  global TYPE *a, global TYPE *b) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  vstore3(BUILTIN(vload3(gid, a), vload3(gid, b)), gid, o);\n"
    "}";

using SoftMathTestOneArgInteger = SoftMathTestOneArg<true>;
using SoftMathTestTwoArgInteger = SoftMathTestTwoArg<true>;
// native floating point maths builtins have undefined precision requirements,
// therefore there is no reference value to validate against.
using SoftMathTestNativeFloat = SoftMathTestOneArg<false>;

INSTANTIATE_TEST_CASE_P(SoftMath, SoftMathTestNativeFloat,
                        ::testing::Values("float", "float2", "float3", "float4",
                                          "float8", "float16"));

const auto IntTestTypes = ::testing::Values(
    "char", "char2", "char3", "char4", "char8", "char16", "uchar", "uchar2",
    "uchar3", "uchar4", "uchar8", "uchar16", "short", "short2", "short3",
    "short4", "short8", "short16", "ushort", "ushort2", "ushort3", "ushort4",
    "ushort8", "ushort16", "int", "int2", "int3", "int4", "int8", "int16",
    "uint", "uint2", "uint3", "uint4", "uint8", "uint16", "long", "long2",
    "long3", "long4", "long8", "long16", "ulong", "ulong2", "ulong3", "ulong4",
    "ulong8", "ulong16");
INSTANTIATE_TEST_CASE_P(SoftMath, SoftMathTestOneArgInteger, IntTestTypes);
INSTANTIATE_TEST_CASE_P(SoftMath, SoftMathTestTwoArgInteger, IntTestTypes);

TEST_P(SoftMathTestNativeFloat, native_cos) { TestBuiltin("native_cos"); }

TEST_P(SoftMathTestNativeFloat, native_exp) { TestBuiltin("native_exp"); }

// CA-2477: exp2f missing on MinGW
#if defined(__MINGW32__) || defined(__MINGW64__)
TEST_P(SoftMathTestNativeFloat, DISABLED_native_exp2) {
#else
TEST_P(SoftMathTestNativeFloat, native_exp2) {
#endif
  TestBuiltin("native_exp2");
}

TEST_P(SoftMathTestNativeFloat, native_log) { TestBuiltin("native_log"); }

// CA-2477: log2f missing on MinGW
#if defined(__MINGW32__) || defined(__MINGW64__)
TEST_P(SoftMathTestNativeFloat, DISABLED_native_log2) {
#else
TEST_P(SoftMathTestNativeFloat, native_log2) {
#endif
  TestBuiltin("native_log2");
}

TEST_P(SoftMathTestNativeFloat, native_log10) { TestBuiltin("native_log10"); }

TEST_P(SoftMathTestNativeFloat, native_sqrt) { TestBuiltin("native_sqrt"); }

TEST_P(SoftMathTestNativeFloat, native_sin) { TestBuiltin("native_sin"); }

TEST_P(SoftMathTestOneArgInteger, clz) { TestBuiltin("clz"); }

TEST_P(SoftMathTestTwoArgInteger, mul_hi) { TestBuiltin("mul_hi"); }
