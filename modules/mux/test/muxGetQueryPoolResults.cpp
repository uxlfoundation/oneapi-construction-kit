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

#include <cstdint>

#include "common.h"
#include "mux/mux.h"

struct muxGetQueryPoolResultsDurationTest : DeviceCompilerTest {
  mux_queue_t queue = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  mux_query_pool_t query_pool = nullptr;
  mux_fence_t fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1,
                                      nullptr, allocator, &query_pool));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
  }

  void TearDown() override {
    if (nullptr != fence) {
      muxDestroyFence(device, fence, allocator);
    }
    if (nullptr != queue) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    if (nullptr != command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }
    DeviceCompilerTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetQueryPoolResultsDurationTest);

TEST_P(muxGetQueryPoolResultsDurationTest, Default) {
  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, 0, 1, 0,
                                      nullptr, nullptr));
  uint64_t store = 0;
  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *user_data) {
        auto &store = *static_cast<uint64_t *>(user_data);
        store = 42;
      },
      &store, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, 0, 1, 0,
                                    nullptr, nullptr));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0, nullptr,
                             0, nullptr, nullptr));
  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
  ASSERT_EQ(42, store);

  mux_query_duration_result_s duration;
  ASSERT_SUCCESS(muxGetQueryPoolResults(
      queue, query_pool, 0, 1, sizeof(mux_query_duration_result_s), &duration,
      sizeof(mux_query_duration_result_s)));
  ASSERT_NE(0, duration.start);
  ASSERT_NE(0, duration.end);
  ASSERT_LE(duration.start, duration.end);
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidQueue) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(nullptr, query_pool, 0, 1,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s)));
  mux_queue_s invalid_queue = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(&invalid_queue, query_pool, 0, 1,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidQueryPool) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxGetQueryPoolResults(
                      queue, nullptr, 0, 1, sizeof(mux_query_duration_result_s),
                      &duration, sizeof(mux_query_duration_result_s)));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, &invalid_query_pool, 0, 1,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidQueryIndex) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, 1, 1,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidQueryCount) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, 0, 2,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidSize) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, 0, 2,
                             sizeof(mux_query_duration_result_s) - 1, &duration,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidData) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, 0, 1,
                             sizeof(mux_query_duration_result_s), nullptr,
                             sizeof(mux_query_duration_result_s)));
}

TEST_P(muxGetQueryPoolResultsDurationTest, InvalidStride) {
  mux_query_duration_result_s duration;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, 0, 1,
                             sizeof(mux_query_duration_result_s), &duration,
                             sizeof(mux_query_duration_result_s) - 1));
}

struct muxGetQueryPoolResultsCounterTest : DeviceCompilerTest {
  mux_queue_t queue = nullptr;
  uint32_t query_index = 0;
  uint32_t query_count = 1;
  std::vector<mux_query_counter_t> counters;
  std::vector<mux_query_counter_description_t> descriptions;
  mux_query_counter_config_t config;
  mux_query_pool_t query_pool = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  mux_executable_t executable = nullptr;
  mux_kernel_t kernel = nullptr;
  mux_fence_t fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    counters.resize(count);
    descriptions.resize(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(),
                                                descriptions.data(), nullptr));

    // Enable the first counter.
    config = {counters[query_index].uuid, nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &config, allocator,
                                      &query_pool));

    // Create a kernel workload for profiling.
    const char *nop_opencl_c = "kernel void nop() {}";
    ASSERT_SUCCESS(createMuxExecutable(nop_opencl_c, &executable));
    ASSERT_SUCCESS(muxCreateKernel(device, executable, "nop", strlen("nop"),
                                   allocator, &kernel));
    const size_t global_offset = 0;
    const size_t global_size = 8;
    size_t local_size[3] = {1, 1, 1};

    mux_ndrange_options_t nd_range_options{};
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = &global_offset;
    nd_range_options.global_size = &global_size;
    nd_range_options.dimensions = 1;

    // Execute the kernel.
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, 0, 1, 0,
                                        nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options,
                                     0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, 0, 1, 0,
                                      nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0,
                               nullptr, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
  }

  void TearDown() override {
    if (device && !IsSkipped()) {
      muxDestroyKernel(device, kernel, allocator);
      muxDestroyExecutable(device, executable, allocator);
      muxDestroyFence(device, fence, allocator);
      muxDestroyCommandBuffer(device, command_buffer, allocator);
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    DeviceCompilerTest::TearDown();
  }

  void PrintCounterValue(mux_query_counter_result_s &result) {
    switch (counters[query_index].storage) {
      case mux_query_counter_result_type_int32:
        std::cout << result.int32 << "\n";
        break;
      case mux_query_counter_result_type_int64:
        std::cout << result.int64 << "\n";
        break;
      case mux_query_counter_result_type_uint32:
        std::cout << result.uint32 << "\n";
        break;
      case mux_query_counter_result_type_uint64:
        std::cout << result.uint64 << "\n";
        break;
      case mux_query_counter_result_type_float32:
        std::cout << result.float32 << "\n";
        break;
      case mux_query_counter_result_type_float64:
        std::cout << result.float64 << "\n";
        break;
    }
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetQueryPoolResultsCounterTest);

TEST_P(muxGetQueryPoolResultsCounterTest, Default) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_SUCCESS(
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
  std::cout << "       name: " << descriptions[query_index].name << "\n";
  std::cout << "   category: " << descriptions[query_index].category << "\n";
  std::cout << "description: " << descriptions[query_index].description << "\n";
  std::cout << "      value: ";
  PrintCounterValue(result);
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidQueue) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(nullptr, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
  mux_queue_s invalid_queue = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(&invalid_queue, query_pool, query_index,
                             query_count, sizeof(mux_query_counter_result_s),
                             &result, sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidQueryPool) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, nullptr, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, &invalid_query_pool, query_index,
                             query_count, sizeof(mux_query_counter_result_s),
                             &result, sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidQueryIndex) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index + 1, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidQueryCount) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count + 1,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidSize) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s) - 1, &result,
                             sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidData) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), nullptr,
                             sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsCounterTest, InvalidStride) {
  if (!device->info->query_counter_support) {
    GTEST_SKIP();
  }
  mux_query_counter_result_s result;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s) - 1));
}

