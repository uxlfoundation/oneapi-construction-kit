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

TEST(clGetPlatformIDs, Default) {
  cl_uint num_platforms;

  ASSERT_SUCCESS(clGetPlatformIDs(0, nullptr, &num_platforms));
  ASSERT_GT(num_platforms, 0u);

  UCL::Buffer<cl_platform_id> platforms(num_platforms);

  ASSERT_SUCCESS(clGetPlatformIDs(num_platforms, platforms, nullptr));

  for (cl_uint i = 0; i < num_platforms; i++) {
    ASSERT_TRUE(platforms[i]);
  }
}

TEST(clGetPlatformIDs, ZeroPlatformsRequestedWithNonNullPlatforms) {
  cl_platform_id platform;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(0, &platform, nullptr));
}

TEST(clGetPlatformIDs, PlatformsRequestedWithNullPlatforms) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(1, nullptr, nullptr));
}

TEST(clGetPlatformIDs, AllValuesNull) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetPlatformIDs(0, nullptr, nullptr));
}
