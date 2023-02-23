// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxGetSupportedQueryCountersTest : DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetSupportedQueryCountersTest);

TEST_P(muxGetSupportedQueryCountersTest, Default) {
  if (device->info->query_counter_support) {
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    ASSERT_GT(count, 0u);
    std::vector<mux_query_counter_t> counters(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(), nullptr,
                                                nullptr));
    std::vector<mux_query_counter_description_t> descriptions(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, nullptr,
                                                descriptions.data(), nullptr));
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(),
                                                descriptions.data(), nullptr));
  } else {
    uint32_t count;
    ASSERT_ERROR_EQ(mux_error_feature_unsupported,
                    muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                 0, nullptr, nullptr, &count));
  }
}

TEST_P(muxGetSupportedQueryCountersTest, InvalidDevice) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  uint32_t count;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxGetSupportedQueryCounters(nullptr, mux_queue_type_compute,
                                               0, nullptr, nullptr, &count));
  mux_device_s invalid_device = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetSupportedQueryCounters(&invalid_device, mux_queue_type_compute, 0,
                                   nullptr, nullptr, &count));
}

TEST_P(muxGetSupportedQueryCountersTest, InvalidQueueType) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  uint32_t count;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxGetSupportedQueryCounters(
                      device, static_cast<mux_queue_type_e>(0xFFFFFFFF), 0,
                      nullptr, nullptr, &count));
}

TEST_P(muxGetSupportedQueryCountersTest, InvalidCount) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_t counter;
  mux_query_counter_description_t description;
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxGetSupportedQueryCounters(device, mux_queue_type_compute, 0, &counter,
                                   &description, nullptr));
}

TEST_P(muxGetSupportedQueryCountersTest, NullOutPointer) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                               0, nullptr, nullptr, nullptr));
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                               1, nullptr, nullptr, nullptr));
}
