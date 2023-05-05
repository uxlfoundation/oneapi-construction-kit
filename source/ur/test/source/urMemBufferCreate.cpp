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

using urMemBufferCreateTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urMemBufferCreateTest);

TEST_P(urMemBufferCreateTest, Success) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                   nullptr, &buffer));
  ASSERT_NE(nullptr, buffer);
  ASSERT_SUCCESS(urMemRelease(buffer));
}

TEST_P(urMemBufferCreateTest, InvalidNullHandleContext) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urMemBufferCreate(nullptr, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, &buffer));
}

TEST_P(urMemBufferCreateTest, InvalidEnumerationFlags) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                   urMemBufferCreate(context, UR_MEM_FLAG_FORCE_UINT32, 4096,
                                     nullptr, &buffer));
}

// TODO: The spec states that hostPtr is non-optional but this seems like a bug,
// the user shouldn't have to pass a host pointer if they don't intend to use
// one. This test has been disabled until this is resolved.
TEST_P(urMemBufferCreateTest, DISABLED_InvalidNullPointerHostPtr) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, &buffer));
}

TEST_P(urMemBufferCreateTest, InvalidNullPointerBuffer) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, nullptr));
}