struct muxGetQueryPoolResultsMultiplePoolsCounterTest
    : muxGetQueryPoolResultsCounterTest {
  mux_query_pool_t query_pool_b = nullptr;

  void SetUp() override {
    // Don't run `muxGetQueryPoolResultsCounterTest::SetUp` as it records and
    // dispatches command buffers.
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    counters.resize(count);
    descriptions.resize(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(),
                                                descriptions.data(), nullptr));

    // Enable the first counter.
    config = {counters[query_index].uuid, nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &config, allocator,
                                      &query_pool));
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &config, allocator,
                                      &query_pool_b));

    // Create a kernel workload for profiling.
    const char *nop_opencl_c = "kernel void nop() {}";
    ASSERT_SUCCESS(createMuxExecutable(nop_opencl_c, &executable));
    ASSERT_SUCCESS(muxCreateKernel(device, executable, "nop", strlen("nop"),
                                   allocator, &kernel));
    const size_t global_offset = 0;
    const size_t global_size = 8;
    size_t local_size[3] = {1, 1, 1};

    mux_ndrange_options_t nd_range_options{};
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = &global_offset;
    nd_range_options.global_size = &global_size;
    nd_range_options.dimensions = 1;

    // Execute the kernel twice, use one query pool to read counters from one,
    // and another query pool to read the values from both.
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, 0, 1, 0,
                                        nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options,
                                     0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool_b, 0, 1, 0,
                                        nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options,
                                     0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool_b, 0, 1, 0,
                                      nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, 0, 1, 0,
                                      nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0,
                               nullptr, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
  }

  void TearDown() override {
    if (query_pool_b) {
      muxDestroyQueryPool(queue, query_pool_b, allocator);
    }
    muxGetQueryPoolResultsCounterTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxGetQueryPoolResultsMultiplePoolsCounterTest);

TEST_P(muxGetQueryPoolResultsMultiplePoolsCounterTest, Default) {
  mux_query_counter_result_s result;
  mux_query_counter_result_s result_b;

  ASSERT_SUCCESS(
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result,
                             sizeof(mux_query_counter_result_s)));
  ASSERT_SUCCESS(
      muxGetQueryPoolResults(queue, query_pool_b, query_index, query_count,
                             sizeof(mux_query_counter_result_s), &result_b,
                             sizeof(mux_query_counter_result_s)));
  std::cout << "       name: " << descriptions[query_index].name << "\n";
  std::cout << "   category: " << descriptions[query_index].category << "\n";
  std::cout << "description: " << descriptions[query_index].description << "\n";
  std::cout << "      value a (two kernels): ";
  PrintCounterValue(result);
  std::cout << "      value b (one kernel): ";
  PrintCounterValue(result_b);
}

