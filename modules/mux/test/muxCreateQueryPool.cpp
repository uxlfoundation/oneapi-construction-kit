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

struct muxCreateQueryPoolTest : DeviceTest {
  mux_queue_t queue = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateQueryPoolTest);

TEST_P(muxCreateQueryPoolTest, DefaultDuration) {
  mux_query_pool_t query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1, nullptr,
                                    allocator, &query_pool));
  muxDestroyQueryPool(queue, query_pool, allocator);
}

TEST_P(muxCreateQueryPoolTest, DefaultCounter) {
  if (device->info->query_counter_support) {
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    ASSERT_GE(count, 1u);
    std::vector<mux_query_counter_t> counters;
    std::vector<mux_query_counter_description_t> descriptions;
    counters.reserve(count);
    descriptions.reserve(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(),
                                                descriptions.data(), nullptr));
    // Enable the first counter only.
    mux_query_counter_config_t query_counter_config;
    query_counter_config.uuid = counters.front().uuid;
    mux_query_pool_t query_pool;
    ASSERT_ERROR_EQ(
        mux_success,
        muxCreateQueryPool(queue, mux_query_type_counter, 1,
                           &query_counter_config, allocator, &query_pool));
    muxDestroyQueryPool(queue, query_pool, allocator);
  } else {
    const mux_query_counter_config_t query_counter_config = {};
    mux_query_pool_t query_pool;
    ASSERT_ERROR_EQ(
        mux_error_feature_unsupported,
        muxCreateQueryPool(queue, mux_query_type_counter, 1,
                           &query_counter_config, allocator, &query_pool));
  }
}

TEST_P(muxCreateQueryPoolTest, InvalidDevice) {
  mux_query_pool_t query_pool;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateQueryPool(nullptr, mux_query_type_duration, 1,
                                     nullptr, allocator, &query_pool));
  mux_queue_s invalid_queue = {};
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateQueryPool(&invalid_queue, mux_query_type_duration, 1,
                                     nullptr, allocator, &query_pool));
}

TEST_P(muxCreateQueryPoolTest, InvalidQueryType) {
  mux_query_pool_t query_pool;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateQueryPool(queue, static_cast<mux_query_type_e>(0xFFFFFFFF), 1,
                         nullptr, allocator, &query_pool));
}

TEST_P(muxCreateQueryPoolTest, InvalidQueryCount) {
  mux_query_pool_t query_pool;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateQueryPool(queue, mux_query_type_duration, 0, nullptr,
                                     allocator, &query_pool));
}

TEST_P(muxCreateQueryPoolTest, InvalidOutQueryPool) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateQueryPool(queue, mux_query_type_duration, 1, nullptr,
                                     allocator, nullptr));
}
