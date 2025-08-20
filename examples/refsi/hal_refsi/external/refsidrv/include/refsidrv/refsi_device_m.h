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

#ifndef _REFSIDRV_REFSI_DEVICE_M_H
#define _REFSIDRV_REFSI_DEVICE_M_H

#include <memory>

#include "refsi_device.h"

struct RefSiCommandProcessor;
class DMADevice;
class PerfCounterDevice;

struct RefSiMDevice : public RefSiDevice {
public:
  RefSiMDevice();

  /// @brief Shut down the device.
  virtual ~RefSiMDevice();

  /// @brief Perform device initialization.
  refsi_result initialize() override;

  /// @brief Asynchronously execute a series of commands on the device.
  /// @param cb_addr Address of the command buffer in device memory.
  /// @param size Size of the command buffer, in bytes.
  refsi_result executeCommandBuffer(refsi_addr_t cb_addr, size_t size);

  /// @brief Wait for all previously enqueued command buffers to be finished.
  void waitForDeviceIdle();

 private:
  void preRunSim(slim_sim_t &sim);

  std::unique_ptr<RefSiCommandProcessor> cmp;
  RAMDevice *tcdm = nullptr;
  RAMDevice *dram = nullptr;
  DMADevice *dma_device = nullptr;
  PerfCounterDevice *perf_counter_device = nullptr;
};

#endif  // _REFSIDRV_REFSI_DEVICE_M_H
