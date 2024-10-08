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

/// @file
///
/// @brief HAL base implementation of the mux_query_pool_s object.

#ifndef MUX_HAL_QUERY_POOL_H_INCLUDED
#define MUX_HAL_QUERY_POOL_H_INCLUDED

#include <cassert>
#include <cstring>

#include "cargo/expected.h"
#include "mux/hal/device.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
struct query_pool : mux_query_pool_s {
  /// @brief Default constructor.
  query_pool() = default;

  query_pool(const query_pool &) = delete;
  query_pool &operator=(const query_pool &) = delete;

  /// @brief Create a new query pool object.
  ///
  /// @tparam QueryPool
  ///
  /// @param query_type Type of results the query pool will store.
  /// @param query_count Number of `uint64_t` query slots to allocate.
  /// @param allocator Mux allocator used for allocations.
  ///
  /// @return Returns a newly constructed query pool on success, or
  /// `mux_error_out_of_memory` on failure.
  // TODO(CA-4313): Use mux::hal::queue once its been ported.
  template <class QueryPool>
  static cargo::expected<QueryPool *, mux_result_t> create(
      mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
      const mux_query_counter_config_t *query_configs,
      mux::allocator allocator) {
    (void)queue;
    (void)query_configs;
    switch (query_type) {
      case mux_query_type_duration:
      case mux_query_type_counter:
        break;
      default:
        return cargo::make_unexpected(mux_error_invalid_value);
    }
    // Calculate the result storage offset past the end of the query_pool_s.
    // FIXME: This wastes sizeof(mux_query_duration_result_s) bytes when
    // sizeof(query_pool_s) has no remainder when divided by it.
    const size_t query_data_offset =
        sizeof(query_pool) + sizeof(mux_query_duration_result_s) -
        (sizeof(query_pool) % sizeof(mux_query_duration_result_s));
    // Calculate the total size of the allocation.
    const size_t query_size = sizeof(mux_query_duration_result_s) * query_count;
    const size_t alloc_size = query_data_offset + query_size;
    // Using a single allocation with storage for the query pool results
    // appended to the end of the query_pool_s.
    auto memory = allocator.alloc(
        alloc_size,
        std::max(alignof(query_pool), alignof(mux_query_duration_result_s)));
    // Construct the query_pool.
    auto query_pool = new (memory) mux::hal::query_pool();
    if (!query_pool) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    // Initialize the parent mux_query_pool_s data members.
    query_pool->type = query_type;
    query_pool->count = query_count;
    query_pool->data = static_cast<uint8_t *>(memory) + query_data_offset;
    query_pool->size = query_size;
    // Finally reset the result storage to zeros ready for use.
    query_pool->reset();
    return query_pool;
  }

  // TODO(CA-4313): Use mux::hal::queue once its been ported.
  template <class QueryPool>
  static void destroy(mux_queue_t queue, QueryPool *query_pool,
                      mux::allocator allocator) {
    if (query_pool->type == mux_query_type_counter) {
      static_cast<mux::hal::device *>(queue->device)
          ->profiler.clear_accumulator(query_pool->counter_accumulator_id);
    }
    allocator.destroy(query_pool);
  }

  /// @see muxGetSupportedQueryCounters
  // TODO(CA-4313): Use mux::hal::queue once its been ported.
  static mux_result_t getSupportedQueryCounters(
      mux::hal::device *device, mux_queue_type_e queue_type, uint32_t count,
      mux_query_counter_t *out_counters,
      mux_query_counter_description_t *out_descriptions, uint32_t *out_count);

  /// @see muxGetQueryCounterRequiredPasses
  // TODO(CA-4313): Use mux::hal::queue once its been ported.
  static cargo::expected<uint32_t, mux_result_t> getQueryCounterRequiredPasses(
      mux_queue_t queue, uint32_t query_count,
      const mux_query_counter_config_t *query_counter_configs);

  /// @see muxGetQueryPoolResults
  // TODO(CA-4313): Use mux::hal::queue once its been ported.
  mux_result_t getQueryPoolResults(mux_queue_t queue, uint32_t query_index,
                                   uint32_t query_count, size_t size,
                                   void *data, size_t stride);

  /// @brief Get the duration query at the given index.
  ///
  /// @param index Index of the query to get.
  ///
  /// @return Returns a pointer to the duration query storage.
  mux_query_duration_result_t getDurationQueryAt(uint32_t index) {
    assert(this->type == mux_query_type_duration &&
           "type must be mux_query_type_duration");
    if (index >= count) {
      return nullptr;
    }
    return static_cast<mux_query_duration_result_t>(this->data) + index;
  }

  /// @brief Reset the query pool result storage to zeros.
  void reset() { std::memset(this->data, 0, this->size); }

  /// @brief Reset a region of the query pool result storage to zeros.
  ///
  /// @param[in] offset The offset in bytes into the data to reset.
  /// @param[in] size The size in bytes of data to reset.
  void reset(size_t offset, size_t size) {
    std::memset(static_cast<uint8_t *>(this->data) + offset, 0, size);
  }

  /// @brief The HAL counter accumulator ID associated with this pool (if
  /// any).
  uint32_t counter_accumulator_id;
  /// @brief Pointer to memory used to store query result data.
  void *data;
  /// @brief Size in bytes of memory pointed to by `data`.
  size_t size;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_QUERY_POOL_H_INCLUDED
