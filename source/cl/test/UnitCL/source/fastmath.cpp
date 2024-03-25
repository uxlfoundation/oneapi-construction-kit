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

#include <cmath>

#include "Common.h"
#include "EventWaitList.h"

namespace {
void ReplaceAll(std::string &str, const std::string &from,
                const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

template <typename T>
struct ReleaseHelper {
  ReleaseHelper(T t) : t(t) {}

  ~ReleaseHelper();

  operator T() const { return t; }

 private:
  T t;
};

template <>
ReleaseHelper<cl_kernel>::~ReleaseHelper() {
  clReleaseKernel(t);
}

template <>
ReleaseHelper<cl_mem>::~ReleaseHelper() {
  clReleaseMemObject(t);
}

template <>
ReleaseHelper<cl_program>::~ReleaseHelper() {
  clReleaseProgram(t);
}
}  // namespace

struct FastMathTest : ucl::CommandQueueTest,
                      testing::WithParamInterface<const char *> {
  cl_program createProgram(const char *source) const {
    cl_int errorcode = CL_SUCCESS;
    cl_program program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    EXPECT_SUCCESS(errorcode);

    EXPECT_EQ_ERRCODE(
        CL_SUCCESS, clBuildProgram(program, 0, nullptr, "-cl-fast-relaxed-math",
                                   nullptr, nullptr));
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

    return mem;
  }

  void setMemArg(cl_kernel kernel, cl_uint i, cl_mem mem) const {
    const cl_int errorcode =
        clSetKernelArg(kernel, i, sizeof(mem), (void *)&mem);
    ASSERT_SUCCESS(errorcode);
  }

  bool skipTest(const std::string &param) {
    // if double is not found in the param name, we want to always run the test
    if (std::string::npos == param.find("double")) {
      return false;
    }

    // If the device doesn't have double support, skip the test
    return !UCL::hasDoubleSupport(device);
  }
};

TEST_P(FastMathTest, Logic) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::string source =
      "void kernel f(global %s *a,\n"
      "  global %s *b,\n"
      "  global %s *c,\n"
      "  global %s *d,\n"
      "  global %s *e) {\n"
      "  size_t gid = get_global_id(0);\n"
      "  a[gid] -= b[gid] * c[gid] + d[gid] / e[gid];\n"
      "}";

  if (skipTest(GetParam())) {
    return;
  }

  ReplaceAll(source, "%s", GetParam());

  const ReleaseHelper<cl_program> program(createProgram(source.c_str()));

  const ReleaseHelper<cl_kernel> kernel(createKernel(program, "f"));

  const size_t bytes = 128;

  const ReleaseHelper<cl_mem> mem_a(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_b(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_c(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_d(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_e(createBuffer(bytes));

  setMemArg(kernel, 0, mem_a);
  setMemArg(kernel, 1, mem_b);
  setMemArg(kernel, 2, mem_c);
  setMemArg(kernel, 3, mem_d);
  setMemArg(kernel, 4, mem_e);

  const size_t global_size = bytes / UCL::getTypeSize(GetParam());
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));
}

TEST_P(FastMathTest, GeometricDistance) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::string source =
      "void kernel f(global %s *a,\n"
      "  global %s *b,\n"
      "  global %s *c) {\n"
      "  size_t gid = get_global_id(0);\n"
      "  a[gid] = distance(b[gid], c[gid]);\n"
      "}";

  if (skipTest(GetParam())) {
    return;
  }

  ReplaceAll(source, "%s", GetParam());

  const ReleaseHelper<cl_program> program(createProgram(source.c_str()));

  const ReleaseHelper<cl_kernel> kernel(createKernel(program, "f"));

  const size_t bytes = 128;

  const ReleaseHelper<cl_mem> mem_a(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_b(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_c(createBuffer(bytes));

  setMemArg(kernel, 0, mem_a);
  setMemArg(kernel, 1, mem_b);
  setMemArg(kernel, 2, mem_c);

  const size_t global_size = bytes / UCL::getTypeSize(GetParam());
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));
}

TEST_P(FastMathTest, GeometricLength) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::string source =
      "void kernel f(global %s *a,\n"
      "  global %s *b) {\n"
      "  size_t gid = get_global_id(0);\n"
      "  a[gid] = length(b[gid]);\n"
      "}";

  if (skipTest(GetParam())) {
    return;
  }

  ReplaceAll(source, "%s", GetParam());

  const ReleaseHelper<cl_program> program(createProgram(source.c_str()));

  const ReleaseHelper<cl_kernel> kernel(createKernel(program, "f"));

  const size_t bytes = 128;

  const ReleaseHelper<cl_mem> mem_a(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_b(createBuffer(bytes));

  setMemArg(kernel, 0, mem_a);
  setMemArg(kernel, 1, mem_b);

  const size_t global_size = bytes / UCL::getTypeSize(GetParam());
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));
}

TEST_P(FastMathTest, GeometricNormalize) {
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::string source =
      "void kernel f(global %s *a,\n"
      "  global %s *b) {\n"
      "  size_t gid = get_global_id(0);\n"
      "  a[gid] = normalize(b[gid]);\n"
      "}";

  if (skipTest(GetParam())) {
    return;
  }

  ReplaceAll(source, "%s", GetParam());

  const ReleaseHelper<cl_program> program(createProgram(source.c_str()));

  const ReleaseHelper<cl_kernel> kernel(createKernel(program, "f"));

  const size_t bytes = 128;

  const ReleaseHelper<cl_mem> mem_a(createBuffer(bytes));
  const ReleaseHelper<cl_mem> mem_b(createBuffer(bytes));

  setMemArg(kernel, 0, mem_a);
  setMemArg(kernel, 1, mem_b);

  const size_t global_size = bytes / UCL::getTypeSize(GetParam());
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));

  ASSERT_SUCCESS(clFinish(command_queue));
}

INSTANTIATE_TEST_CASE_P(FastMath, FastMathTest,
                        ::testing::Values("float", "float2", "float3", "float4",
                                          "double", "double2", "double3",
                                          "double4"));
