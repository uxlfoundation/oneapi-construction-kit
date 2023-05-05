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

class clRetainEventTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    event = clCreateUserEvent(context, &errorcode);
    EXPECT_TRUE(event);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    ContextTest::TearDown();
  }

  cl_event event = nullptr;
};

TEST_F(clRetainEventTest, Default) {
  EXPECT_EQ_ERRCODE(CL_INVALID_EVENT, clRetainEvent(nullptr));
  ASSERT_SUCCESS(clRetainEvent(event));
  ASSERT_SUCCESS(clReleaseEvent(event));
}
