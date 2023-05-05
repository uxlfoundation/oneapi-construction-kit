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
/// Host's PAPI event abstraction.

#ifndef HOST_PAPI_COUNTERS_H_INCLUDED
#define HOST_PAPI_COUNTERS_H_INCLUDED

#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <mux/mux.h>
#include <papi.h>

#include <mutex>
#include <string>

namespace host {

/// @brief Struct containing all the information host needs to know about a
/// PAPI event.
///
/// Also has helper functions to dole this information out into the various Mux
/// structs.
struct host_papi_counter final {
  // Defaulted constructors needed for the assign operator.
  host_papi_counter() = default;
  host_papi_counter(const host_papi_counter &) = default;

  /// @brief Assign operator for copying list of counters back to the host
  /// device.
  ///
  /// This needs to be explicitely defined as we have a const member variable
  /// (`unit`).
  host_papi_counter &operator=(const host_papi_counter &other) {
    if (this == &other) {
      return *this;
    }
    this->papi_event_code = other.papi_event_code;
    this->name = other.name;
    this->description = other.description;
    this->storage = other.storage;
    this->hardware_counters = other.hardware_counters;
    return *this;
  }

  /// @brief Unique ID papi uses to refer to the event.
  int papi_event_code;
  /// @brief Number of hardware counters taken up by this counter.
  uint32_t hardware_counters;
  /// @brief Event name, in PAPI shorthand so not particularly descriptive.
  std::string name;
  /// @brief Short description queried from PAPI, 64 characters or less.
  std::string description;
  /// @brief Data storage type of the counter.
  mux_query_counter_storage_e storage;
  /// @brief Category string we return in mux counter description structs.
  ///
  /// For now all papi counters are just "PAPI counter" but one day we might
  /// want to report user defined events and distinguish them with a category
  /// of their own.
  const char *category = "PAPI counter";
  /// @brief Unit of measurement the counter is counting.
  ///
  /// This is always generic for PAPI counters, although PAPI events do have a
  /// query-able unit string associated with them, it isn't set anywhere in
  /// PAPI's source for the builtin events. One day we might define our own
  /// events and populate this, and there are ways we can deduce it if we
  /// really want to, so it remains as a member of the struct.
  const mux_query_counter_unit_e unit = mux_query_counter_unit_generic;

  /// @brief Helper function to populate a `mux_query_counter_s` with this
  /// counter's info.
  ///
  /// @param out_query_counter Counter struct to populate.
  void populateMuxQueryCounter(mux_query_counter_s *out_query_counter) const {
    out_query_counter->unit = unit;
    out_query_counter->storage = storage;
    out_query_counter->uuid = static_cast<uint32_t>(papi_event_code);
    out_query_counter->hardware_counters = hardware_counters;
  }

  /// @brief Helper function to populate a `mux_query_counter_descriptions_s`
  /// with this counter's info.
  ///
  /// @param out_description Counter description struct to populate.
  void populateMuxQueryCounterDescription(
      mux_query_counter_description_s *out_description) const {
    std::strncpy(out_description->name, name.c_str(), 256);
    std::strncpy(out_description->category, category, 256);
    std::strncpy(out_description->description, description.c_str(), 256);
  }
};

/// @brief Helper function for getting a Mux storage type from a PAPI storage
/// type.
///
/// @param papi_data_type Data type retrieved from a `PAPI_event_info_t`.
cargo::expected<mux_query_counter_storage_e, mux_result_t> getMuxStorageType(
    int papi_data_type);

/// @brief Helper function that queries PAPI for all available counters and
/// returns a dynamic array of `host_papi_counter` structs.
cargo::expected<cargo::dynamic_array<host_papi_counter>, mux_result_t>
initPapiCounters();
}  // namespace host

#endif
