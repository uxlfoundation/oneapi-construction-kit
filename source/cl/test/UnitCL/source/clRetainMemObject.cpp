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

class clRetainMemObjectTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

TEST_F(clRetainMemObjectTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, clRetainMemObject(nullptr));
  ASSERT_SUCCESS(clRetainMemObject(buffer));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}
