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

#include "common.h"

struct muxDestroyQueryPoolTest : DeviceTest {
  mux_queue_t queue = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyQueryPoolTest);

TEST_P(muxDestroyQueryPoolTest, DefaultDuration) {
  mux_query_pool_t query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1, nullptr,
                                    allocator, &query_pool));
  muxDestroyQueryPool(queue, query_pool, allocator);
}

TEST_P(muxDestroyQueryPoolTest, DefaultCounter) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_t counter;
  ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute, 1,
                                              &counter, nullptr, nullptr));
  const mux_query_counter_config_t enabled_counter = {counter.uuid, nullptr};
  mux_query_pool_t query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter, 1,
                                    &enabled_counter, allocator, &query_pool));
  muxDestroyQueryPool(queue, query_pool, allocator);
}

TEST_P(muxDestroyQueryPoolTest, InvalidQueue) {
  mux_query_pool_t query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1, nullptr,
                                    allocator, &query_pool));
  // Expect these so the query_pool still gets destroyed on failure.
  muxDestroyQueryPool(nullptr, query_pool, allocator);
  mux_queue_t invalid_queue = {};
  muxDestroyQueryPool(invalid_queue, query_pool, allocator);
  // Actually destroy the query_pool.
  muxDestroyQueryPool(queue, query_pool, allocator);
}

TEST_P(muxDestroyQueryPoolTest, InvalidQueryPool) {
  muxDestroyQueryPool(queue, nullptr, allocator);
  mux_query_pool_t invalid_query_pool = {};
  muxDestroyQueryPool(queue, invalid_query_pool, allocator);
}
