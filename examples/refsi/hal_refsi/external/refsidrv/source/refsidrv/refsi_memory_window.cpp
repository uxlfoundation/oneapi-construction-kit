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

#include "refsidrv/refsi_memory_window.h"

bool RefSiMemoryWindow::splitCmpRegister(refsi_cmp_register_id reg_idx,
                                         refsi_cmp_register_id &canonical_reg,
                                         uint32_t &window_id) {
  if ((reg_idx >= CMP_REG_WINDOW_BASE0) && (reg_idx <= CMP_REG_WINDOW_BASEn)) {
    canonical_reg = CMP_REG_WINDOW_BASE0;
  } else if ((reg_idx >= CMP_REG_WINDOW_TARGET0) &&
             (reg_idx <= CMP_REG_WINDOW_TARGETn)) {
    canonical_reg = CMP_REG_WINDOW_TARGET0;
  } else if ((reg_idx >= CMP_REG_WINDOW_MODE0) &&
             (reg_idx <= CMP_REG_WINDOW_MODEn)) {
    canonical_reg = CMP_REG_WINDOW_MODE0;
  } else if ((reg_idx >= CMP_REG_WINDOW_SCALE0) &&
             (reg_idx <= CMP_REG_WINDOW_SCALEn)) {
    canonical_reg = CMP_REG_WINDOW_SCALE0;
  } else {
    canonical_reg = reg_idx;
    window_id = 0;
    return false;
  }
  window_id = reg_idx - canonical_reg;
  return true;
}

refsi_result RefSiMemoryWindow::handleRegWrite(
    refsi_cmp_register_id canonical_reg, uint64_t value,
    MemoryController &mem_if) {
  bool mapping_changed = false;
  switch (canonical_reg) {
    default:
      return refsi_failure;
    case CMP_REG_WINDOW_BASE0:
      config.base_address = value;
      mapping_changed |= (config.base_address != mapped_config.base_address);
      break;
    case CMP_REG_WINDOW_TARGET0:
      config.target_address = value;
      mapping_changed |=
          (config.target_address != mapped_config.target_address);
      break;
    case CMP_REG_WINDOW_SCALE0:
      config.scale_a = CMP_GET_WINDOW_SCALE_A(value);
      config.scale_b = CMP_GET_WINDOW_SCALE_B(value);
      mapping_changed |= (config.getScale() != mapped_config.getScale());
      break;
    case CMP_REG_WINDOW_MODE0:
      config.active = CMP_GET_WINDOW_ACTIVE(value) != 0;
      config.mode = CMP_GET_WINDOW_MODE(value);
      mapping_changed |= (config.mode != mapped_config.mode);
      config.size = CMP_GET_WINDOW_SIZE(value);
      mapping_changed |= (config.size != mapped_config.size);
      break;
  }

  // Disable the window when the ACTIVE bit is cleared or when the mapping
  // settings have changed.
  if (!config.active || mapping_changed) {
    disableWindow(mem_if);
  }

  // Enable the window when the ACTIVE bit is set.
  if (config.active && !isMapped()) {
    return enableWindow(mem_if);
  } else {
    return refsi_success;
  }
}

refsi_result RefSiMemoryWindow::enableWindow(MemoryController &mem_if) {
  reg_t dev_offset = 0;
  MemoryDevice *device = mem_if.find_device(config.target_address, dev_offset);
  if (!device) {
    return refsi_failure;
  } else if (dynamic_cast<RefSiMemoryWindow *>(device) != nullptr) {
    // Do not allow mapping to another window, which would allow cycles in the
    // mapping graph.
    return refsi_failure;
  } else if (config.mode != CMP_WINDOW_MODE_SHARED &&
             config.mode != CMP_WINDOW_MODE_PERT_HART) {
    return refsi_failure;
  }
  config.active = true;
  mapped_device = device;
  mapped_offset = dev_offset;
  mapped_config = config;
  mem_if.add_device(config.base_address, this);
  return refsi_success;
}

refsi_result RefSiMemoryWindow::disableWindow(MemoryController &mem_if) {
  if (!isMapped()) {
    return refsi_failure;
  }
  MemoryDevice *old_device = mem_if.remove_device(mapped_config.base_address);
  config.active = false;
  mapped_device = nullptr;
  mapped_offset = 0;
  mapped_config = RefSiMemoryWindowConfig();
  return (old_device == this) ? refsi_success : refsi_failure;
}

uint64_t RefSiMemoryWindowConfig::getScale() const {
  return (scale_a == 0) ? 0 : (1 << (scale_a - 1)) * (scale_b + 1);
}

refsi_result RefSiMemoryWindowConfig::setScale(uint64_t newScale) {
  if (newScale == 0) {
    scale_a = scale_b = 0;
    return refsi_success;
  }

  // Find the largest power-of-two value the scale can be evenly divided by.
  constexpr const uint64_t max_scale_a = 31;
  scale_a = 0;
  do {
    if (scale_a == max_scale_a) {
      break;
    } else if (newScale % (1ull << (scale_a + 1))) {
      break;
    }
    scale_a++;
  } while (true);

  // Compute scale_b based on scale_a.
  constexpr const uint64_t max_scale_b = 1ull << 32;
  scale_b = newScale >> (scale_a - 1);
  if (scale_b > max_scale_b) {
    scale_a = scale_b = 0;
    return refsi_failure;
  }
  scale_b--;

  return refsi_success;
}

refsi_result RefSiMemoryWindow::getEffectiveAddress(reg_t addr, unit_id_t unit,
                                                    refsi_addr_t &eff_address) {
  if (mapped_config.mode == CMP_WINDOW_MODE_SHARED) {
    eff_address = mapped_offset + addr;
    return refsi_success;
  } else if (mapped_config.mode != CMP_WINDOW_MODE_PERT_HART) {
    return refsi_failure;
  } else if (get_unit_kind(unit) != unit_kind::acc_hart) {
    return refsi_failure;
  }

  uint16_t hart_id = get_unit_index(unit);
  eff_address = mapped_offset + (hart_id * mapped_config.getScale()) + addr;
  return refsi_success;
}

size_t RefSiMemoryWindow::mem_size() const {
  return isMapped() ? mapped_config.size : config.size;
}

bool RefSiMemoryWindow::load(reg_t addr, size_t len, uint8_t *bytes,
                             unit_id_t unit) {
  refsi_addr_t target_addr = 0;
  if ((addr + len) >= config.size) {
    return false;
  } else if (!mapped_device) {
    return false;
  } else if (refsi_success != getEffectiveAddress(addr, unit, target_addr)) {
    return false;
  } else {
    return mapped_device->load(target_addr, len, bytes, unit);
  }
}

bool RefSiMemoryWindow::store(reg_t addr, size_t len, const uint8_t *bytes,
                              unit_id_t unit) {
  refsi_addr_t target_addr = 0;
  if ((addr + len) >= config.size) {
    return false;
  } else if (!mapped_device) {
    return false;
  } else if (refsi_success != getEffectiveAddress(addr, unit, target_addr)) {
    return false;
  } else {
    return mapped_device->store(target_addr, len, bytes, unit);
  }
}

uint8_t *RefSiMemoryWindow::addr_to_mem(reg_t addr, size_t size,
                                        unit_id_t unit) {
  refsi_addr_t target_addr = 0;
  if (!mapped_device) {
    return nullptr;
  } else if (refsi_success != getEffectiveAddress(addr, unit, target_addr)) {
    return nullptr;
  } else {
    return mapped_device->addr_to_mem(target_addr, size, unit);
  }
  return nullptr;
}
