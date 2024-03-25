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

#include <cargo/small_vector.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include <algorithm>

#include "Common.h"

struct clGetPlatformInfoTest : ucl::PlatformTest,
                               testing::WithParamInterface<cl_platform_info> {};

TEST_P(clGetPlatformInfoTest, HaveSizes) {
  size_t size;
  ASSERT_SUCCESS(clGetPlatformInfo(platform, GetParam(), 0, nullptr, &size));
  ASSERT_GT(size, 0u);
}

TEST_P(clGetPlatformInfoTest, EnsureReturnedStringsHaveRightSizes) {
  size_t size;
  EXPECT_SUCCESS(clGetPlatformInfo(platform, GetParam(), 0, nullptr, &size));

  UCL::Buffer<char> buffer(size);

  ASSERT_SUCCESS(
      clGetPlatformInfo(platform, GetParam(), size, buffer, nullptr));
  ASSERT_EQ(size, strlen(buffer) + 1);  // + 1 for the nullptr Terminator
}

TEST_P(clGetPlatformInfoTest, EnsureReturnedStringsAreNullTerminated) {
  size_t size;
  EXPECT_SUCCESS(clGetPlatformInfo(platform, GetParam(), 0, nullptr, &size));

  UCL::Buffer<char> buffer(size);

  ASSERT_SUCCESS(
      clGetPlatformInfo(platform, GetParam(), size, buffer, nullptr));
  ASSERT_EQ('\0', buffer[size - 1]);
}

TEST_F(clGetPlatformInfoTest, InvalidPlatformInfo) {
  size_t size;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetPlatformInfo(platform, 0, 0, nullptr, &size));
}

TEST_P(clGetPlatformInfoTest, InvalidValueSize) {
  size_t size;
  EXPECT_SUCCESS(clGetPlatformInfo(platform, GetParam(), 0, nullptr, &size));

  UCL::Buffer<char> buffer(size);

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetPlatformInfo(
          platform, GetParam(), size - 1, buffer,
          nullptr));  // size - 1 so that it fails with not right size
}

static cl_platform_info param_infos[] = {
    CL_PLATFORM_PROFILE, CL_PLATFORM_VERSION, CL_PLATFORM_NAME,
    CL_PLATFORM_VENDOR, CL_PLATFORM_EXTENSIONS};

INSTANTIATE_TEST_CASE_P(clGetPlatformInfo, clGetPlatformInfoTest,
                        ::testing::ValuesIn(param_infos));

TEST_F(clGetPlatformInfoTest, VerifyPlatformVersion) {
  size_t version_string_size{};
  ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 0, nullptr,
                                   &version_string_size));
  std::string version_string(version_string_size, '\0');
  ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_VERSION,
                                   version_string.size(), version_string.data(),
                                   nullptr));
  ASSERT_TRUE(UCL::verifyOpenCLVersionString(version_string))
      << "Malformed platform OpenCL version, must be of form "
         "\"OpenCL<space><major_version.minor_version>\"";
}

#if defined(CL_VERSION_3_0)
struct clGetPlatformInfoTestOpenCL30
    : ucl::PlatformTest,
      testing::WithParamInterface<std::tuple<size_t, int>> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetPlatformInfoTestOpenCL30, CheckSizeQuerySucceeds) {
  // Get the enumaration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(
      clGetPlatformInfo(platform, query_enum_value, 0, nullptr, &size));
}

TEST_P(clGetPlatformInfoTestOpenCL30, CheckSizeQueryIsCorrect) {
  // Get the enumaration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(
      clGetPlatformInfo(platform, query_enum_value, 0, nullptr, &size));
  // Get the correct size of the query.
  auto value_size_in_bytes = std::get<0>(GetParam());
  // Check the queried value is correct.
  EXPECT_EQ(size, value_size_in_bytes);
}

TEST_P(clGetPlatformInfoTestOpenCL30, CheckQuerySucceeds) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetPlatformInfo(platform, query_enum_value,
                                   value_buffer.size(), value_buffer.data(),
                                   nullptr));
}

TEST_P(clGetPlatformInfoTestOpenCL30, CheckIncorrectSizeQueryFails) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetPlatformInfo(platform, query_enum_value, value_buffer.size() - 1,
                        value_buffer.data(), nullptr));
}

INSTANTIATE_TEST_CASE_P(
    PlatformQuery, clGetPlatformInfoTestOpenCL30,
    ::testing::Values(
        std::make_tuple(sizeof(cl_version), CL_PLATFORM_NUMERIC_VERSION),
        std::make_tuple(sizeof(cl_ulong), CL_PLATFORM_HOST_TIMER_RESOLUTION)),
    [](const testing::TestParamInfo<clGetPlatformInfoTestOpenCL30::ParamType>
           &info) {
      return UCL::platformQueryToString(std::get<1>(info.param));
    });

