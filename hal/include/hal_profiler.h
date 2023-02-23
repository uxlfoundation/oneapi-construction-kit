// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HAL_PROFILER_H_INCLUDED
#define HAL_PROFILER_H_INCLUDED

#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>

#include "hal.h"
#include "hal_types.h"

namespace hal {
/// @addtogroup hal
/// @{
namespace util {
/// @addtogroup util
/// @{

struct hal_profiler_t {
  ~hal_profiler_t() {
    if (output_file_ofs.is_open()) {
      output_file_ofs.close();
    }
  }
  /// @brief Write the headings line to the csv file
  void write_title();
  /// @brief Initialize the profiler with the given counters
  void setup_counters(hal_device_t &device);
  /// @brief Check for new counter values, updating a
  /// @param device The HAL device to check for counter values
  /// @param name The name of the event to associate with the log entries for
  /// this update. Usually the kernel name for kernel execs, otherwise blank
  void update_counters(hal_device_t &device, std::string name = {});
  /// @brief Write the summary to stdout
  void write_summary();
  /// @brief Set the output file path to use for the log
  /// Will default to /tmp/hal_profile.csv
  void set_output_path(const std::string &path);

  /// @brief Start accumulating all counter values until stopped
  /// @return A unique ID representing the started set of accumulated totals
  uint32_t start_accumulating();
  /// @brief Stop accumulating counter values
  void stop_accumulating(uint32_t acc_id);
  /// @brief Read the total accumulated value for the given counter
  uint64_t read_acc_value(uint32_t acc_id, uint32_t counter_id);
  void clear_accumulator(uint32_t acc_id);

 private:
  struct log_row_t {
    std::string sub_value_name;
    size_t sub_value_id;  // e.g. 3 for hart_id 3
    std::unordered_map<std::string, uint64_t> values;
  };
  struct accumulators_t {
    std::vector<uint64_t> accs;
    bool enabled = false;
  };
  std::ostream &get_out_stream();
  std::string format_value(uint64_t val, hal_counter_description_t &desc);
  std::string format_value_bytes(uint64_t val, bool per_sec);
  hal_counter_description_t *descs;
  uint32_t num_counters = 0;
  hal_counter_verbosity_t log_level = hal_counter_verbose_none;
  std::ofstream output_file_ofs;
  std::string output_file_path = "/tmp/hal_profile.csv";

  std::vector<std::string> main_headings;
  std::vector<std::string> additional_headings;

  // Map each sub-value category (e.g. hart_id) to the rows associated with it
  std::map<std::string, std::vector<log_row_t>> map_subval_to_rows;

  // Accumulated values for 'total' values in summary
  std::vector<uint64_t> total_acc;

  // Accumulated values for user-triggered requests
  std::unordered_map<uint32_t, accumulators_t> user_accs;
  uint32_t user_acc_index = 0;
};

/// @}
}  // namespace util
/// @}
}  // namespace hal
#endif
