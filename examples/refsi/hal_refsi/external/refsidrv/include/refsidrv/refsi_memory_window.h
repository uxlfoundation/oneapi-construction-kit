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

#ifndef _REFSIDRV_REFSI_MEMORY_WINDOW_H
#define _REFSIDRV_REFSI_MEMORY_WINDOW_H

#include "common_devices.h"
#include "refsidrv.h"

struct RefSiMemoryController;

/// @brief Holds the state needed to configure a memory window.
struct RefSiMemoryWindowConfig {
  /// @brief Whether or not the window is active, i.e. whether it has been
  /// added to the platform's memory map or not.
  bool active = false;
  /// @brief Mapping function used to redirect memory accesses. Can be 'shared'
  /// or 'per-hart'.
  uint32_t mode = CMP_WINDOW_MODE_SHARED;
  /// @brief Base address the window should be mapped at.
  refsi_addr_t base_address = 0;
  /// @brief Target address where memory accesses are redirected to.
  refsi_addr_t target_address = 0;
  /// @brief Size of the memory window, in bytes.
  uint64_t size = 0;
  /// @brief First (A) part of the scaling factor. The scaling factor is
  /// defined as the product of A and B.
  uint64_t scale_a = 0;
  /// @brief Second (B) part of the scaling factor. The scaling factor is
  /// defined as the product of A and B.
  uint64_t scale_b = 0;

  /// @brief Calculate the scaling factor for the window. It is only valid for
  /// windows in per-hart mode, where the mapping function is:
  ///   BASE + offset -> TARGET + (SCALE * hart_id) + offset
  uint64_t getScale() const;

  /// @brief Try to scale the factor for the window to the given value.
  refsi_result setScale(uint64_t newScale);
};

/// @brief Memory device that can be used to redirect memory accesses to another
/// memory device. This enables the creation of memory 'windows' into the
/// system's memory map which point to another area of memory.
class RefSiMemoryWindow : public MemoryDeviceBase {
public:
  RefSiMemoryWindow(RefSiMemoryController &mem_ctl) : mem_ctl(mem_ctl) {}

  bool isMapped() const { return mapped_device != nullptr; }
  refsi_result handleRegWrite(refsi_cmp_register_id canonical_reg,
                              uint64_t value, MemoryController &mem_if);
  refsi_result enableWindow(MemoryController &mem_if);
  refsi_result disableWindow(MemoryController &mem_if);

  RefSiMemoryWindowConfig & getConfig() { return config; }

  size_t mem_size() const override;

  bool load(reg_t addr, size_t len, uint8_t* bytes, unit_id_t unit) override;
  bool store(reg_t addr, size_t len, const uint8_t* bytes,
             unit_id_t unit) override;
  uint8_t *addr_to_mem(reg_t addr, size_t size, unit_id_t unit) override;

  static bool splitCmpRegister(refsi_cmp_register_id reg_idx,
                              refsi_cmp_register_id &canon_reg_idx,
                              uint32_t &window_id);

private:
  refsi_result getEffectiveAddress(reg_t addr, unit_id_t unit,
                                   refsi_addr_t &eff_address);

  RefSiMemoryController &mem_ctl;
  /// Window configuration, updated when window configuration registers are
  /// written to.
  RefSiMemoryWindowConfig config;
  /// Target device.
  MemoryDevice *mapped_device = nullptr;
  /// Offset between the target device's starting address and the memory
  /// window's target address.
  reg_t mapped_offset = 0;
  /// Snapshot of the window configuration when the window was mapped.
  RefSiMemoryWindowConfig mapped_config;
};

#endif  // _REFSIDRV_REFSI_MEMORY_WINDOW_H
