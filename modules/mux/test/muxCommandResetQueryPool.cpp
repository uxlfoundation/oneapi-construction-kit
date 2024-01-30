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

struct muxCommandResetQueryPoolDurationTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
  mux_query_pool_t query_pool = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration, 1,
                                      nullptr, allocator, &query_pool));
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

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandResetQueryPoolDurationTest);

TEST_P(muxCommandResetQueryPoolDurationTest, Default) {
  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool, 0, 1, 0,
                                          nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolDurationTest, InvalidCommandBuffer) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandResetQueryPool(nullptr, query_pool, 0, 1, 0, nullptr, nullptr));
  mux_command_buffer_s invalid_command_buffer = {};
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(&invalid_command_buffer, query_pool,
                                           0, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolDurationTest, InvalidQueryPool) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(command_buffer, nullptr, 0, 1, 0,
                                           nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(command_buffer, &invalid_query_pool,
                                           0, 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolDurationTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(command_buffer, query_pool, 1, 1, 0,
                                           nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolDurationTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(command_buffer, query_pool, 0, 2, 0,
                                           nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolDurationTest, Sync) {
  mux_sync_point_t wait = nullptr;
  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool, 0, 1, 0,
                                          nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool, 0, 1, 1,
                                          &wait, nullptr));
}

struct muxCommandResetQueryPoolCounterTest : DeviceTest {
  mux_queue_t queue = nullptr;
  mux_command_buffer_t command_buffer = nullptr;
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
    std::vector<mux_query_counter_t> counters(count);
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                count, counters.data(), nullptr,
                                                nullptr));
    const mux_query_counter_config_t counter_config = {counters.front().uuid,
                                                       nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter,
                                      query_count, &counter_config, allocator,
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

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandResetQueryPoolCounterTest);

TEST_P(muxCommandResetQueryPoolCounterTest, Default) {
  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool,
                                          query_index, query_count, 0, nullptr,
                                          nullptr));
}

TEST_P(muxCommandResetQueryPoolCounterTest, InvalidCommandBuffer) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(nullptr, query_pool, query_index,
                                           query_count, 0, nullptr, nullptr));
  mux_command_buffer_s invalid_command_buffer = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandResetQueryPool(&invalid_command_buffer, query_pool, query_index,
                               query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolCounterTest, InvalidQueryPool) {
  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCommandResetQueryPool(command_buffer, nullptr, query_index,
                                           query_count, 0, nullptr, nullptr));
  mux_query_pool_s invalid_query_pool = {};
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandResetQueryPool(command_buffer, &invalid_query_pool, query_index,
                               query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolCounterTest, InvalidQueryIndex) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandResetQueryPool(command_buffer, query_pool, query_index + 1,
                               query_count, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolCounterTest, InvalidQueryCount) {
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCommandResetQueryPool(command_buffer, query_pool, query_index,
                               query_count + 1, 0, nullptr, nullptr));
}

TEST_P(muxCommandResetQueryPoolCounterTest, Sync) {
  mux_sync_point_t wait = nullptr;
  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool, 0, 1, 0,
                                          nullptr, &wait));
  ASSERT_NE(wait, nullptr);

  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer, query_pool, 0, 1, 1,
                                          &wait, nullptr));
}

struct muxCommandResetQueryPoolCounterReuseTest : DeviceCompilerTest {
  mux_query_pool_t query_pool = nullptr;
  mux_query_counter_t counter = {};
  mux_executable_t executable = nullptr;
  mux_kernel_t kernel = nullptr;
  mux_queue_t queue = nullptr;
  mux_command_buffer_t work_command_buffer = nullptr;
  mux_command_buffer_t reset_command_buffer = nullptr;
  // TODO: We could perhaps use a single fence here and reset it between the
  // waits.
  mux_fence_t work_fence = nullptr;
  mux_fence_t reset_fence = nullptr;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    if (!device->info->query_counter_support) {
      GTEST_SKIP();
    }

    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    uint32_t count;
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                0, nullptr, nullptr, &count));
    ASSERT_SUCCESS(muxGetSupportedQueryCounters(device, mux_queue_type_compute,
                                                1, &counter, nullptr, nullptr));

    // Enable the first counter.
    const mux_query_counter_config_t config = {counter.uuid, nullptr};
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_counter, 1, &config,
                                      allocator, &query_pool));

    ASSERT_SUCCESS(muxCreateCommandBuffer(device, callback, allocator,
                                          &work_command_buffer));
    ASSERT_SUCCESS(muxCreateCommandBuffer(device, callback, allocator,
                                          &reset_command_buffer));

    ASSERT_SUCCESS(muxCreateFence(device, allocator, &work_fence));
    ASSERT_SUCCESS(muxCreateFence(device, allocator, &reset_fence));

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

    // Execute our workload to get some readings in the query pool (at least
    // potentially, without specifically choosing a counter on a
    // per-implementation basis there isn't any guarantee that we'll measure
    // anything).
    ASSERT_SUCCESS(muxCommandBeginQuery(work_command_buffer, query_pool, 0, 1,
                                        0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandNDRange(work_command_buffer, kernel,
                                     nd_range_options, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxCommandEndQuery(work_command_buffer, query_pool, 0, 1, 0,
                                      nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, work_command_buffer, work_fence, nullptr,
                               0, nullptr, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, work_fence));
  }

  void TearDown() override {
    if (query_pool) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }
    if (kernel) {
      muxDestroyKernel(device, kernel, allocator);
    }
    if (executable) {
      muxDestroyExecutable(device, executable, allocator);
    }
    if (work_fence) {
      muxDestroyFence(device, work_fence, allocator);
    }
    if (reset_fence) {
      muxDestroyFence(device, reset_fence, allocator);
    }
    if (work_command_buffer) {
      muxDestroyCommandBuffer(device, work_command_buffer, allocator);
    }
    if (reset_command_buffer) {
      muxDestroyCommandBuffer(device, reset_command_buffer, allocator);
    }
    DeviceCompilerTest::TearDown();
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCommandResetQueryPoolCounterReuseTest);

TEST_P(muxCommandResetQueryPoolCounterReuseTest, Default) {
  ASSERT_SUCCESS(muxCommandResetQueryPool(reset_command_buffer, query_pool, 0,
                                          1, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxDispatch(queue, reset_command_buffer, reset_fence, nullptr,
                             0, nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, reset_fence));

  // Run the command buffer with the kernel in it again to make sure the query
  // pool was left in a usable state after the reset.
  ASSERT_SUCCESS(muxDispatch(queue, work_command_buffer, work_fence, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxTryWait(queue, UINT64_MAX, work_fence));

  // Try reading the value out to make sure no weird invalidation happened or
  // something. We can't really validate the value because counter behaviour is
  // entirely implementation defined.
  mux_query_counter_result_s result = {};
  ASSERT_SUCCESS(muxGetQueryPoolResults(
      queue, query_pool, 0, 1, sizeof(mux_query_counter_result_s), &result,
      sizeof(mux_query_counter_result_s)));
}
