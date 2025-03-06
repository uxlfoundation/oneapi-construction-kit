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

#include <CL/cl_ext_codeplay.h>
#include <cargo/small_vector.h>
#include <cl/command_queue.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/macros.h>
#include <cl/mux.h>
#include <extension/codeplay_performance_counters.h>

extension::codeplay_performance_counters::codeplay_performance_counters()
    : extension("cl_codeplay_performance_counters",
#ifdef OCL_EXTENSION_cl_codeplay_performance_counters
                usage_category::DEVICE
#ifndef OCL_EXTENSION_cl_khr_create_command_queue
#error cl_codeplay_performance_counters requires cl_khr_create_command_queue
#endif
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 1, 0)) {
}

cl_int extension::codeplay_performance_counters::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  // Don't participate in info queries when the device does not support the
  // extension, this includes being inlcuded in `CL_DEVICE_EXTENSIONS`.
  if (!device->mux_device->info->query_counter_support) {
    return CL_INVALID_VALUE;
  }

  if (param_name != CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY) {
    return extension::GetDeviceInfo(device, param_name, param_value_size,
                                    param_value, param_value_size_ret);
  }

  OCL_CHECK(param_value_size == 0 && param_value != nullptr,
            return CL_INVALID_VALUE);
  OCL_CHECK(param_value_size != 0 && param_value == nullptr,
            return CL_INVALID_VALUE);

  uint32_t count;
  if (muxGetSupportedQueryCounters(device->mux_device, mux_queue_type_compute,
                                   0, nullptr, nullptr, &count)) {
    return CL_INVALID_VALUE;
  }

  const size_t value_size = sizeof(cl_performance_counter_codeplay) * count;
  OCL_SET_IF_NOT_NULL(param_value_size_ret, value_size);

  if (param_value) {
    OCL_CHECK(param_value_size < value_size, return CL_INVALID_VALUE);

    cargo::small_vector<mux_query_counter_t, 256> mux_counters;
    if (mux_counters.resize(count)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    cargo::small_vector<mux_query_counter_description_t, 256> mux_descs;
    if (mux_descs.resize(count)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    if (muxGetSupportedQueryCounters(device->mux_device, mux_queue_type_compute,
                                     count, mux_counters.data(),
                                     mux_descs.data(), nullptr)) {
      return CL_INVALID_VALUE;
    }

    auto counters = static_cast<cl_performance_counter_codeplay *>(param_value);
    for (uint32_t index = 0; index < count; index++) {
      counters[index].unit = mux_counters[index].unit;
      counters[index].storage = mux_counters[index].storage;
      counters[index].uuid = mux_counters[index].uuid;
      std::strncpy(counters[index].name, mux_descs[index].name, 256);
      std::strncpy(counters[index].category, mux_descs[index].category, 256);
      std::strncpy(counters[index].description, mux_descs[index].description,
                   256);
    }
  }

  return CL_SUCCESS;
}

cargo::optional<cl_int>
extension::codeplay_performance_counters::ApplyPropertyToCommandQueue(
    cl_command_queue command_queue, cl_queue_properties_khr property,
    cl_queue_properties_khr value) const {
  if (property != CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY) {
    return cargo::nullopt;
  }

  OCL_CHECK(!command_queue->device->mux_device->info->query_counter_support,
            return CL_INVALID_QUEUE_PROPERTIES);

  auto config =
      reinterpret_cast<cl_performance_counter_config_codeplay *>(value);
  if (config == nullptr || config->descs == nullptr) {
    return CL_INVALID_VALUE;
  }

  if (auto mux_error = muxCreateQueryPool(
          command_queue->mux_queue, mux_query_type_counter, config->count,
          reinterpret_cast<mux_query_counter_config_t *>(config->descs),
          command_queue->device->mux_allocator,
          &command_queue->counter_queries)) {
    cl::getErrorFrom(mux_error);
  }

  return CL_SUCCESS;
}

cl_int extension::codeplay_performance_counters::GetEventProfilingInfo(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
  if (param_name != CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY) {
    return CL_INVALID_VALUE;
  }

  OCL_CHECK(param_value_size == 0 && param_value != nullptr,
            return CL_INVALID_VALUE);
  OCL_CHECK(param_value_size != 0 && param_value == nullptr,
            return CL_INVALID_VALUE);

  mux_query_pool_t counter_queries = event->queue->counter_queries;
  OCL_CHECK(counter_queries == nullptr, return CL_INVALID_VALUE);

  const size_t value_size = sizeof(cl_performance_counter_result_codeplay) *
                            event->queue->counter_queries->count;
  OCL_SET_IF_NOT_NULL(param_value_size_ret, value_size);

  if (param_value) {
    OCL_CHECK(param_value_size < value_size, return CL_INVALID_VALUE);

    if (auto mux_error = muxGetQueryPoolResults(
            event->queue->mux_queue, counter_queries, 0, counter_queries->count,
            sizeof(mux_query_counter_result_t) * counter_queries->count,
            param_value, sizeof(mux_query_counter_result_t))) {
      return cl::getErrorFrom(mux_error);
    }
  }

  return CL_SUCCESS;
}
