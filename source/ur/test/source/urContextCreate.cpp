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

#include "uur/fixtures.h"

using urContextCreateTest = uur::DeviceTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urContextCreateTest);

TEST_P(urContextCreateTest, Success) {
  ur_context_handle_t context = nullptr;
  ASSERT_SUCCESS(urContextCreate(1, &device, nullptr, &context));
  ASSERT_NE(nullptr, context);
  ASSERT_SUCCESS(urContextRelease(context));
}

TEST_P(urContextCreateTest, InvalidNullPointerDevices) {
  ur_context_handle_t context = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urContextCreate(1, nullptr, nullptr, &context));
}

TEST_P(urContextCreateTest, InvalidNullPointerContext) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urContextCreate(1, &device, nullptr, nullptr));
}
