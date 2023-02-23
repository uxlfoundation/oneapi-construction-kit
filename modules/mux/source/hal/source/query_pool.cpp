// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/hal/query_pool.h"

#include "mux/utils/small_vector.h"

namespace mux {
namespace hal {

mux_result_t query_pool::getSupportedQueryCounters(
    mux::hal::device *device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count) {
  auto max_num_counters = device->hal_device->get_info()->num_counters;
  if (queue_type != mux_queue_type_compute) {
    return mux_error_invalid_value;
  }
  if (count == 0) {
    *out_count = max_num_counters;
    return mux_success;
  }
  if (count > max_num_counters) {
    return mux_error_invalid_value;
  }
  for (uint32_t i = 0; i < std::min(count, max_num_counters); i++) {
    auto &hal_counter = device->hal_device->get_info()->counter_descriptions[i];
    if (out_counters != nullptr) {
      out_counters[i] = {mux_query_counter_unit_generic,
                         mux_query_counter_result_type_uint64,
                         hal_counter.counter_id, 1};
    }
    if (out_descriptions != nullptr) {
      std::strncpy(out_descriptions[i].name, hal_counter.name, 256);
      std::strncpy(out_descriptions[i].category, device->info->device_name,
                   256);
      std::strncpy(out_descriptions[i].description, hal_counter.description,
                   256);
    }
  }
  return mux_success;
}

cargo::expected<uint32_t, mux_result_t>
query_pool::getQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs) {
  (void)queue;
  (void)query_count;
  (void)query_counter_configs;
  return 1;
}

mux_result_t query_pool::getQueryPoolResults(mux_queue_t queue,
                                             uint32_t query_index,
                                             uint32_t query_count, size_t size,
                                             void *data, size_t stride) {
  (void)size;
  auto results = static_cast<uint8_t *>(data);
  auto device = static_cast<mux::hal::device *>(queue->device);
  auto max_num_counters = device->hal_device->get_info()->num_counters;
  if (type == mux_query_type_duration) {
    for (auto index = query_index; index < query_index + query_count; index++) {
      auto result = getDurationQueryAt(index);
      if (!result) {
        return mux_error_invalid_value;  // Out-of-range index.
      }
      std::memcpy(results, result, sizeof(mux_query_duration_result_s));
      results += stride;
    }
  } else if (type == mux_query_type_counter) {
    if (query_index + query_count > max_num_counters) {
      return mux_error_invalid_value;  // Out-of-range index.
    }
    for (auto index = query_index; index < query_index + query_count; index++) {
      // We don't use the query pool for storage; we can just read the values
      // straight from the profiler
      mux_query_counter_result_s result;
      result.uint64 =
          device->profiler.read_acc_value(counter_accumulator_id, index);
      std::memcpy(results, &result, sizeof(mux_query_counter_result_s));
      results += stride;
    }
  }
  return mux_success;
}
}  // namespace hal
}  // namespace mux
