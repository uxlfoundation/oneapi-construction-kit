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

// This file provides all testing of the OpenCL ctz builtin which was introduced
// in OpenCL-2.0.

#include <cargo/fixed_vector.h>
#include <cargo/type_traits.h>

#include <limits>

#include "Common.h"

template <typename T>
static std::enable_if_t<std::is_integral_v<T>, T> reference_ctz(T val) {
  if (!val) {
    return sizeof(val) << 3;
  }
  T count{0};
  while (T(0) == (T(1) & val)) {
    val >>= 1;
    ++count;
  }
  return count;
}

template <typename T>
static std::enable_if_t<UCL::is_cl_vector<T>::value, T> reference_ctz(T val) {
  T output{};
  unsigned i{0};
  for (const auto &element : val.s) {
    output.s[i++] = reference_ctz(element);
  }
  return output;
}

template <typename T>
class CtzTest : public ucl::CommandQueueTest {
 public:
  CtzTest() {}

  virtual void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::CommandQueueTest::SetUp());
    // ctz was introduced in OpenCL-2.0.
    if (!UCL::isDeviceVersionAtLeast({2, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    // Try and build the program.
    cl_int error_code{};
    program = clCreateProgramWithSource(context, 1, &kernel_source, nullptr,
                                        &error_code);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(error_code);
    auto device_version = ucl::Environment::instance->deviceOpenCLVersion;
    const std::string cl_std_option =
        "-cl-std=CL" + std::to_string(device_version.major()) + '.' +
        std::to_string(device_version.minor());
    const std::string type_definition =
        "-DTYPE=" + std::string{T::source_name()};
    const std::string compiler_options = cl_std_option + ' ' + type_definition;
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, compiler_options.c_str(),
                                  nullptr, nullptr));
    // Create the kernel.
    kernel = clCreateKernel(program, "test_ctz", &error_code);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(error_code);

    // Create the opencl buffers.
    input_buffer = clCreateBuffer(CommandQueueTest::context, CL_MEM_READ_ONLY,
                                  sizeof(T), nullptr, &error_code);
    EXPECT_TRUE(input_buffer);
    ASSERT_SUCCESS(error_code);
    output_buffer = clCreateBuffer(CommandQueueTest::context, CL_MEM_WRITE_ONLY,
                                   sizeof(T), nullptr, &error_code);
    EXPECT_TRUE(output_buffer);
    ASSERT_SUCCESS(error_code);

    // Set the kernel arguments.
    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(input_buffer),
                                  static_cast<void *>(&input_buffer)));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(output_buffer),
                                  static_cast<void *>(&output_buffer)));
  }

  template <typename Ty>
  std::enable_if_t<ucl::is_scalar<Ty>::value, Ty> generateRandomData() {
    return {
        getInputGenerator().template GenerateInt<typename Ty::value_type>()};
  }

  template <typename Ty>
  std::enable_if_t<ucl::is_vector<Ty>::value, Ty> generateRandomData() {
    using elementType = typename Ty::value_type;
    std::vector<elementType> buffer(Ty::size());
    getInputGenerator().template GenerateData<elementType>(buffer);
    return Ty{buffer};
  }

  virtual void TearDown() override {
    if (input_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
    }
    if (output_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  void executeSingleElementCtz(const T &input) {
    ASSERT_SUCCESS(clEnqueueWriteBuffer(CommandQueueTest::command_queue,
                                        input_buffer, CL_TRUE, 0, sizeof(T),
                                        &input, 0, nullptr, nullptr));
    ASSERT_SUCCESS(clEnqueueTask(CommandQueueTest::command_queue, kernel, 0,
                                 nullptr, nullptr));
    T result{};
    ASSERT_SUCCESS(clEnqueueReadBuffer(
        CommandQueueTest::command_queue, output_buffer, CL_TRUE, 0,
        sizeof(result), &result, 0, nullptr, nullptr));
    EXPECT_EQ(T(reference_ctz(input.value())), result);
  }

  cl_program program{nullptr};
  cl_kernel kernel{nullptr};
  cl_mem input_buffer{nullptr};
  cl_mem output_buffer{nullptr};
  const char *kernel_source =
      "void __kernel test_ctz(__global TYPE *input, __global TYPE *output)"
      "{"
      " int tid = get_global_id(0);"
      " output[tid] = ctz(input[tid]);"
      "}";
};

TYPED_TEST_SUITE_P(CtzTest);

TYPED_TEST_P(CtzTest, SingleIntger) {
  CtzTest<TypeParam>::executeSingleElementCtz(
      CtzTest<TypeParam>::template generateRandomData<TypeParam>());
}

REGISTER_TYPED_TEST_SUITE_P(CtzTest, SingleIntger);
#if !defined(__clang_analyzer__)
using IntegerTypes = testing::Types<
    ucl::Char, ucl::Char2, ucl::Char3, ucl::Char4, ucl::Char8, ucl::Char16,
    ucl::UChar, ucl::UChar2, ucl::UChar3, ucl::UChar4, ucl::UChar8,
    ucl::UChar16, ucl::Short, ucl::Short2, ucl::Short3, ucl::Short4,
    ucl::Short8, ucl::Short16, ucl::UShort, ucl::UShort2, ucl::UShort3,
    ucl::UShort4, ucl::UShort8, ucl::UShort16, ucl::Int, ucl::Int2, ucl::Int3,
    ucl::Int4, ucl::Int8, ucl::Int16, ucl::UInt, ucl::UInt2, ucl::UInt3,
    ucl::UInt4, ucl::UInt8, ucl::UInt16, ucl::Long, ucl::Long2, ucl::Long3,
    ucl::Long4, ucl::Long8, ucl::Long16, ucl::ULong, ucl::ULong2, ucl::ULong3,
    ucl::ULong4, ucl::ULong8, ucl::ULong16>;
using ScalarIntegerTypes =
    testing::Types<ucl::Char, ucl::UChar, ucl::Short, ucl::UShort, ucl::Int,
                   ucl::UInt, ucl::Long, ucl::ULong>;
#else
// Reduce the number of types to test if running clang analyzer (or
// clang-tidy), they'll all result in basically the same code but it takes a
// long time to analyze all of them.
using IntegerTypes = testing::Types<ucl::Int>;
using ScalarIntegerTypes = testing::Types<ucl::Int>;
#endif
INSTANTIATE_TYPED_TEST_SUITE_P(SingleIntger, CtzTest, IntegerTypes, );

template <typename T>
class CtzEdgeCaseTest : public CtzTest<T> {};
TYPED_TEST_SUITE_P(CtzEdgeCaseTest);
TYPED_TEST_P(CtzEdgeCaseTest, EdgeCase) {
  CtzTest<TypeParam>::executeSingleElementCtz(TypeParam(0x0));
  CtzTest<TypeParam>::executeSingleElementCtz(TypeParam(0x1));
  CtzTest<TypeParam>::executeSingleElementCtz(TypeParam(0x2));
  CtzTest<TypeParam>::executeSingleElementCtz(TypeParam(0x4));
  CtzTest<TypeParam>::executeSingleElementCtz(TypeParam(0x8));
  CtzTest<TypeParam>::executeSingleElementCtz(
      std::numeric_limits<TypeParam>::min());
  CtzTest<TypeParam>::executeSingleElementCtz(
      std::numeric_limits<TypeParam>::max());
}

REGISTER_TYPED_TEST_SUITE_P(CtzEdgeCaseTest, EdgeCase);
INSTANTIATE_TYPED_TEST_SUITE_P(EdgeCase, CtzEdgeCaseTest, ScalarIntegerTypes, );
