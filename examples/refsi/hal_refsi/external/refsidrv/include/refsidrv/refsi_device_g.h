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

#ifndef _REFSIDRV_REFSI_DEVICE_G_H
#define _REFSIDRV_REFSI_DEVICE_G_H

#include "refsi_device.h"

class PerfCounterDevice;

struct RefSiGDevice : public RefSiDevice {
public:
  RefSiGDevice(const char *isa, unsigned vlen);

  /// @brief Shut down the device.
  virtual ~RefSiGDevice();

  /// @brief Perform device initialization.
  refsi_result initialize() override;

  /// @brief Query information about the device.
  /// @param device_info To be filled with information about the device.
  refsi_result queryDeviceInfo(refsi_device_info_t &device_info) override;

  refsi_result executeKernel(refsi_addr_t entry_fn_addr, uint32_t num_harts);

  /// @brief Retrieve the default RISC-V configuration for RefSi G1 devices.
  static void getDefaultConfig(const char * &isa, int &vlen);

private:
  refsi_result setupELFWindow(unsigned index);
  refsi_result setupHartLocalWindow(unsigned index);
  void pre_run_kernel(slim_sim_t &sim, reg_t entry_point_addr);

  static constexpr const unsigned window_index_elf = 0;
  static constexpr const unsigned window_index_harts = 1;
  unsigned max_harts = REFSI_SIM_MAX_HARTS;
  refsi_addr_t elf_mem_mapped_addr = 0;
  refsi_addr_t harts_mem_mapped_addr = 0;

  RAMDevice *tcdm = nullptr;
  RAMDevice *dram = nullptr;
  ROMDevice *loader_rom = nullptr;
  PerfCounterDevice *perf_counter_device = nullptr;
};

#endif  // _REFSIDRV_REFSI_DEVICE_G_H
