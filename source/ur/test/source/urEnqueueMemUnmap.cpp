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

struct urEnqueueMemUnmapTest : public uur::MemBufferQueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(uur::MemBufferQueueTest::SetUp());
    ASSERT_SUCCESS(urEnqueueMemBufferMap(
        queue, buffer, true, UR_MAP_FLAG_READ | UR_MAP_FLAG_WRITE, 0, size, 0,
        nullptr, nullptr, (void **)&map));
  };

  void TearDown() override { uur::MemBufferQueueTest::TearDown(); }

  uint32_t *map = nullptr;
};
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueMemUnmapTest);

TEST_P(urEnqueueMemUnmapTest, Success) {
  ASSERT_SUCCESS(urEnqueueMemUnmap(queue, buffer, map, 0, nullptr, nullptr));
  EXPECT_SUCCESS(urQueueFinish(queue));
}

TEST_P(urEnqueueMemUnmapTest, SuccessOffset) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urEnqueueMemUnmap(nullptr, buffer, map, 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemUnmapTest, SuccessPartialMap) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemUnmap(queue, nullptr, map, 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemUnmapTest, InvalidNullHandleQueue) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urEnqueueMemUnmap(queue, buffer, nullptr, 0, nullptr, nullptr));
}
