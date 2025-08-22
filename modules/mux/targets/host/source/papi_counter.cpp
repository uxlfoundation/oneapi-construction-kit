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

#include <host/papi_counter.h>
#include <host/papi_error_codes.h>

namespace host {

/// @brief Helper function to do the event info query loop for a type of event.
///
/// The way we query events is lifted from PAPI's papi_avail utility. The PAPI
/// event codes are constructed like `PAPI_TYPE_MASK | n`, with n starting at 0
/// and incrementing up to however many events there are. So we can start
/// querying by just taking an event type mask (equivalent to `MASK | 0`), doing
/// an initial non-incrementing call to PAPI_enum_event with ENUM_FIRST to check
/// if there are any events of that type on the system, and then letting rip
/// with the loop.
///
/// @param event_code Event type mask to start querying events for.
/// @param event_enum_modifier PAPI defined modifier that changes how we
/// enumerate events.
/// @param counter_buffer Vector to return found event structs in.
template <long unsigned N>
inline mux_result_t
queryEvents(int event_code, int event_enum_modifier,
            cargo::small_vector<host_papi_counter, N> &counter_buffer) {
  // Check to see if there are any events before looping (if ENUM_FIRST doesn't
  // get us one there just aren't any to report).
  if (PAPI_enum_event(&event_code, PAPI_ENUM_FIRST) == PAPI_OK) {
    while (PAPI_enum_event(&event_code, event_enum_modifier) == PAPI_OK) {
      PAPI_event_info_t event_info{};
      auto papi_result = PAPI_get_event_info(event_code, &event_info);
      if (papi_result != PAPI_OK) {
        return getMuxResult(papi_result);
      }

      auto result_type_or_error = getMuxStorageType(event_info.data_type);
      if (!result_type_or_error.has_value()) {
        return result_type_or_error.value();
      }

      host_papi_counter counter = {event_code, event_info.count,
                                   event_info.symbol, event_info.short_descr,
                                   result_type_or_error.value()};

      if (counter_buffer.push_back(counter)) {
        return mux_error_out_of_memory;
      }
    }
  }
  return mux_success;
}

cargo::expected<mux_query_counter_storage_e, mux_result_t>
getMuxStorageType(int papi_data_type) {
  mux_query_counter_storage_e result_type;
  switch (papi_data_type) {
  case PAPI_DATATYPE_INT64:
    result_type = mux_query_counter_result_type_int64;
    break;
  case PAPI_DATATYPE_UINT64:
  case PAPI_DATATYPE_BIT64:
    result_type = mux_query_counter_result_type_uint64;
    break;
  case PAPI_DATATYPE_FP64:
    result_type = mux_query_counter_result_type_float64;
    break;
  default:
    return cargo::make_unexpected(mux_error_invalid_value);
  }
  return result_type;
}

cargo::expected<cargo::dynamic_array<host_papi_counter>, mux_result_t>
initPapiCounters() {
  if (PAPI_is_initialized() == PAPI_NOT_INITED) {
    auto papi_result = PAPI_library_init(PAPI_VER_CURRENT);
    if (papi_result != PAPI_VER_CURRENT) {
      return cargo::make_unexpected(getMuxResult(papi_result));
    }
  }

  // This is a global teardown that we only want to call when the application
  // exits.
  static std::once_flag registered_papi_shutdown;
  std::call_once(registered_papi_shutdown, []() { atexit(PAPI_shutdown); });

  // Initial buffer size based on modern (ish) Intel CPUs in developer machines
  // reporting 59 available counters, circa 2022.
  cargo::small_vector<host_papi_counter, 60> counter_buffer;

  // Check for preset counters, that's the only kind we support for now.
  auto result =
      queryEvents(PAPI_PRESET_MASK, PAPI_PRESET_ENUM_AVAIL, counter_buffer);

  if (result != mux_success) {
    return cargo::make_unexpected(result);
  }

  // Now copy the resultant event list out
  cargo::dynamic_array<host_papi_counter> out_array;
  if (out_array.alloc(counter_buffer.size())) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }
  std::copy_n(counter_buffer.begin(), counter_buffer.size(), out_array.begin());
  return {std::move(out_array)};
}
} // namespace host
