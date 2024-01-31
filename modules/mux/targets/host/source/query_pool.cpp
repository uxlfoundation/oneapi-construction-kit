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

#include <host/device.h>
#include <host/query_pool.h>

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
#include <host/papi_error_codes.h>
#include <papi.h>
#endif

cargo::expected<host::query_pool_s *, mux_result_t> host::query_pool_s::create(
    mux_query_type_e query_type, uint32_t query_count, mux::allocator allocator,
    const mux_query_counter_config_t *query_configs, mux_queue_t queue) {
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  auto host_device = static_cast<host::device_s *>(queue->device);
  auto thread_count = host_device->thread_pool.initialized_threads;
#endif
  // Calculate the result storage offset past the end of the query_pool_s.
  // FIXME: This wastes sizeof(mux_query_duration_result_s) bytes when
  // sizeof(query_pool_s) has no remainder when divided by it.
  size_t query_data_offset = 0;
  if (query_type == mux_query_type_duration) {
    query_data_offset =
        sizeof(query_pool_s) + sizeof(mux_query_duration_result_s) -
        sizeof(query_pool_s) % sizeof(mux_query_duration_result_s);
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  } else if (query_type == mux_query_type_counter) {
    query_data_offset = sizeof(query_pool_s) + sizeof(host_papi_event_info_s) -
                        sizeof(query_pool_s) % sizeof(host_papi_event_info_s);
#endif
  }
  // Calculate the total size of the allocation.
  size_t query_size = 0;
  size_t query_align = 0;
  if (query_type == mux_query_type_duration) {
    query_size = sizeof(mux_query_duration_result_s) * query_count;
    query_align = alignof(mux_query_duration_result_s);
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  } else if (query_type == mux_query_type_counter) {
    query_size = sizeof(host_papi_event_info_s) * thread_count;
    query_align = alignof(host_papi_event_info_s);
#endif
  }
  const size_t alloc_size = query_data_offset + query_size;
  // Using a single allocation with storage for the query pool results appended
  // to the end of the query_pool_s.
  auto memory =
      allocator.alloc(alloc_size, std::max(alignof(query_pool_s), query_align));
  if (!memory) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }
  // Construct the query_pool.
  auto query_pool = new (memory) query_pool_s();
  // Initialize the parent mux_query_pool_s data members.
  query_pool->type = query_type;
  query_pool->count = query_count;
  query_pool->data = static_cast<uint8_t *>(memory) + query_data_offset;
  query_pool->size = query_size;
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  if (query_type == mux_query_type_counter) {
    // Initialize the query pool's array view.
    auto event_info_begin =
        static_cast<host_papi_event_info_s *>(query_pool->data);
    query_pool->papi_event_infos = cargo::array_view<host_papi_event_info_s>(
        event_info_begin, event_info_begin + thread_count);
    // Create and store a `host_papi_event_info_s` for each worker thread.
    for (size_t thread_index = 0; thread_index < thread_count; thread_index++) {
      auto thread_id = host_device->thread_pool.pool[thread_index].get_id();
      host_papi_event_info_s event_info = {
          PAPI_NULL,
          host_device->thread_pool.thread_ids[thread_id],
          {},
          nullptr};
      // Each `host_papi_event_info_s` wraps a papi event set.
      int papi_result = PAPI_create_eventset(&event_info.papi_event_set);
      if (papi_result != PAPI_OK) {
        return cargo::make_unexpected(getMuxResult(papi_result));
      }

      // Create an allocation for each event set to read its results out into.
      event_info.result_buffer = static_cast<host_query_counter_result_s *>(
          allocator.alloc(sizeof(host_query_counter_result_s) * query_count));
      if (!event_info.result_buffer) {
        return cargo::make_unexpected(mux_error_out_of_memory);
      }
      event_info.results = cargo::array_view<host_query_counter_result_s>(
          event_info.result_buffer, event_info.result_buffer + query_count);

      // Add each requested counter to this event set.
      for (uint32_t query_index = 0; query_index < query_count; query_index++) {
        papi_result = PAPI_add_event(event_info.papi_event_set,
                                     query_configs[query_index].uuid);
        if (papi_result != PAPI_OK) {
          return cargo::make_unexpected(getMuxResult(papi_result));
        }

        // Get the added event's data type and set the disriminator on its
        // corresponding result struct.
        PAPI_event_info_t papi_event_info{};
        papi_result = PAPI_get_event_info(query_configs[query_index].uuid,
                                          &papi_event_info);
        if (papi_result != PAPI_OK) {
          return cargo::make_unexpected(getMuxResult(papi_result));
        }

        auto storage_type = getMuxStorageType(papi_event_info.data_type);
        if (!storage_type.has_value()) {
          return cargo::make_unexpected(mux_error_invalid_value);
        }
        event_info.results[query_index].storage = storage_type.value();
      }

      // Attach the event set to the worker thread ID it is associated with.
      papi_result =
          PAPI_attach(event_info.papi_event_set, event_info.thread_id);

      if (papi_result != PAPI_OK) {
        return cargo::make_unexpected(getMuxResult(papi_result));
      }

      query_pool->papi_event_infos[thread_index] = std::move(event_info);
    }
  }
