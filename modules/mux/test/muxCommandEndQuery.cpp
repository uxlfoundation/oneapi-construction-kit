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

struct muxCommandEndQueryDurationTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  mux_query_pool_t query_pool = nullptr;
  const uint32_t query_index = 0;
  const uint32_t query_count = 1;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration,
                                      query_count, nullptr, allocator,
                                      &query_pool));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                        query_count, 0, nullptr, nullptr));
  }

  void TearDown() override {
    if (query_pool) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    if (command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandEndQueryDurationTest);

TEST_P(muxCommandEndQueryDurationTest, Default) {
  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryDurationTest, InvalidCommandBuffer) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandEndQuery(nullptr, query_pool, query_index,
                                     query_count, 0, nullptr, nullptr));
  mux_command_buffer_s invalid_command_buffer = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(&invalid_command_buffer, query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryDurationTest, InvalidQueryPool) {
  mux_query_pool_t other_query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1, nullptr,
                                    allocator, &other_query_pool));
  EXPECT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, other_query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
  muxDestroyQueryPool(queue, other_query_pool, allocator);

  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 0, nullptr, nullptr));
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandEndQuery(command_buffer, nullptr, query_index,
                                     query_count, 0, nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, &invalid_query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryDurationTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, query_pool, query_index + 1,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryDurationTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandEndQuery(command_buffer, query_pool, query_index,
                                     query_count + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryDurationTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 1, &wait, nullptr));
}

struct muxCommandEndQueryCounterTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  std::vector<mux_query_counter_t> counters;
  mux_query_pool_t query_pool = nullptr;
  const uint32_t query_index = 0;
  const uint32_t query_count = 1;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    counters.resize(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(), nullptr,
                                                nullptr));
    const mux_query_counter_config_t counter_config = {
        counters[query_index].uuid, nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &counter_config, allocator,
                                      &query_pool));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                        query_count, 0, nullptr, nullptr));
  }

  void TearDown() override {
    if (query_pool) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    if (command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandEndQueryCounterTest);

TEST_P(muxCommandEndQueryCounterTest, Default) {
  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryCounterTest, InvalidCommandBuffer) {
  mux_command_buffer_t invalid_command_buffer = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(invalid_command_buffer, query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryCounterTest, InvalidQueryPool) {
  const mux_query_counter_config_t counter_config = {counters[query_index].uuid,
                                                     nullptr};
  mux_query_pool_t other_query_pool;
  ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter, query_count,
                                    &counter_config, allocator,
                                    &other_query_pool));
  EXPECT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, other_query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
  muxDestroyQueryPool(queue, other_query_pool, allocator);

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandEndQuery(command_buffer, nullptr, query_index,
                                     query_count, 0, nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, &invalid_query_pool, query_index,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryCounterTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandEndQuery(command_buffer, query_pool, query_index + 1,
                         query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryCounterTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandEndQuery(command_buffer, query_pool, query_index,
                                     query_count + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandEndQueryCounterTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, query_index,
                                    query_count, 1, &wait, nullptr));
}
