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

namespace {

class refsi_tutorial_hal : public hal::hal_t {
 protected:
  hal::hal_info_t hal_info;
  riscv::hal_device_info_riscv_t hal_device_info;
  std::recursive_mutex lock;
  bool initialized = false;

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
  hal::hal_device_t *device_create(uint32_t index) override { return nullptr; }

  // destroy a device instance
  bool device_delete(hal::hal_device_t *device) override { return false; }

  refsi_tutorial_hal() {
    const char *target_name = "RefSi M1 Tutorial";
    static constexpr uint32_t implemented_api_version = 6;
    static_assert(implemented_api_version == hal_t::api_version,
                  "Implemented API version for RefSi HAL does not match hal.h");
    hal_info.platform_name = target_name;
    hal_info.num_devices = 1;
    hal_info.api_version = implemented_api_version;
    initialized = true;

    const unsigned num_cores = 1;
    const unsigned num_harts_per_core = 4;
    const unsigned num_harts = num_cores * num_harts_per_core;
    const uint64_t global_mem_max_over_allocation = 16 * 1024 * 1024;  // 16 MiB
    const uint64_t dram_size = 128 * 1024 * 1024;  // 128 MiB
    const uint64_t tcdm_size = 4 * 1024 * 1024;    // 4 MiB
    const char *core_isa = "RV64GCV";
    const unsigned core_vlen = 512;
    hal_device_info.type = hal::hal_device_type_riscv;
    hal_device_info.word_size = 64;
    hal_device_info.target_name = target_name;
    hal_device_info.global_memory_avail =
        dram_size - global_mem_max_over_allocation;
    hal_device_info.shared_local_memory_size =
        ((tcdm_size * 3) / 4) / num_harts;
    hal_device_info.should_link = true;
    hal_device_info.should_vectorize = false;
    hal_device_info.preferred_vector_width = 1;
    hal_device_info.supports_fp16 = false;
    hal_device_info.supports_doubles = false;
    hal_device_info.max_workgroup_size = 1024;
    hal_device_info.is_little_endian = true;
    hal_device_info.linker_script =
        std::string(hal_refsi_tutorial_linker_script,
                    hal_refsi_tutorial_linker_script_size);

    // Parse the RISC-V ISA description string reported by the device.
    if (!riscv::update_info_from_riscv_isa_description(
            core_isa, hal_device_info, hal_device_info)) {
      fprintf(stderr, "error: unsupported RISC-V ISA: %s\n", core_isa);
      abort();
    }

    // Update various properties based on the info we've just parsed.
    hal_device_info.update_base_info_from_riscv(hal_device_info);
    hal_device_info.abi = riscv::rv_abi_LP64;

    if (hal_device_info.extensions & riscv::rv_extension_V) {
      hal_device_info.vlen = core_vlen;
    }
  }
};
}  // namespace

static refsi_tutorial_hal hal_object;

hal::hal_t *get_hal(uint32_t &api_version) {
  api_version = hal_object.get_info().api_version;
  return &hal_object;
}
