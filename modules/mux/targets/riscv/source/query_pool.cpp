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

#include "riscv/query_pool.h"

#include "riscv/device.h"

mux_result_t riscvGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count) {
  return riscv::query_pool_s::getSupportedQueryCounters(
      static_cast<riscv::device_s *>(device), queue_type, count, out_counters,
      out_descriptions, out_count);
}

mux_result_t riscvCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool) {
  auto query_pool = riscv::query_pool_s::create<riscv::query_pool_s>(
      queue, query_type, query_count, query_configs, allocator_info);
  if (!query_pool) {
    return query_pool.error();
  }
  *out_query_pool = *query_pool;
  return mux_success;
}

void riscvDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                           mux_allocator_info_t allocator_info) {
  riscv::query_pool_s::destroy<riscv::query_pool_s>(
      queue, static_cast<riscv::query_pool_s *>(query_pool), allocator_info);
}

mux_result_t riscvGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count) {
  auto required_passes = riscv::query_pool_s::getQueryCounterRequiredPasses(
      queue, query_count, query_counter_configs);
  if (!required_passes) {
    return required_passes.error();
  }
  *out_pass_count = required_passes.value();
  return mux_success;
}

mux_result_t riscvGetQueryPoolResults(mux_queue_t queue,
                                      mux_query_pool_t query_pool,
                                      uint32_t query_index,
                                      uint32_t query_count, size_t size,
                                      void *data, size_t stride) {
  return static_cast<riscv::query_pool_s *>(query_pool)
      ->getQueryPoolResults(queue, query_index, query_count, size, data,
                            stride);
}
