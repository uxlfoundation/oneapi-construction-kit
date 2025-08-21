// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

class clReleaseSamplerTest : public ucl::ContextTest {
protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clReleaseSamplerTest, InvalidSampler) {
  ASSERT_EQ_ERRCODE(CL_INVALID_SAMPLER, clReleaseSampler(nullptr));
}

TEST_F(clReleaseSamplerTest, Default) {
  cl_int status;
  cl_sampler sampler = clCreateSampler(context, CL_FALSE, CL_ADDRESS_NONE,
                                       CL_FILTER_NEAREST, &status);
  EXPECT_TRUE(sampler);
  ASSERT_SUCCESS(status);
  ASSERT_SUCCESS(clReleaseSampler(sampler));
}
