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

class clIcdGetPlatformIDsKHRTest : public ucl::PlatformTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    if (!isPlatformExtensionSupported("cl_khr_icd")) {
      GTEST_SKIP();
    }
    size_t size;
    ASSERT_SUCCESS(
        clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, nullptr, &size));
    std::string extensionString(size, '\0');
    ASSERT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
                                     extensionString.length(),
                                     extensionString.data(), nullptr));
    clIcdGetPlatformIDsKHRPtr = reinterpret_cast<clIcdGetPlatformIDsKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clIcdGetPlatformIDsKHR"));
    ASSERT_TRUE(clIcdGetPlatformIDsKHRPtr);
  }

  clIcdGetPlatformIDsKHR_fn clIcdGetPlatformIDsKHRPtr = nullptr;
};

TEST_F(clIcdGetPlatformIDsKHRTest, Default) {
  cl_uint num_platforms;

  ASSERT_SUCCESS(clIcdGetPlatformIDsKHRPtr(0, nullptr, &num_platforms));
  ASSERT_GT(num_platforms, 0u);

  UCL::Buffer<cl_platform_id> platforms(num_platforms);

  ASSERT_SUCCESS(clIcdGetPlatformIDsKHRPtr(num_platforms, platforms, nullptr));

  for (cl_uint i = 0; i < num_platforms; i++) {
    ASSERT_TRUE(platforms[i]);
  }
}

TEST_F(clIcdGetPlatformIDsKHRTest, ZeroPlatformsRequestedWithNonNullPlatforms) {
  cl_platform_id platform;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clIcdGetPlatformIDsKHRPtr(0, &platform, nullptr));
}

TEST_F(clIcdGetPlatformIDsKHRTest, PlatformsRequestedWithNullPlatforms) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clIcdGetPlatformIDsKHRPtr(1, nullptr, nullptr));
}

TEST_F(clIcdGetPlatformIDsKHRTest, AllValuesNull) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clIcdGetPlatformIDsKHRPtr(0, nullptr, nullptr));
}
