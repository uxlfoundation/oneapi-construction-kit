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

struct MacroTestParams {
  const char *condition;
  const char *options;
  const ucl::Version minimum_version;
};

static std::ostream &operator<<(std::ostream &out,
                                const MacroTestParams &params) {
  out << "MacroTestParams{.condition{\"" << params.condition
      << "\"}, .options{\"" << params.options << "\"}, .minimum_version{"
      << params.minimum_version << "}}";
  return out;
}

struct MacrosTest : ucl::CommandQueueTest,
                    testing::WithParamInterface<MacroTestParams> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    if (!UCL::isDeviceVersionAtLeast(GetParam().minimum_version)) {
      GTEST_SKIP();
    }

    cl_int status;
    const char *sources[] = {"kernel void foo(global char *out) {\n",
                             "#if ",
                             GetParam().condition,
                             "\n",
                             "  *out = 1;\n",
                             "#else\n",
                             "  *out = 0;\n",
                             "#endif\n",
                             "}\n"};

    const size_t sources_length = sizeof(sources) / sizeof(sources[0]);

    program = clCreateProgramWithSource(context, sources_length, sources,
                                        nullptr, &status);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(status);

    ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, GetParam().options,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "foo", &status);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(status);

    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 1, nullptr, &status);
    ASSERT_SUCCESS(status);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&buffer)));
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_mem buffer = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

TEST_P(MacrosTest, Default) {
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, nullptr));

  cl_char result{};

  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0, 1,
                                     &result, 0, nullptr, nullptr));

  const auto &param = GetParam();
  ASSERT_TRUE(result) << "condition: " << param.condition
                      << ", options: " << param.options
                      << ", minimum version: " << param.minimum_version.major()
                      << "." << param.minimum_version.minor();
}

INSTANTIATE_TEST_SUITE_P(
    Macros, MacrosTest,
    ::testing::Values(
        MacroTestParams{"!defined(NULL)", "-cl-std=CL1.2", ucl::Version{1, 2}},
        MacroTestParams{"defined(NULL)", "-cl-std=CL3.0", ucl::Version{3, 0}},
        MacroTestParams{"defined(__FILE__)", "", ucl::Version{1, 2}},
        MacroTestParams{"defined(__LINE__) && (__LINE__ == 2)", "",
                        ucl::Version{1, 2}},
        MacroTestParams{"defined(__OPENCL_VERSION__)", "", ucl::Version{1, 2}},
        MacroTestParams{"(__OPENCL_VERSION__ == CL_VERSION_1_2)",
                        "-cl-std=CL1.2", ucl::Version{1, 2}},
        MacroTestParams{"defined(__OPENCL_VERSION__)", "-cl-std=CL3.0",
                        ucl::Version{3, 0}},
        MacroTestParams{"(__OPENCL_VERSION__ == 300)", "-cl-std=CL3.0",
                        ucl::Version{3, 0}},
        MacroTestParams{"defined(CL_VERSION_1_0)", "", ucl::Version{1, 2}},
        MacroTestParams{"defined(CL_VERSION_1_1)", "", ucl::Version{1, 2}},
        MacroTestParams{"defined(CL_VERSION_1_2)", "", ucl::Version{1, 2}},
        /* These !version checks are pretty ComputeAorta specific, another
         * OpenCL 2.x implementation should in theory pass UnitCL but will fail
         * these tests.  A future version of ComputeAorta may fail these tests
         * (they can be changed at such a time). */
        MacroTestParams{"!defined(CL_VERSION_2_0)", "", ucl::Version{1, 2}},
        MacroTestParams{"!defined(CL_VERSION_2_1)", "", ucl::Version{1, 2}},
        MacroTestParams{"!defined(CL_VERSION_2_2)", "", ucl::Version{1, 2}},
        MacroTestParams{"defined(__OPENCL_C_VERSION__)", "",
                        ucl::Version{1, 2}},
        MacroTestParams{"(__OPENCL_C_VERSION__ == CL_VERSION_1_1)",
                        "-cl-std=CL1.1", ucl::Version{1, 2}},
        MacroTestParams{"(__OPENCL_C_VERSION__ == CL_VERSION_1_2)",
                        "-cl-std=CL1.2", ucl::Version{1, 2}},
        MacroTestParams{"(__OPENCL_C_VERSION__ == CL_VERSION_3_0)",
                        "-cl-std=CL3.0", ucl::Version{3, 0}},
        MacroTestParams{
            "!defined(__ENDIAN_LITTLE__) || (__ENDIAN_LITTLE__ == 1)", "",
            ucl::Version{1, 2}},
        MacroTestParams{"defined(__kernel_exec)", "", ucl::Version{1, 2}},
        MacroTestParams{"defined(kernel_exec)", "", ucl::Version{1, 2}},
        MacroTestParams{
            "!defined(__IMAGE_SUPPORT__) || (__IMAGE_SUPPORT__ == 1)", "",
            ucl::Version{1, 2}},
        MacroTestParams{"!defined(__FAST_RELAXED_MATH__)", "",
                        ucl::Version{1, 2}},
        MacroTestParams{
            "defined(__FAST_RELAXED_MATH__) && (__FAST_RELAXED_MATH__ == 1)",
            "-cl-fast-relaxed-math", ucl::Version{1, 2}},
        MacroTestParams{
            "!defined(__EMBEDDED_PROFILE__) || (__EMBEDDED_PROFILE__ == 1)", "",
            ucl::Version{1, 2}}));

template <cl_device_info device_option>
class OptionalMacrosTest : public MacrosTest {
 protected:
  OptionalMacrosTest() : MacrosTest() {}

  void Test() {
    // Concievable that these could go out of date with future header versions,
    // but that should be immediately obvious.
    static_assert(CL_DEVICE_TYPE < device_option,
                  "Device option outside of valid range");
    static_assert(CL_DEVICE_PRINTF_BUFFER_SIZE >= device_option,
                  "Device option outside of valid range");

    ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, nullptr));

    cl_char result;
    ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0, 1,
                                       &result, 0, nullptr, nullptr));

    cl_bool device_option_supported;
    ASSERT_SUCCESS(clGetDeviceInfo(device, device_option, sizeof(cl_bool),
                                   &device_option_supported, NULL));
    if (device_option_supported) {
      ASSERT_TRUE(result);
    } else {
      ASSERT_FALSE(result);
    }
  }
};

#define TEST_P_OPTIONAL(NAME, DEVICE_INFO)      \
  using NAME = OptionalMacrosTest<DEVICE_INFO>; \
  TEST_P(NAME, Default) { Test(); }

TEST_P_OPTIONAL(EndianMacrosTest, CL_DEVICE_ENDIAN_LITTLE)
TEST_P_OPTIONAL(ImageMacrosTest, CL_DEVICE_IMAGE_SUPPORT)

INSTANTIATE_TEST_CASE_P(
    EndianMacros, EndianMacrosTest,
    ::testing::Values(MacroTestParams{
        "defined(__ENDIAN_LITTLE__) && (__ENDIAN_LITTLE__ == 1)", "",
        ucl::Version{1, 2}}));

INSTANTIATE_TEST_CASE_P(
    ImageMacros, ImageMacrosTest,
    ::testing::Values(MacroTestParams{
        "defined(__IMAGE_SUPPORT__) && (__IMAGE_SUPPORT__ == 1)", "",
        ucl::Version{1, 2}}));
