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

#include "common.h"

struct muxGetQueryCounterRequiredPassesTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_query_pool_t query_pool = nullptr;
  mux_query_counter_config_t counter_config = {};

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    mux_query_counter_t counter;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                1, &counter, nullptr, nullptr));
    counter_config.uuid = counter.uuid;
    counter_config.data = nullptr;
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter, 1,
                                      &counter_config, allocator, &query_pool));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetQueryCounterRequiredPassesTest);

TEST_P(muxGetQueryCounterRequiredPassesTest, Default) {
  uint32_t pass_count;
  ASSERT_SUCCESS(
      muxGetQueryCounterRequiredPasses(queue, 1, &counter_config, &pass_count));
  ASSERT_GE(1u, pass_count);
}

TEST_P(muxGetQueryCounterRequiredPassesTest, InvalidQueue) {
  uint32_t pass_count;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxGetQueryCounterRequiredPasses(nullptr, 1, &counter_config,
                                                   &pass_count));
  mux_queue_s invalid_queue = {};
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxGetQueryCounterRequiredPasses(
                      &invalid_queue, 1, &counter_config, &pass_count));
}

TEST_P(muxGetQueryCounterRequiredPassesTest, InvalidQueryCount) {
  uint32_t pass_count;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryCounterRequiredPasses(queue, 0, &counter_config, &pass_count));
}

TEST_P(muxGetQueryCounterRequiredPassesTest, InvalidQueryCounterConfigs) {
  uint32_t pass_count;
  ASSERT_ERROR_EQ(mux_error_invalid_value, muxGetQueryCounterRequiredPasses(
                                               queue, 1, nullptr, &pass_count));
}

TEST_P(muxGetQueryCounterRequiredPassesTest, NullOutPassCount) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxGetQueryCounterRequiredPasses(queue, 1, &counter_config, nullptr));
}