struct muxGetQueryPoolResultsMultipleCountersCounterTest
    : muxGetQueryPoolResultsCounterTest {
  void SetUp() override {
    // Don't run `muxGetQueryPoolResultsCounterTest::SetUp` as it records and
    // dispatches command buffers.
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    counters.resize(count);
    descriptions.resize(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(),
                                                descriptions.data(), nullptr));

    // We can't test multiple counters if we don't have multiple counters.
    if (counters.size() < 2) {
      GTEST_SKIP();
    }

    // Enable an arbitrary non-trivial but modest (to keep the run time down on
    // platforms where counters incur a significant perf hit) number of
    // counters.
    std::vector<mux_query_counter_config_t> counters_to_enable;
    const uint32_t max_hw_counters = device->info->max_hardware_counters;
    // Obviously we can't try for more counters than there are on the system.
    const uint32_t target_counters_to_enable =
        std::min(static_cast<uint32_t>(counters.size()), 4u);
    uint32_t hw_counters_used = 0;

    // Walk through all the available counters and try to fill our list with
    // the target number of counters, while respecting hardware limits. This
    // is designed such that the worst case is we end up with a list of one
    // counter (if the platform's counters and hardware limits line up that
    // way).
    for (size_t i = 0; i < counters.size(); i++) {
      auto &counter = counters[i];
      if (hw_counters_used + counter.hardware_counters <= max_hw_counters) {
        counters_to_enable.push_back({counter.uuid, nullptr});
        descriptions_enabled.push_back(descriptions[i]);
        hw_counters_used += counter.hardware_counters;
      }

      if (counters_to_enable.size() == target_counters_to_enable) {
        break;
      }
    }

    query_count = counters_to_enable.size();

    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, counters_to_enable.data(),
                                      allocator, &query_pool));

    // Create a kernel workload for profiling.
    const char *nop_opencl_c = "kernel void nop() {}";
    ASSERT_SUCCESS(createMuxExecutable(nop_opencl_c, &executable));
    ASSERT_SUCCESS(muxCreateKernel(device, executable, "nop", strlen("nop"),
                                   allocator, &kernel));
    const size_t global_offset = 0;
    const size_t global_size = 8;
    size_t local_size[3] = {1, 1, 1};

    mux_ndrange_options_t nd_range_options{};
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = &global_offset;
    nd_range_options.global_size = &global_size;
    nd_range_options.dimensions = 1;

    // Execute the kernel twice, use one query pool to read counters from one,
    // and another query pool to read the values from both.
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &fence));
    ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer, query_pool, 0, 1, 0,
                                        nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options,
                                     0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandEndQuery(command_buffer, query_pool, 0, 1, 0,
                                      nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_buffer, fence, nullptr, 0,
                               nullptr, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, fence));
  }

  std::vector<mux_query_counter_description_t> descriptions_enabled;

  void TearDown() override { muxGetQueryPoolResultsCounterTest::TearDown(); }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(
    muxGetQueryPoolResultsMultipleCountersCounterTest);

TEST_P(muxGetQueryPoolResultsMultipleCountersCounterTest, Default) {
  std::vector<mux_query_counter_result_s> results(query_count);

  ASSERT_SUCCESS(muxGetQueryPoolResults(
      queue, query_pool, query_index, query_count,
      sizeof(mux_query_counter_result_s) * results.size(), results.data(),
      sizeof(mux_query_counter_result_s)));
  for (size_t i = 0; i < query_count; i++) {
    std::cout << "       name: " << descriptions_enabled[i].name << "\n";
    std::cout << "   category: " << descriptions_enabled[i].category << "\n";
    std::cout << "description: " << descriptions_enabled[i].description << "\n";
    std::cout << "      values: ";
    PrintCounterValue(results[i]);
  }
}

TEST_P(muxGetQueryPoolResultsMultipleCountersCounterTest, IndexOffset) {
  auto result_count = query_count - 1;
  auto query_index = 1;

  std::vector<mux_query_counter_result_s> results(result_count);

  ASSERT_SUCCESS(muxGetQueryPoolResults(
      queue, query_pool, query_index, result_count,
      sizeof(mux_query_counter_result_s) * result_count, results.data(),
      sizeof(mux_query_counter_result_s)));
  for (size_t i = 0; i < result_count; i++) {
    std::cout << "       name: " << descriptions_enabled[query_index + i].name
              << "\n";
    std::cout << "   category: "
              << descriptions_enabled[query_index + i].category << "\n";
    std::cout << "description: "
              << descriptions_enabled[query_index + i].description << "\n";
    std::cout << "      values: ";
    PrintCounterValue(results[i]);
  }
}

TEST_P(muxGetQueryPoolResultsMultipleCountersCounterTest, InvalidIndexOffset) {
  std::vector<mux_query_counter_result_s> results(query_count);

  // Try to read from an invalid index.
  query_index = query_count;

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), results.data(),
                             sizeof(mux_query_counter_result_s)));
}

TEST_P(muxGetQueryPoolResultsMultipleCountersCounterTest, InvalidQueryCount) {
  std::vector<mux_query_counter_result_s> results(query_count);

  // Try to read starting from the last query, but request two results.
  query_index = query_count - 1;
  query_count = 2;

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxGetQueryPoolResults(queue, query_pool, query_index, query_count,
                             sizeof(mux_query_counter_result_s), results.data(),
                             sizeof(mux_query_counter_result_s)));
}
