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

#include "refsidrv/refsi_device_g.h"

#include "refsidrv/refsi_accelerator.h"
#include "refsidrv/refsi_memory.h"
#include "refsidrv/refsi_memory_window.h"
#include "refsidrv/refsi_perf_counters.h"

#include "device/memory_map.h"
#include "refsidrv/debugger.h"

// Default memory area for storing kernel ELF binaries. When the RefSi device
// does not have dedicated (TCIM) memory for storing kernel exeutables, a memory
// window is set up to map this memory area to a reserved area in DMA.
// We have increased the memory size from 1 << 20 to handle kernels larger than
// 1MiB
constexpr const uint64_t REFSI_ELF_BASE = 0x10000ull;
constexpr const uint64_t REFSI_ELF_SIZE = (1 << 27) - REFSI_ELF_BASE;

// Memory area for per-hart storage.
constexpr const uint64_t G_HART_LOCAL_BASE = 0x20800000;
constexpr const uint64_t G_HART_LOCAL_END = 0x21000000;
constexpr const uint64_t G_HART_LOCAL_SIZE = G_HART_LOCAL_END
  - G_HART_LOCAL_BASE;

RefSiGDevice::RefSiGDevice(const char *isa, unsigned vlen)
  : RefSiDevice(refsi_soc_family::g) {
  RefSiLock lock(mutex);
  mem_ctl = std::make_unique<RefSiMemoryController>(*this);
  size_t loader_size = REFSI_LOADER_END_ADDRESS - REFSI_LOADER_ADDRESS;
  loader_rom = new ROMDevice(loader_size);
  mem_ctl->addMemDevice(REFSI_LOADER_ADDRESS, loader_size, TCIM, loader_rom);
  tcdm = mem_ctl->createMemRange(TCDM, tcdm_base, tcdm_size);
  dram = mem_ctl->createMemRange(DRAM, dram_base, dram_size);
  perf_counter_device = new PerfCounterDevice(*this);
  mem_ctl->addMemDevice(perf_counters_io_base, perf_counters_io_size,
                        PERF_COUNTERS, perf_counter_device);

  accelerator = std::make_unique<RefSiAccelerator>(*this);
  accelerator->setISA(isa);
  accelerator->setVectorLen(vlen);
  accelerator->setNumHarts(2); // Default NUM_HARTS_FOR_CA_MODE
}

RefSiGDevice::~RefSiGDevice() {
  RefSiLock locker(mutex);
  allocator.free(elf_mem_mapped_addr);
  allocator.free(harts_mem_mapped_addr);
}

void RefSiGDevice::getDefaultConfig(const char *&isa, int &vlen) {
  isa = REFSI_G1_ISA;
  vlen = 128;
  if (const char *val = getenv("CA_RISCV_VLEN_BITS_MIN")) {
    vlen = std::atoi(val);
  }
}

refsi_result RefSiGDevice::queryDeviceInfo(refsi_device_info_t &device_info) {
  if (refsi_result error = RefSiDevice::queryDeviceInfo(device_info)) {
    return error;
  }
  device_info.num_harts_per_core = max_harts;
  return refsi_success;
}

refsi_result RefSiGDevice::initialize() {
  if (refsi_result result = accelerator->createSim()) {
    return result;
  }

  // Set up a memory window for ELF executables.
  if (refsi_result result = setupELFWindow(window_index_elf)) {
    return result;
  }

  // Set up a memory window for per-hart storage.
  if (refsi_result result = setupHartLocalWindow(window_index_harts)) {
    return result;
  }

  return refsi_success;
}

refsi_result RefSiGDevice::setupELFWindow(unsigned index) {
  // Allocate device memory for the window.
  refsi_addr_t elf_mem_base = REFSI_ELF_BASE;
  refsi_addr_t elf_mem_size = REFSI_ELF_SIZE;
  elf_mem_mapped_addr = allocDeviceMemory(elf_mem_size, 4096, DRAM);
  if (elf_mem_mapped_addr == 0) {
    return refsi_failure;
  }

  // Set up and enable the memory window.
  RefSiMemoryWindow *win = mem_ctl->getWindow(index);
  if (!win) {
    return refsi_failure;
  }
  RefSiMemoryWindowConfig &config = win->getConfig();
  config.base_address = elf_mem_base;
  config.target_address = elf_mem_mapped_addr;
  config.size = elf_mem_size;
  config.mode = CMP_WINDOW_MODE_SHARED;
  config.setScale(0);
  return win->enableWindow(*mem_ctl.get());
}

refsi_result RefSiGDevice::setupHartLocalWindow(unsigned index) {
  // Allocate device memory for the window.
  refsi_addr_t harts_mem_base = G_HART_LOCAL_BASE;
  refsi_addr_t harts_mem_size = G_HART_LOCAL_SIZE;
  harts_mem_mapped_addr = allocDeviceMemory(harts_mem_size * max_harts,
                                            4096, DRAM);
  if (harts_mem_mapped_addr == 0) {
    return refsi_failure;
  }

  // Set up and enable the memory window.
  RefSiMemoryWindow *win = mem_ctl->getWindow(index);
  if (!win) {
    return refsi_failure;
  }
  RefSiMemoryWindowConfig &config = win->getConfig();
  config.base_address = harts_mem_base;
  config.target_address = harts_mem_mapped_addr;
  config.size = harts_mem_size;
  config.mode = CMP_WINDOW_MODE_PERT_HART;
  config.setScale(harts_mem_size);
  return win->enableWindow(*mem_ctl.get());
}

refsi_result RefSiGDevice::executeKernel(refsi_addr_t entry_fn_addr,
                                         uint32_t num_harts) {
  // Run the kernel in the simulator.
  accelerator->set_pre_run_callback([&](slim_sim_t &sim) {
    pre_run_kernel(sim, entry_fn_addr);
  });
  return accelerator->runKernelGeneric(num_harts);
}

void RefSiGDevice::pre_run_kernel(slim_sim_t &sim, reg_t entry_point_addr) {
  // Run step by step in debug mode until we hit the kernel entry point
  debugger_t &debugger = sim.get_debugger();
  std::stringstream address;
  address << std::hex << entry_point_addr;

  debugger.set_cmd("until");
  debugger.set_args({"pc", "0", address.str()});
  debugger.do_until_silent();

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

  for (size_t i = 0; i < sim.get_hart_number(); i++) {
    processor_t *hart = sim.get_hart(i);
    if (profile_level > 2) {
      hart->get_state()->profiler_mode = true;
    }
    // When the profiler is enabled and start to run, we want to count the
    // number of instructions from that point. To do that we want to reset
    // the number of retired instructions that is always incremented by spike
    // regardless if the profiler is enabled.

    // The ISA mandates that if an instruction writes instret, the write
    // takes precedence over the increment to instret.  However, Spike
    // unconditionally increments instret after executing an instruction.
    // To correct this artifact, Spike decrementing instret after setting it.

    // Setting instret to 1, is actually setting it to 0 because we internally
    // reset it, it is not a CSR instructions that is setting it.
    hart->put_csr(CSR_MINSTRET, 1);
    hart->put_csr(CSR_MCYCLE, 1);
  }
}