#endif
  // Finally reset the result storage to zeros ready for use.
  query_pool->reset();
  return query_pool;
}

void host::query_pool_s::reset() {
  if (type == mux_query_type_duration) {
    std::memset(this->data, 0, this->size);
  }
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  if (type == mux_query_type_counter) {
    for (auto &event : papi_event_infos) {
      // These can't be read from until they've been written again, so just
      // indiscriminately writing to the uint64 member won't cause UB unless
      // something else goes horribly wrong (at which point union-related UB
      // is presumably the least of our problems).
      for (auto &result : event.results) {
        result.uint64 = 0;
      }
      // Reset internal counters for our event sets.
      PAPI_reset(event.papi_event_set);
    }
  }
#endif
}

void host::query_pool_s::reset(size_t offset, size_t count) {
  if (type == mux_query_type_duration) {
    std::memset(static_cast<uint8_t *>(this->data) +
                    offset * sizeof(mux_query_duration_result_s),
                0, count * sizeof(mux_query_duration_result_s));
  }
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  if (type == mux_query_type_counter) {
    for (auto &event : papi_event_infos) {
      for (size_t i = offset; i < count; i++) {
        event.results[i].uint64 = 0;
      }
      // Reset internal counters for our event sets.
      PAPI_reset(event.papi_event_set);
    }
  }
#endif
}

#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
void host::query_pool_s::startEvents() {
  for (const auto &event_info : papi_event_infos) {
    PAPI_start(event_info.papi_event_set);
  }
}

void host::query_pool_s::endEvents() {
  // `PAPI_stop` takes a `long long*` to write results out into, we then copy
  // the results from that into each `event_info`'s result buffer.
  cargo::small_vector<long long, 8> papi_values_out;
  if (papi_values_out.resize(count)) {
    assert(false && "Couldn't allocate result memory!");
    return;
  }
  for (auto &event_info : papi_event_infos) {
    PAPI_stop(event_info.papi_event_set, papi_values_out.data());
    for (size_t i = 0; i < event_info.results.size(); i++) {
      auto &result = event_info.results[i];
      switch (result.storage) {
        case mux_query_counter_result_type_int32:
          result.int32 = static_cast<int32_t>(papi_values_out[i]);
          break;
        case mux_query_counter_result_type_int64:
          result.int64 = static_cast<int64_t>(papi_values_out[i]);
          break;
        case mux_query_counter_result_type_uint32:
          result.uint32 = static_cast<uint32_t>(papi_values_out[i]);
          break;
        case mux_query_counter_result_type_uint64:
          result.uint64 = static_cast<uint64_t>(papi_values_out[i]);
          break;
        case mux_query_counter_result_type_float32:
          result.float32 = static_cast<float>(papi_values_out[i]);
          break;
        case mux_query_counter_result_type_float64:
          result.float64 = static_cast<double>(papi_values_out[i]);
          break;
      }
    }
    std::fill_n(papi_values_out.begin(), papi_values_out.size(), 0);
  }
}

void host::query_pool_s::freeEvents(mux::allocator &allocator) {
  for (auto &event_info : papi_event_infos) {
    allocator.free(event_info.result_buffer);
    PAPI_destroy_eventset(&event_info.papi_event_set);
  }
}

