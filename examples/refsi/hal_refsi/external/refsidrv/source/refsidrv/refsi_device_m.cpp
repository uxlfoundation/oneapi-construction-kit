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

#include "refsidrv/refsi_device_m.h"

#include "refsidrv/refsi_accelerator.h"
#include "refsidrv/refsi_command_processor.h"
#include "refsidrv/refsi_memory.h"
#include "refsidrv/refsi_perf_counters.h"
#include "refsidrv/kernel_dma.h"

RefSiMDevice::RefSiMDevice() : RefSiDevice(refsi_soc_family::m) {
  RefSiLock lock(mutex);
  mem_ctl = std::make_unique<RefSiMemoryController>(*this);
  tcdm = mem_ctl->createMemRange(TCDM, tcdm_base, tcdm_size);
  dram = mem_ctl->createMemRange(DRAM, dram_base, dram_size);
  dma_device = new DMADevice(elf_machine::riscv_rv64, dma_io_base,
                             *mem_ctl.get(), debug);
  mem_ctl->addMemDevice(dma_device->get_base(), dma_io_size, KERNEL_DMA_PRIVATE,
                        dma_device);
  perf_counter_device = new PerfCounterDevice(*this);
  mem_ctl->addMemDevice(perf_counters_io_base, perf_counters_io_size,
                        PERF_COUNTERS, perf_counter_device);

  accelerator = std::make_unique<RefSiAccelerator>(*this);
  cmp = std::make_unique<RefSiCommandProcessor>(*this);
  accelerator->setISA(REFSI_M1_ISA);
  accelerator->set_pre_run_callback([&] (slim_sim_t &sim) { preRunSim(sim); });
}

RefSiMDevice::~RefSiMDevice() {
  RefSiLock locker(mutex);
  cmp->stop(locker);
}

refsi_result RefSiMDevice::initialize() {
  return accelerator->createSim();
}

refsi_result RefSiMDevice::executeCommandBuffer(refsi_addr_t cb_addr,
                                                size_t size) {
  RefSiLock lock(mutex);
  cmp->enqueueRequest({cb_addr, size}, lock);
  return refsi_success;
}

void RefSiMDevice::waitForDeviceIdle() {
  RefSiLock lock(mutex);
  cmp->waitEmptyQueue(lock);
}

void RefSiMDevice::preRunSim(slim_sim_t &sim) {
  // We only need to enable the sim's profiler_mode if the profile level is
  // set to 3, as the instruction and cycle counts will be captured
  // regardless. This mode causes huge slowdowns so as a slight hack we can
  // read the env var here and only enable it if it is really needed.
  int profile_level = 0;
  if (const char *profile_env = std::getenv("CA_PROFILE_LEVEL")) {
    if (int profile_env_val = atoi(profile_env)) {
      profile_level = profile_env_val;
    }
  }

  if (profile_level > 2) {
    for (size_t i = 0; i < sim.get_hart_number(); i++) {
      sim.get_hart(i)->get_state()->profiler_mode = true;
    }
  }
}
