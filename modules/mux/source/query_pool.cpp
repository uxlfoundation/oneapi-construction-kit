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

#include "mux/select.h"
#include "mux/utils/id.h"
#include "tracer/tracer.h"

mux_result_t muxGetSupportedQueryCounters(
    mux_device_t device, mux_queue_type_e queue_type, uint32_t count,
    mux_query_counter_t *out_counters,
    mux_query_counter_description_t *out_descriptions, uint32_t *out_count) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(device)) {
    return mux_error_invalid_value;
  }

  if (!device->info->query_counter_support) {
    return mux_error_feature_unsupported;
  }

  if (static_cast<uint32_t>(queue_type) >=
      static_cast<uint32_t>(mux_queue_type_total)) {
    return mux_error_invalid_value;
  }

  if (count == 0 && (out_counters != nullptr || out_descriptions != nullptr)) {
    return mux_error_null_out_parameter;
  }

  if (count != 0 && (out_counters == nullptr && out_descriptions == nullptr)) {
    return mux_error_null_out_parameter;
  }

  if (out_counters == nullptr && out_descriptions == nullptr &&
      out_count == nullptr) {
    return mux_error_null_out_parameter;
  }

  return muxSelectGetSupportedQueryCounters(
      device, queue_type, count, out_counters, out_descriptions, out_count);
}

mux_result_t muxCreateQueryPool(
    mux_queue_t queue, mux_query_type_e query_type, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    mux_allocator_info_t allocator_info, mux_query_pool_t *out_query_pool) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(queue)) {
    return mux_error_invalid_value;
  }

  if (query_count == 0) {
    return mux_error_invalid_value;
  }

  if (query_type != mux_query_type_duration &&
      query_type != mux_query_type_counter) {
    return mux_error_invalid_value;
  }

  if (query_type == mux_query_type_duration &&
      query_counter_configs != nullptr) {
    return mux_error_invalid_value;
  }

  if (query_type == mux_query_type_counter) {
    if (!queue->device->info->query_counter_support) {
      return mux_error_feature_unsupported;
    }
    if (query_counter_configs == nullptr) {
      return mux_error_invalid_value;
    }
  }

  if (nullptr == out_query_pool) {
    return mux_error_null_out_parameter;
  }

  auto error = muxSelectCreateQueryPool(queue, query_type, query_count,
                                        query_counter_configs, allocator_info,
                                        out_query_pool);

  if (mux_success == error) {
    mux::setId<mux_object_id_query_pool>(queue->device->id, *out_query_pool);
  }

  return error;
}

void muxDestroyQueryPool(mux_queue_t queue, mux_query_pool_t query_pool,
                         mux_allocator_info_t allocator_info) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(queue)) {
    return;
  }

  if (mux::objectIsInvalid(query_pool)) {
    return;
  }

  muxSelectDestroyQueryPool(queue, query_pool, allocator_info);
}

mux_result_t muxGetQueryCounterRequiredPasses(
    mux_queue_t queue, uint32_t query_count,
    const mux_query_counter_config_t *query_counter_configs,
    uint32_t *out_pass_count) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(queue)) {
    return mux_error_invalid_value;
  }

  if (!queue->device->info->query_counter_support) {
    return mux_error_feature_unsupported;
  }

  if (query_count == 0 || query_counter_configs == nullptr) {
    return mux_error_invalid_value;
  }

  if (out_pass_count == nullptr) {
    return mux_error_null_out_parameter;
  }

  return muxSelectGetQueryCounterRequiredPasses(
      queue, query_count, query_counter_configs, out_pass_count);
}

mux_result_t muxGetQueryPoolResults(mux_queue_t queue,
                                    mux_query_pool_t query_pool,
                                    uint32_t query_index, uint32_t query_count,
                                    size_t size, void *data, size_t stride) {
  const tracer::TraceGuard<tracer::Mux> trace(__func__);

  if (mux::objectIsInvalid(queue)) {
    return mux_error_invalid_value;
  }

  if (mux::objectIsInvalid(query_pool)) {
    return mux_error_invalid_value;
  }

  if (query_index >= query_pool->count ||
      (query_count + query_index) > query_pool->count) {
    return mux_error_invalid_value;
  }

  if (query_pool->type == mux_query_type_duration) {
    if (size < sizeof(mux_query_duration_result_s) * query_count) {
      return mux_error_invalid_value;
    }

    if (stride < sizeof(mux_query_duration_result_s)) {
      return mux_error_invalid_value;
    }
  } else if (query_pool->type == mux_query_type_counter) {
    if (size < sizeof(mux_query_counter_result_s) * query_count) {
      return mux_error_invalid_value;
    }

    if (stride < sizeof(mux_query_counter_result_s)) {
      return mux_error_invalid_value;
    }
  }

  if (nullptr == data) {
    return mux_error_invalid_value;
  }

  return muxSelectGetQueryPoolResults(queue, query_pool, query_index,
                                      query_count, size, data, stride);
}
