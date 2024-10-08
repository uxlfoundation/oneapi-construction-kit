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

#include <mutex>

#include "hal_riscv_common.h"
#include "linker_script.h"
#include "refsi_hal.h"
#include "refsi_hal_g1.h"
#include "refsi_hal_m1.h"
#include "refsidrv/refsidrv.h"

namespace {

bool queryMemRange(refsi_device_t device, refsi_memory_map_kind kind,
                   refsi_memory_map_entry &range) {
  refsi_device_info_t device_info;
  if (refsi_success != refsiQueryDeviceInfo(device, &device_info)) {
    return false;
  }
  for (unsigned i = 0; i < device_info.num_memory_map_entries; i++) {
    refsi_memory_map_entry entry;
    if (refsi_success != refsiQueryDeviceMemoryMap(device, i, &entry)) {
      return false;
    } else if (entry.kind == kind) {
      range = entry;
      return true;
    }
  }
  return false;
}

class refsi_hal : public hal::hal_t {
 protected:
  hal::hal_info_t hal_info;
  riscv::hal_device_info_riscv_t hal_device_info;
  std::mutex lock;
  bool initialized = false;
#if defined(HAL_REFSI_TARGET_M1)
  refsi_device_family family = REFSI_M;
#elif defined(HAL_REFSI_TARGET_G1)
  refsi_device_family family = REFSI_G;
#else
#error The RefSi SoC family to target is undefined. Please set HAL_REFSI_SOC when building hal_refsi.
#endif
  std::vector<hal::hal_counter_description_t> counter_description_data;

 public:
  // return generic platform information
  const hal::hal_info_t &get_info() override {
    refsi_locker locker(lock);
    return hal_info;
  }

  // return generic target information
  const hal::hal_device_info_t *device_get_info(uint32_t index) override {
    refsi_locker locker(lock);
    return initialized ? &hal_device_info : nullptr;
  }

  // request the creation of a new hal
  hal::hal_device_t *device_create(uint32_t index) override {
    refsi_locker locker(lock);
    if (!initialized || (index > 0)) {
      return nullptr;
    }
    refsi_device_t device = refsiOpenDevice(family);
    if (!device) {
      return nullptr;
    }
    std::unique_ptr<refsi_hal_device> hal_device;
    switch (family) {
      default:
        refsiShutdownDevice(device);
        return nullptr;
      case REFSI_M:
        hal_device.reset(
            new refsi_m1_hal_device(device, &hal_device_info, lock));
        break;
      case REFSI_G:
        hal_device.reset(
            new refsi_g1_hal_device(device, &hal_device_info, lock));
        break;
    }
    if (!hal_device->initialize(locker)) {
      return nullptr;
    }
    return hal_device.release();
  }

  // destroy a device instance
  bool device_delete(hal::hal_device_t *device) override {
    // No locking - this is done by refsi_hal_device's destructor.
    delete static_cast<refsi_hal_device *>(device);
    return device != nullptr;
  }

  const char *get_target_name() const {
    switch (family) {
      default:
        return nullptr;
      case REFSI_M:
        return "RefSi M1";
      case REFSI_G:
        return hal_device_info.word_size == 32 ? "RefSi G1 RV32"
                                               : "RefSi G1 RV64";
    }
  }

  refsi_hal() {
    static constexpr uint32_t implemented_api_version = 6;
    static_assert(implemented_api_version == hal_t::api_version,
                  "Implemented API version for RefSi HAL does not match hal.h");
    hal_info.num_devices = 1;
    hal_info.api_version = implemented_api_version;
    if (refsi_success != refsiInitialize()) {
      hal_info.num_devices = 0;
      return;
    }
    refsi_device_t device = refsiOpenDevice(family);
    refsi_device_info_t device_info;
    if (!device ||
        refsi_success != refsiQueryDeviceInfo(device, &device_info)) {
      hal_info.num_devices = 0;
      return;
    }
    initialized = true;

    std::string isa_str(device_info.core_isa);
    if (isa_str.find("RV32") == 0) {
      hal_device_info.word_size = 32;
    } else if (isa_str.find("RV64") == 0) {
      hal_device_info.word_size = 64;
    } else {
      fprintf(stderr, "error: unsupported RISC-V ISA: %s\n",
              device_info.core_isa);
      abort();
    }

    unsigned num_harts = device_info.num_cores * device_info.num_harts_per_core;
    refsi_memory_map_entry dram;
    refsi_memory_map_entry perf_counters;
    if (!queryMemRange(device, DRAM, dram)) {
      hal_info.num_devices = 0;
      return;
    }
    if (queryMemRange(device, PERF_COUNTERS, perf_counters)) {
      populatePerfCounters(num_harts);
    }

    const uint64_t global_mem_max_over_allocation = 16 << 20;
    hal_device_info.type = hal::hal_device_type_riscv;
    hal_device_info.global_memory_avail =
        dram.size - global_mem_max_over_allocation;
    hal_device_info.shared_local_memory_size =
        (family == REFSI_G) ? 256 * 1024 : 64 * 1024;
    hal_device_info.should_link = true;
    hal_device_info.should_vectorize = false;
    hal_device_info.preferred_vector_width = 1;
    hal_device_info.supports_fp16 = false;
    hal_device_info.supports_doubles = false;
#if defined(HAL_REFSI_MODE_WG)
    hal_device_info.max_workgroup_size = 1024;
#elif defined(HAL_REFSI_MODE_WI)
    // Executing one work-item per hart limits the maximum work-group size to
    // the number of harts.
    hal_device_info.max_workgroup_size = REFSI_SIM_MAX_HARTS;
#else
#error Either HAL_REFSI_MODE_WG or HAL_REFSI_MODE_WI needs to be defined.
#endif
    hal_device_info.is_little_endian = true;
    hal_device_info.linker_script =
        std::string(hal_refsi_linker_script, hal_refsi_linker_script_size);

    // Parse the RISC-V ISA description string reported by the device.
    if (!riscv::update_info_from_riscv_isa_description(
            device_info.core_isa, hal_device_info, hal_device_info)) {
      fprintf(stderr, "error: unsupported RISC-V ISA: %s\n",
              device_info.core_isa);
      abort();
    }

    // Update various properties based on the info we've just parsed.
    hal_device_info.update_base_info_from_riscv(hal_device_info);

    if (hal_device_info.extensions & riscv::rv_extension_V) {
      hal_device_info.vlen = device_info.core_vlen;
    }

    if (hal_device_info.word_size == 32) {
      hal_device_info.abi = riscv::rv_abi_ILP32;
    } else if (hal_device_info.word_size == 64) {
      hal_device_info.abi = riscv::rv_abi_LP64;
    } else {
      fprintf(stderr, "error: unsupported RISC-V ISA: %s\n",
              device_info.core_isa);
      abort();
    }
    hal_info.platform_name = get_target_name();
    hal_device_info.target_name = get_target_name();
  }

