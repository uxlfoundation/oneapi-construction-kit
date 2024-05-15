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
/// @brief Device Hardware Abstraction Layer types.

#ifndef HAL_TYPES_H_INCLUDED
#define HAL_TYPES_H_INCLUDED

#include <cstdint>
#include <string>

namespace hal {
/// @addtogroup hal
/// @{

/// @brief Intended to store a target side memory address.
typedef uint64_t hal_addr_t;
/// @brief Intended to store a target side size.
typedef uint64_t hal_size_t;
/// @brief Intended to store a target value.
typedef uint64_t hal_word_t;
/// @brief A unique handle identifying a loaded program.
typedef uint64_t hal_program_t;
/// @brief A unique handle identifying a kernel.
typedef uint64_t hal_kernel_t;

enum {
  hal_nullptr = 0,
  hal_invalid_program = 0,
  hal_invalid_kernel = 0,
};

enum hal_arg_kind_t {
  hal_arg_address,
  hal_arg_value,
  // ... add more ...
};

enum hal_addr_space_t {
  hal_space_local,
  hal_space_global,
};

enum hal_counter_verbosity_t {
  hal_counter_verbose_low,
  hal_counter_verbose_mid,
  hal_counter_verbose_high,

  hal_counter_verbose_none,
};

enum hal_counter_unit_t {
  hal_counter_unit_generic,
  hal_counter_unit_percentage,
  hal_counter_unit_nanoseconds,
  hal_counter_unit_bytes,
  hal_counter_unit_bytes_per_second,
  hal_counter_unit_kelvin,
  hal_counter_unit_watts,
  hal_counter_unit_volts,
  hal_counter_unit_amps,
  hal_counter_unit_hertz,
  hal_counter_unit_cycles
};

struct hal_counter_log_config_t {
  /// @brief A hint describing the minimum log/verbosity level at which to
  /// display the individual values of this counter. Set to 'none' to not
  /// display the counter in this way at any level.
  hal_counter_verbosity_t min_verbosity_per_value;
  /// @brief A hint describing the minimum log/verbosity level at which to
  /// display the overall total value of this counter. Set to 'none' to not
  /// display the counter in this way at any level.
  hal_counter_verbosity_t min_verbosity_total;
};

struct hal_counter_description_t {
  /// @brief A unique id for this counter.
  uint32_t counter_id;
  /// @brief Short-form name for this counter, e.g. "cycles"
  const char *name;
  /// @brief Descriptive name for this counter, e.g. "elapsed cycles"
  const char *description;
  /// @brief Used if contained_value > 1. Descriptive name for what each
  /// contained value represents, e.g. "core" for per-core contained values
  const char *sub_value_name;
  /// @brief The number of contained values within this counter. Must be at
  /// least 1.
  uint32_t contained_values;
  /// @brief The unit to display this counter's value with.
  hal_counter_unit_t unit;
  /// @brief Configuration for displaying this counter in profiling logs
  hal_counter_log_config_t log_cfg;
};

struct hal_arg_t {
  hal_arg_kind_t kind;
  hal_addr_space_t space;
  hal_size_t size;
  union {
    hal_addr_t address;
    void *pod_data;
  };
};

struct hal_ndrange_t {
  hal_size_t offset[3];
  hal_size_t global[3];
  hal_size_t local[3];
};

enum hal_device_type_t {
  hal_device_type_riscv,  // hal_device_riscv_t
};

struct hal_device_info_t {
  /// @brief The derived type of the HAL (one of hal_type_t).
  hal_device_type_t type;

  /// @brief Processor word size (32bits, 64bits, etc).
  uint32_t word_size;

  /// @brief Name of the target.
  const char *target_name;

  /// @brief Available global memory in bytes.
  uint64_t global_memory_avail;

  /// @brief Shared local memory size.
  uint64_t shared_local_memory_size;

  /// @brief set to true if CA should query the linker script and run lld.
  bool should_link;

  /// @brief A string containing the linker script to be used if should_link is
  /// true. This member should be left as an empty string if linking is not
  /// required.  Note, this is not a file path but rather the script contents
  /// itself.
  std::string linker_script;

  /// @brief true if should run the vectorizer.
  bool should_vectorize;

  /// @brief Number of bits of preferred vector width.
  uint32_t preferred_vector_width;

  /// @brief true if supports fp16.
  bool supports_fp16;

  /// @brief true if supports doubles.
  bool supports_doubles;

  /// @brief max workgroup size
  uint32_t max_workgroup_size;

  /// @brief true if little endian.
  bool is_little_endian;

  /// @brief number of supported performance counters.
  uint32_t num_counters = 0;

  /// @brief Array of counter descriptions. Can be null if num_counters == 0.
  hal_counter_description_t *counter_descriptions = nullptr;

  /// @brief Used to dictate whether to link as dynamic library.
  bool link_shared = false;
};

struct hal_info_t {
  /// @brief Name of the platform.
  const char *platform_name;
  /// @brief Return the number of devices supported on this platform.
  uint32_t num_devices;
  /// @brief Current version of the HAL API. The version number needs to be
  /// bumped any time the interface is changed.
  uint32_t api_version;
};

struct hal_sched_info_32_t {
  /// @brief Start group ID for each dimension
  uint32_t group_id_start[3];
  /// @brief Total number of groups in each dimension (needed for
  /// get_global_size())
  uint32_t num_groups_total[3];
  /// @brief Global offset for each dimension (needed for get_global_offset())
  uint32_t global_offset[3];
  /// @brief Number of work-items in each dimension
  uint32_t local_size[3];
  /// @brief Number of ND-range dimensions (1, 2 or 3)
  uint32_t num_dim;
  /// @brief number of groups from top level call to kernel if supported
  uint32_t num_groups_per_call[3];
  /// @brief device pointer to additional information
  uint32_t hal_extra;
};

struct hal_sched_info_64_t {
  /// @brief Start group ID for each dimension
  uint64_t group_id_start[3];
  /// @brief Total number of groups in each dimension (needed for
  /// get_global_size())
  uint64_t num_groups_total[3];
  /// @brief Global offset for each dimension (needed for get_global_offset())
  uint64_t global_offset[3];
  /// @brief Number of work-items in each dimension
  uint32_t local_size[3];
  /// @brief Number of ND-range dimensions (1, 2 or 3)
  uint32_t num_dim;
  /// @brief number of groups from top level call to kernel if supported
  uint64_t num_groups_per_call[3];
  /// @brief device pointer to additional information
  uint64_t hal_extra;
};

/// @}
}  // namespace hal

#endif  // HAL_TYPES_H_INCLUDED
