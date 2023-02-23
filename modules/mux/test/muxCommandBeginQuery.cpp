// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxCommandBeginQueryDurationTest : DeviceTest {
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

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandBeginQueryDurationTest);

TEST_P(muxCommandBeginQueryDurationTest, Default) {
  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryDurationTest, InvalidCommandBuffer) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(nullptr, query_pool, query_index,
                                       query_count, 0, nullptr, nullptr));
  mux_command_buffer_s invalid_command_buffer = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(&invalid_command_buffer, query_pool, query_index,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryDurationTest, InvalidQueryPool) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(command_buffer, nullptr, query_index,
                                       query_count, 0, nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(command_buffer, &invalid_query_pool, query_index,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryDurationTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(command_buffer, query_pool, query_index + 1,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryDurationTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                       query_count + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryDurationTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 1, &wait, nullptr));
}

struct muxCommandBeginQueryCounterTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_query_pool_t query_pool = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  const uint32_t query_index = 0;
  const uint32_t query_count = 1;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    std::vector<mux_query_counter_t> counters(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(), nullptr,
                                                nullptr));
    mux_query_counter_config_t counter_config = {counters[query_index].uuid,
                                                 nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &counter_config, allocator,
                                      &query_pool));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  }

  void TearDown() override {
    if (command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    if (query_pool) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    DeviceTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandBeginQueryCounterTest);

TEST_P(muxCommandBeginQueryCounterTest, Default) {
  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryCounterTest, InvalidCommandBuffer) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(nullptr, query_pool, query_index,
                                       query_count, 0, nullptr, nullptr));
  mux_command_buffer_s invalid_command_buffer = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(&invalid_command_buffer, query_pool, query_index,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryCounterTest, InvalidQueryPool) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(command_buffer, nullptr, query_index,
                                       query_count, 0, nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(command_buffer, &invalid_query_pool, query_index,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryCounterTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandBeginQuery(command_buffer, query_pool, query_index + 1,
                           query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryCounterTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                       query_count + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandBeginQueryCounterTest, Sync) {
  mux_sync_point_t wait = nullptr;

  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 0, nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, query_index,
                                      query_count, 1, &wait, nullptr));
}
