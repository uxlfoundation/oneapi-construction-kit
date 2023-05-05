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