TEST_F(clGetPlatformInfoTestOpenCL30, VerifyNumericVersion) {
  // Query for the string platform version.
  size_t platform_version_string_size{};
  ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 0, nullptr,
                                   &platform_version_string_size));
  std::string platform_version_string(platform_version_string_size, '\0');
  ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_VERSION,
                                   platform_version_string.size(),
                                   platform_version_string.data(), nullptr));

  // Query for the value.
  cl_version numeric_version{};
  EXPECT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_NUMERIC_VERSION,
                                   sizeof(numeric_version), &numeric_version,
                                   nullptr));

  // Numeric version is required by spec. to match string version.
  int major_version{}, minor_version{};
  auto version = UCL::parseOpenCLVersionString(platform_version_string);
  ASSERT_TRUE(version);
  std::tie(major_version, minor_version) = *version;
  EXPECT_EQ(CL_VERSION_MAJOR_KHR(numeric_version), major_version)
      << "Major version mismatch";
  EXPECT_EQ(CL_VERSION_MINOR_KHR(numeric_version), minor_version)
      << "Minor version mismatch";
  // Patch versions are not included in the platform version.
}

TEST_F(clGetPlatformInfoTest, EXTENSIONS_WITH_VERSION) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isPlatformVersion("3.")) {
    GTEST_SKIP();
  }

  // Query for size of value.
  // Output is an array so can't check for correct size.
  size_t size{};
  ASSERT_SUCCESS(clGetPlatformInfo(
      platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION, 0, nullptr, &size));

  // Query for the value.
  UCL::Buffer<char> value_buffer{size};
  EXPECT_SUCCESS(
      clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                        value_buffer.size(), value_buffer.data(), nullptr));

  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer_too_small{size - 1};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                        value_buffer_too_small.size(),
                        value_buffer_too_small.data(), nullptr));
}

TEST_F(clGetPlatformInfoTest, ValidateExtensionsWithVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isPlatformVersion("3.")) {
    GTEST_SKIP();
  }

  // First query for the CL_PLATFORM_EXTENSIONS to get extensions listed
  // as a space separated string.
  size_t platform_extentions_size{};
  ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, nullptr,
                                   &platform_extentions_size));
  UCL::Buffer<char> platform_extentions_buffer(platform_extentions_size);
  EXPECT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
                                   platform_extentions_buffer.size(),
                                   platform_extentions_buffer.data(), nullptr));

  // Now query for the CL_PLATFORM_EXTENSIONS_WITH_VERSION to get extensions
  // as an array of cl_version objects.
  size_t platform_extentions_with_version_size{};
  ASSERT_SUCCESS(
      clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION, 0,
                        nullptr, &platform_extentions_with_version_size));
  cargo::small_vector<cl_name_version_khr, 4>
      platform_extentions_with_version{};
  auto error = platform_extentions_with_version.assign(
      platform_extentions_with_version_size /
          sizeof(decltype(platform_extentions_with_version)::value_type),
      {});
  ASSERT_EQ(error, cargo::success) << "Error: out of memory";
  ASSERT_SUCCESS(clGetPlatformInfo(
      platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
      sizeof(decltype(platform_extentions_with_version)::value_type) *
          platform_extentions_with_version.size(),
      platform_extentions_with_version.data(), nullptr));

  // The list of extensions reported in the array must math the list in the
  // space separated string.

  // Much easier to work with a string view into the space separated string from
  // now on
  const cargo::string_view platform_extentions(platform_extentions_buffer);
  // Construct an array of strings so we can easily traverse the space separated
  // list.
  auto split_extensions = cargo::split(platform_extentions, " ");
  // Check that the lists have the same size for an early exit.
  ASSERT_EQ(split_extensions.size(), platform_extentions_with_version.size());
  // Construct second array of strings from versioned extensions.
  std::vector<cargo::string_view> split_version_extensions{};
  for (const auto &ext : platform_extentions_with_version) {
    split_version_extensions.push_back(ext.name);
  }

  // Sort the two lists and compare to ensure that up to ordering, they are the
  // same.
  std::sort(split_extensions.begin(), split_extensions.end());
  std::sort(split_version_extensions.begin(), split_version_extensions.end());
  ASSERT_EQ(split_extensions, split_version_extensions);
}
#endif
