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

#include "ur_api.h"
#include "uur/checks.h"

struct urTearDownTest : testing::Test {
  void SetUp() override {
    const ur_device_init_flags_t device_flags = 0;
    ASSERT_SUCCESS(urInit(device_flags));
  }
};

TEST_F(urTearDownTest, Success) {
  ur_tear_down_params_t tear_down_params{};
  ASSERT_SUCCESS(urTearDown(&tear_down_params));
}

TEST_F(urTearDownTest, InvalidNullPointerParams) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER, urTearDown(nullptr));
}
