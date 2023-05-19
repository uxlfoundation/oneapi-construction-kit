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
/// @brief A utility class providing a helpful way for HAL implementations to
/// store and track counter values.

#ifndef HAL_COUNTERS_H_INCLUDED
#define HAL_COUNTERS_H_INCLUDED

#include <cstdint>
#include <vector>

namespace hal {
/// @addtogroup hal
/// @{
namespace util {
/// @addtogroup util
/// @{

struct hal_counter_value_t {
  hal_counter_value_t(uint32_t counter_id, uint32_t num_values) {
    data = std::vector<uint64_t>(num_values);
    has_values = std::vector<bool>(num_values);
  };

  bool has_value(uint32_t value_index) const {
    return has_values.at(value_index);
  }
  void clear_value(uint32_t value_index) { has_values.at(value_index) = false; }
  void set_value(uint32_t value_index, uint64_t value) {
    data[value_index] = value;
    has_values[value_index] = true;
  }
  uint64_t get_value(uint32_t value_index) const { return data[value_index]; }

 private:
  std::vector<bool> has_values;
  std::vector<uint64_t> data;
};

/// @}
}  // namespace util
/// @}
}  // namespace hal

#endif  // HAL_COUNTERS_H_INCLUDED