mux_result_t host::query_pool_s::readPapiResults(
    mux_query_counter_result_s *results, size_t result_count,
    size_t query_index) {
  for (size_t result_index = 0; result_index < result_count; result_index++) {
    // Before we accumulate the results from all the worker threads, zero out
    // the output result struct. We can just check the first `event_info`'s
    // type for the appropriate query index, they should all have the same
    // storage type for a given index.
    switch (papi_event_infos[0].results[query_index + result_index].storage) {
      case mux_query_counter_result_type_int32:
        results[result_index].int32 = 0;
        break;
      case mux_query_counter_result_type_int64:
        results[result_index].int64 = 0;
        break;
      case mux_query_counter_result_type_uint32:
        results[result_index].uint32 = 0;
        break;
      case mux_query_counter_result_type_uint64:
        results[result_index].uint64 = 0;
        break;
      case mux_query_counter_result_type_float32:
        results[result_index].float32 = 0;
        break;
      case mux_query_counter_result_type_float64:
        results[result_index].float64 = 0;
        break;
    }

    // Accumulate the results from each worker thread's event info into the
    // output buffer, factoring in the query index we were requested to start
    // from.
    for (const auto &event_info : papi_event_infos) {
      auto &result = event_info.results[query_index + result_index];
      switch (event_info.results[query_index + result_index].storage) {
        case mux_query_counter_result_type_int32:
          results[result_index].int32 += result.int32;
          break;
        case mux_query_counter_result_type_int64:
          results[result_index].int64 += result.int64;
          break;
        case mux_query_counter_result_type_uint32:
          results[result_index].uint32 += result.uint32;
          break;
        case mux_query_counter_result_type_uint64:
          results[result_index].uint64 += result.uint64;
          break;
        case mux_query_counter_result_type_float32:
          results[result_index].float32 += result.float32;
          break;
        case mux_query_counter_result_type_float64:
          results[result_index].float64 += result.float64;
          break;
      }
    }
  }
  return mux_success;
}

#endif

mux_result_t hostGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count) {
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  auto host_device_info = static_cast<host::device_info_s *>(device->info);
  if (out_count) {
    *out_count = static_cast<uint32_t>(host_device_info->papi_counters.size());
  }

  // We only need to enter to loop if we have either of the out buffers.
  if (out_counters || out_descriptions) {
    for (uint32_t i = 0; i < count; i++) {
      if (out_counters) {
        host_device_info->papi_counters[i].populateMuxQueryCounter(
            &out_counters[i]);
      }
      if (out_descriptions) {
        host_device_info->papi_counters[i].populateMuxQueryCounterDescription(
            &out_descriptions[i]);
      }
    }
  }

  return mux_success;
#else
  (void)device;
  (void)queue_type;
  (void)count;
  (void)out_counters;
  (void)out_descriptions;
  (void)out_count;
  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool) {
  cargo::expected<mux_query_pool_t, mux_result_t> query_pool_or_error =
      host::query_pool_s::create(query_type, query_count, allocator_info,
                                 query_configs, queue);
  if (!query_pool_or_error.has_value()) {
    return query_pool_or_error.error();
  }

  *out_query_pool = query_pool_or_error.value();
  return mux_success;
}

void hostDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                          mux_allocator_info_t allocator_info) {
  (void)queue;
  mux::allocator allocator(allocator_info);
  auto host_query_pool = static_cast<host::query_pool_s *>(query_pool);
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  host_query_pool->freeEvents(allocator);
#endif
  allocator.destroy(host_query_pool);
}

mux_result_t hostGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count) {
  (void)queue;
  (void)query_counter_configs;
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
  for (uint32_t i = 0; i < query_count; i++) {
    out_pass_count[i] = 1;
  }
  return mux_success;
#else
  (void)query_count;
  (void)out_pass_count;
  return mux_error_feature_unsupported;
#endif
}

mux_result_t hostGetQueryPoolResults(mux_queue_t queue,
                                     mux_query_pool_t query_pool,
                                     uint32_t query_index, uint32_t query_count,
                                     size_t size, void *data, size_t stride) {
  (void)queue;
  auto host_query_pool = static_cast<host::query_pool_s *>(query_pool);

  if (host_query_pool->type == mux_query_type_duration) {
    auto results = static_cast<uint8_t *>(data);

    for (auto index = query_index; index < query_index + query_count; index++) {
      auto result = host_query_pool->getDurationQueryAt(index);
      std::memcpy(results, result, sizeof(mux_query_duration_result_s));
      results += stride;
    }
    return mux_success;
  }
  if (host_query_pool->type == mux_query_type_counter) {
#ifdef CA_HOST_ENABLE_PAPI_COUNTERS
    return host_query_pool->readPapiResults(
        static_cast<mux_query_counter_result_s *>(data), query_count,
        query_index);
#else
    return mux_error_feature_unsupported;
#endif
  }
  // We somehow got passed a query pool with an invalid type.
  return mux_error_invalid_value;
}
