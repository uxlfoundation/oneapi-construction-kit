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

#include "refsidrv/refsi_memory.h"

#include "refsidrv/refsi_device.h"
#include "refsidrv/refsi_memory_window.h"

RefSiMemoryController::RefSiMemoryController(RefSiDevice &soc) : soc(soc) {
  for (unsigned i = 0; i < num_memory_windows; i++) {
    windows.push_back(new RefSiMemoryWindow(*this));
  }
}

RefSiMemoryController::~RefSiMemoryController() {
  for (RefSiMemoryWindow *win_device : windows) {
    if (!win_device->isMapped()) {
      delete win_device;
    }
  }
  for (const auto &mapping : *this) {
    delete mapping.second;
  }
}

/// @brief Access the device's memory map.
const std::vector<refsi_memory_map_entry> &RefSiMemoryController::getMemoryMap()
    const {
  return memory_map;
}

RAMDevice *RefSiMemoryController::createMemRange(refsi_memory_map_kind kind,
                                                 reg_t address, size_t size) {
  RAMDevice *mem = new RAMDevice(size);
  addMemDevice(address, size, kind, mem);
  return mem;
}

void RefSiMemoryController::addMemDevice(reg_t address, size_t size,
                                         refsi_memory_map_kind kind,
                                         MemoryDevice *device) {
  add_device(address, device);
  memory_map.push_back({kind, address, size});
}

RefSiMemoryWindow *RefSiMemoryController::getWindow(unsigned index) const {
  return (index < windows.size()) ? windows[index] : 0;
}

refsi_result RefSiMemoryController::handleWindowRegWrite(
    refsi_cmp_register_id reg_idx, uint64_t value) {
  // Determine the window index and canonical register.
  uint32_t window_idx = 0;
  refsi_cmp_register_id canonical_reg = reg_idx;
  if (!RefSiMemoryWindow::splitCmpRegister(reg_idx, canonical_reg,
                                           window_idx)) {
    return refsi_failure;
  }

  // Delegate the register write to the window device.
  if (window_idx >= windows.size()) {
    return refsi_failure;
  }
  RefSiMemoryWindow *window = windows[window_idx];
  return window->handleRegWrite(canonical_reg, value, *this);
}
