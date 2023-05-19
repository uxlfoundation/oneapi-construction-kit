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

#include <hal_profiler.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace hal {
/// @addtogroup hal
/// @{

namespace util {
/// @addtogroup util
/// @{

void hal_profiler_t::write_title() {
  if (main_headings.size() == 0 || log_level == hal_counter_verbose_none) {
    return;
  }
  get_out_stream() << "kernel_name,";
  for (auto &name : additional_headings) {
    get_out_stream() << name << ",";
  }
  for (auto &name : main_headings) {
    get_out_stream() << name << ",";
  }
  get_out_stream() << "\n";
}

std::ostream &hal_profiler_t::get_out_stream() {
  if (!output_file_ofs.is_open()) {
    if (std::strcmp(output_file_path.c_str(), "-") == 0) {
      return std::cout;
    }
    // Open the output file and write the CSV headings
    output_file_ofs.open(output_file_path,
                         std::ofstream::out | std::ofstream::trunc);
    if (!output_file_ofs.fail()) {
      write_title();
    }
  }

  return output_file_ofs;
}

void hal_profiler_t::set_output_path(const std::string &path) {
  output_file_path = path;
  if (output_file_path == "-" &&
      this->log_level != hal::hal_counter_verbose_none) {
    write_title();
  }
}

void hal_profiler_t::update_counters(hal::hal_device_t &device,
                                     std::string name) {
  bool log_enable = log_level != hal::hal_counter_verbose_none;
  size_t total_acc_index = 0;

  for (unsigned i = 0; i < num_counters; i++) {
    auto id = descs[i].counter_id;
    bool log_per_val =
        log_enable && descs[i].log_cfg.min_verbosity_per_value <= log_level;
    bool log_total =
        log_enable && descs[i].log_cfg.min_verbosity_total <= log_level;
    bool multiple_values = descs[i].contained_values > 1;

    uint64_t value;
    for (unsigned j = 0; j < descs[i].contained_values; j++) {
      if (device.counter_read(id, value, j)) {
        if (log_per_val) {
          // If there are multiple values in this counter, put the values in
          // the specific row for that sub-value
          if (multiple_values) {
            map_subval_to_rows[descs[i].sub_value_name][j]
                .values[descs[i].name] = value;
          } else {
            map_subval_to_rows[""][j].values[descs[i].name] = value;
          }
        }

        if (log_total) {
          total_acc[total_acc_index] += value;
        }

        for (auto &user_acc : user_accs) {
          if (user_acc.second.enabled) {
            user_acc.second.accs[i] += value;
          }
        }
      }
    }

    if (log_total) {
      total_acc_index++;
    }
  }

  unsigned subval_count = 0;
  auto subval_total = map_subval_to_rows.size();
  for (auto &subval_entry : map_subval_to_rows) {
    auto &sub_val_type = subval_entry.first;
    auto &sub_val_rows = subval_entry.second;
    for (auto &row : sub_val_rows) {
      if (row.values.size() == 0) {
        continue;
      }
      get_out_stream() << name << ",";
      // Print sub-value values for this row
      for (unsigned i = 1; i < subval_total; i++) {
        if (i == subval_count) get_out_stream() << row.sub_value_id;
        get_out_stream() << ",";
      }

      // Print every counter for this row
      for (unsigned i = 0; i < num_counters; i++) {
        if (descs[i].log_cfg.min_verbosity_per_value > log_level) {
          continue;
        }
        auto &name = descs[i].name;
        if (row.values.count(name)) {
          get_out_stream() << row.values[name];
        }
        get_out_stream() << ",";
      }
      get_out_stream() << "\n";
    }
    subval_count++;
  }

  // Clear all the values
  for (auto &row : map_subval_to_rows) {
    auto &val_rows = row.second;
    for (auto &val_row : val_rows) {
      val_row.values.clear();
    }
  }
}

void hal_profiler_t::setup_counters(hal::hal_device_t &device) {
  descs = device.get_info()->counter_descriptions;
  num_counters = device.get_info()->num_counters;
  int log_level = 0;
  if (const char *log_env = std::getenv("CA_PROFILE_LEVEL")) {
    if (int log_env_val = atoi(log_env)) {
      log_level = log_env_val;
    }
  }
  switch (log_level) {
    case 3:
      this->log_level = hal::hal_counter_verbose_high;
      break;
    case 2:
      this->log_level = hal::hal_counter_verbose_mid;
      break;
    case 1:
      this->log_level = hal::hal_counter_verbose_low;
      break;
    case 0:
    default:
      this->log_level = hal::hal_counter_verbose_none;
      break;
  }

  if (this->log_level != hal::hal_counter_verbose_none) {
    device.counter_set_enabled(true);
  }

  // Sub-values essentially get transposed to being a single additional column,
  // meaning that `update_counters` can create N different rows when there are
  // N different sub-values across all counters. For readability we put them all
  // at the left-most columns after the kernel name.

  // Create a default vector of rows for values that don't belong to a specific
  // sub-value (e.g. counters with no 'hart_id' etc)
  map_subval_to_rows.clear();
  std::vector<log_row_t> default_rows{};
  default_rows.push_back({"", 0, {}});
  map_subval_to_rows[""] = default_rows;

  for (unsigned i = 0; i < num_counters; i++) {
    // If this counter can contain more than one value, it maps to a sub-value (
    // e.g 'hart_id')
    if (descs[i].contained_values > 1) {
      // If this is a new sub-value, created a new vector of rows as these
      // counters will go in separate rows. Also save the heading so we have a
      // column for the actual sub-value values (e.g. the actual hart_id value)
      if (!map_subval_to_rows.count(descs[i].sub_value_name)) {
        std::vector<log_row_t> sv_rows{};
        for (unsigned j = 0; j < descs[i].contained_values; j++) {
          sv_rows.push_back({descs[i].sub_value_name, j, {}});
        }
        map_subval_to_rows[descs[i].sub_value_name] = sv_rows;
        additional_headings.push_back(descs[i].sub_value_name);
      }
    }

    if (descs[i].log_cfg.min_verbosity_per_value <= this->log_level) {
      main_headings.push_back(descs[i].name);
    }

    if (descs[i].log_cfg.min_verbosity_total <= this->log_level) {
      total_acc.push_back(0);
    }
  }
}

void hal_profiler_t::write_summary() {
  size_t total_acc_index = 0;
  if (log_level == hal::hal_counter_verbose_none) {
    return;
  }
  for (unsigned i = 0; i < num_counters; i++) {
    auto &desc = descs[i];
    if (desc.log_cfg.min_verbosity_total <= log_level) {
      std::cout << "[+] total " << desc.description << ": "
                << format_value(total_acc[total_acc_index++], desc) << "\n";
    }
  }
}

std::string hal_profiler_t::format_value_bytes(uint64_t val, bool per_sec) {
  static const std::array<const char *, 4> units = {"B", "KB", "MB", "GB"};

  int read_index = 0;
  for (auto &unit : units) {
    (void)unit;
    if (val < 1024) {
      break;
    }
    val /= 1024;
    read_index += 1;
  }

  const char *postfix = per_sec ? "/s" : "";
  return std::to_string(val) + units[read_index] + postfix;
}

std::string hal_profiler_t::format_value(uint64_t val,
                                         hal::hal_counter_description_t &desc) {
  switch (desc.unit) {
    case hal::hal_counter_unit_bytes:
      return format_value_bytes(val, false);
    case hal::hal_counter_unit_bytes_per_second:
      return format_value_bytes(val, true);
    case hal::hal_counter_unit_nanoseconds:
      return std::to_string(val) + "ns";
    case hal::hal_counter_unit_hertz:
      return std::to_string(val) + "hz";
    case hal::hal_counter_unit_percentage:
      return std::to_string(val) + "%";
    default:
      return std::to_string(val);
  }
}

uint32_t hal_profiler_t::start_accumulating() {
  auto acc_id = user_acc_index++;

  accumulators_t acc;
  acc.enabled = true;
  user_accs[acc_id] = acc;

  for (uint32_t i = 0; i < num_counters; i++) {
    user_accs[acc_id].accs.push_back(0);
  }
  return acc_id;
}

void hal_profiler_t::stop_accumulating(uint32_t acc_id) {
  user_accs[acc_id].enabled = false;
}

uint64_t hal_profiler_t::read_acc_value(uint32_t acc_id, uint32_t counter_id) {
  return user_accs[acc_id].accs.at(counter_id);
}

void hal_profiler_t::clear_accumulator(uint32_t acc_id) {
  user_accs.erase(acc_id);
}

/// @}
}  // namespace util
/// @}
}  // namespace hal
