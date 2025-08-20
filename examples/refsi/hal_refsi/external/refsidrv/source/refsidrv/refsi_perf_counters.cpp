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

#include "refsi_perf_counters.h"
#include "refsi_device.h"
#include "refsi_accelerator.h"

PerfCounterDevice::PerfCounterDevice(RefSiDevice &soc) : soc(soc) {
  global_counters.resize(num_global_perf_counters);
}

PerfCounterDevice::~PerfCounterDevice() {
}

bool PerfCounterDevice::get_perf_counter_index(reg_t rel_addr,
                                               size_t &counter_idx,
                                               bool &is_per_hart) const {
  uint64_t abs_idx = rel_addr / sizeof(uint64_t);
  counter_idx = 0;
  is_per_hart = false;
  if (rel_addr % sizeof(uint64_t)) {
    return false;
  } else if (abs_idx < num_per_hart_perf_counters) {
    // Per-hart counters are located at the start of the I/O memory area.
    counter_idx = abs_idx;
    is_per_hart = true;
    return true;
  }
  // Global counters are located after per-hart counters in the I/O memory area.
  counter_idx = abs_idx - num_per_hart_perf_counters;
  return (counter_idx < global_counters.size());
}

bool PerfCounterDevice::load(reg_t addr, size_t len, uint8_t *bytes,
                             unit_id_t unit_id) {
  // Handle multi-counter accesses.
  if (len > sizeof(uint64_t)) {
    if (len % sizeof(uint64_t)) {
      return false;
    }
    for (reg_t i = 0; i < len; i += sizeof(uint64_t)) {
      if (!load(addr + i, sizeof(uint64_t), bytes + i, unit_id)) {
        return false;
      }
    }
    return true;
  }

  // Retrieve and validate the counter index.
  size_t counter_idx = 0;
  bool is_per_hart = false;
  if (!get_perf_counter_index(addr, counter_idx, is_per_hart)) {
    return false;
  }

  // Read the performance counter.
  uint64_t val = 0;
  if (is_per_hart) {
    if (get_unit_kind(unit_id) != unit_kind::acc_hart) {
      return false;
    }
    RefSiAccelerator &acc = soc.getAccelerator();
    if (refsi_success != acc.readPerfCounter(counter_idx,
                                             get_unit_index(unit_id), val)) {
      return false;
    }
  } else {
    val = global_counters[counter_idx];
  }

  // Copy the value read from the performance counter to the caller.
  if (len == sizeof(uint64_t)) {
    memcpy(bytes, &val, len);
  } else if (len == sizeof(uint32_t)) {
    uint32_t lo_val = (uint32_t)val;
    memcpy(bytes, &lo_val, len);
  }
  return true;
}

bool PerfCounterDevice::store(reg_t addr, size_t len, const uint8_t *bytes,
                              unit_id_t unit_id) {
  // Multi-counter writes are not supported.
  if (len > sizeof(uint64_t)) {
    return false;
  }

  // Retrieve and validate the counter index.
  size_t counter_idx = 0;
  bool is_per_hart = false;
  if (!get_perf_counter_index(addr, counter_idx, is_per_hart)) {
    return false;
  }

  // Load the value to write to the counter.
  uint64_t val = 0;
  if (len == sizeof(uint64_t)) {
    val = *(const uint64_t *)bytes;
  } else if (len == sizeof(uint32_t)) {
    val = *(const uint32_t *)bytes;
  } else {
    return false;
  }

  // Write the value to the counter.
  if (is_per_hart) {
    if (get_unit_kind(unit_id) != unit_kind::acc_hart) {
      return false;
    }
    RefSiAccelerator &acc = soc.getAccelerator();
    if (refsi_success != acc.writePerfCounter(counter_idx,
                                              get_unit_index(unit_id), val)) {
      return false;
    }
  } else {
    global_counters[counter_idx] = val;
  }
  return true;
}

size_t PerfCounterDevice::mem_size() const {
  unsigned num_regs = num_global_perf_counters + num_per_hart_perf_counters;
  return num_regs * sizeof(uint64_t);
}
