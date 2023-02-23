// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief A utility class providing a helpful way for HAL implementations to
/// store and track counter values.
///
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