  void populatePerfCounters(size_t num_total_harts) {
    // Default counter configuration for cycles, instrs, mem r/w. Display
    // totals at low log level and individual values at medium log level.
    hal::hal_counter_log_config_t cfg_default = {hal::hal_counter_verbose_mid,
                                                 hal::hal_counter_verbose_low};
    // Config for all other counters, don't display total and display individual
    // values at high log level.
    hal::hal_counter_log_config_t cfg_detailed = {
        hal::hal_counter_verbose_high, hal::hal_counter_verbose_none};

    // Number of values for each multi-value (per-hart) performance counter.
    uint32_t num_values = num_total_harts;

    counter_description_data = {
        {REFSI_PERF_CNTR_RETIRED_INSN, "retired_inst", "retired instructions",
         "hart", num_values, hal::hal_counter_unit_generic, cfg_default},
        {REFSI_PERF_CNTR_CYCLE, "cycles", "elapsed cycles", "hart", num_values,
         hal::hal_counter_unit_cycles, cfg_default}};

    // These extra profiling counters slow down the sim a lot when enabled, so
    // we disable them unless the user has specified the max profiling level.
    int profile_level = 0;
    if (const char *profile_env = std::getenv("CA_PROFILE_LEVEL")) {
      if (int profile_env_val = atoi(profile_env)) {
        profile_level = profile_env_val;
      }
    }
    if (profile_level > 2) {
      std::vector<hal::hal_counter_description_t> extra_counters = {
          {REFSI_PERF_CNTR_INT_INSN, "int_inst", "integer instructions", "hart",
           num_values, hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_FLOAT_INSN, "float_inst", "float instructions",
           "hart", num_values, hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_BRANCH_INSN, "branches_inst", "branch instructions",
           "hart", num_values, hal::hal_counter_unit_generic, cfg_detailed},

          {REFSI_PERF_CNTR_READ_INSN, "mem_read_inst", "read instructions",
           "hart", num_values, hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_READ_BYTE_INSN, "mem_read_bytes_inst",
           "read byte instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_READ_SHORT_INSN, "mem_read_short_inst",
           "read short instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_READ_WORD_INSN, "mem_read_word_inst",
           "read word instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_READ_DOUBLE_INSN, "mem_read_double_inst",
           "read double instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_READ_QUAD_INSN, "mem_read_quad_inst",
           "read quad instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},

          {REFSI_PERF_CNTR_WRITE_INSN, "mem_write_inst", "write instructions",
           "hart", num_values, hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_WRITE_BYTE_INSN, "mem_write_bytes_inst",
           "write byte instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_WRITE_SHORT_INSN, "mem_write_short_inst",
           "write short instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_WRITE_WORD_INSN, "mem_write_word_inst",
           "write word instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_WRITE_DOUBLE_INSN, "mem_write_double_inst",
           "write double instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed},
          {REFSI_PERF_CNTR_WRITE_QUAD_INSN, "mem_write_quad_inst",
           "write quad instructions", "hart", num_values,
           hal::hal_counter_unit_generic, cfg_detailed}};

      counter_description_data.insert(std::end(counter_description_data),
                                      std::begin(extra_counters),
                                      std::end(extra_counters));
    }

    uint32_t host_prefix = REFSI_NUM_PERF_COUNTERS;
    counter_description_data.push_back(
        {host_prefix + CTR_HOST_MEM_WRITE, "host_write",
         "direct memory write "
         "access",
         "", 1, hal::hal_counter_unit_bytes, cfg_default});
    counter_description_data.push_back(
        {host_prefix + CTR_HOST_MEM_READ, "host_read",
         "direct memory read "
         "access",
         "", 1, hal::hal_counter_unit_bytes, cfg_default});

    hal_device_info.counter_descriptions = counter_description_data.data();
    hal_device_info.num_counters = counter_description_data.size();
  }
};
}  // namespace

static refsi_hal hal_object;

hal::hal_t *get_hal(uint32_t &api_version) {
  api_version = hal_object.get_info().api_version;
  return &hal_object;
}
