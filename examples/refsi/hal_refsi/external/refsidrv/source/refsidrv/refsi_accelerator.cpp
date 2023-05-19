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

#include "refsidrv/refsi_accelerator.h"

#include "kernel_dma.h"
#include "refsidrv/refsi_device.h"
#include "refsidrv/refsi_memory.h"
#include "device/host_io_regs.h"
#include "riscv/mmu.h"
#include "trap.h"

RefSiAccelerator::RefSiAccelerator(RefSiDevice &soc) : soc(soc) {
  vlen = core_vlen;
  elen = core_elen;
  total_harts = num_harts_per_core;
}

std::string RefSiAccelerator::getVectorArch() const {
  std::string varch = "vlen:";
  varch += std::to_string(getVectorLen());
  varch += ",elen:";
  varch += std::to_string(getVectorElemLen());
  return varch;
}

refsi_result RefSiAccelerator::getPerfCounterReg(uint32_t counter_idx,
                                                 reg_t &reg_idx) {
  if (counter_idx >= num_per_hart_perf_counters) {
    return refsi_failure;
  }
  reg_idx = CSR_MCYCLE + counter_idx;
  return refsi_success;
}

csr_t *RefSiAccelerator::getCSR(uint32_t hart_id, uint32_t csr_idx) {
  processor_t *hart = sim->get_hart(hart_id);
  if (!hart) {
    return nullptr;
  }
  state_t *state = hart->get_state();
  auto csr_it = state->csrmap.find(csr_idx);
  if (csr_it == state->csrmap.end()) {
    return nullptr;
  }
  return csr_it->second.get();
}

refsi_result RefSiAccelerator::readPerfCounter(uint32_t counter_id,
                                               uint32_t hart_id,
                                               uint64_t &value) {
  reg_t reg_idx = 0;
  if (refsi_result result = getPerfCounterReg(counter_id, reg_idx)) {
    return result;
  }
  if (csr_t *csr = getCSR(hart_id, reg_idx)) {
    value = csr->read();
  } else {
    // 'Missing' performance counter CSRs always read zero.
    value = 0;
  }
  return refsi_success;
}

refsi_result RefSiAccelerator::writePerfCounter(uint32_t counter_id,
                                                uint32_t hart_id,
                                                uint64_t &value) {
  reg_t reg_idx = 0;
  refsi_result result = getPerfCounterReg(counter_id, reg_idx);
  if (result != refsi_success) {
    return result;
  }
  if (csr_t *csr = getCSR(hart_id, reg_idx)) {
    csr->write(value);
  } else {
    // Writing to a 'missing' performance counter CSRs is a no-op.
  }
  return refsi_success;
}

refsi_result RefSiAccelerator::createSim() {
  if (!getISA()) {
    return refsi_failure;
  } else if (kernel_config.num_harts > REFSI_SIM_MAX_HARTS) {
    return refsi_failure;
  }
  kernel_config.isa = getISA();
  kernel_config.varch = getVectorArch();
  kernel_config.vlen = getVectorLen();
  kernel_config.num_harts = total_harts;
  sim.reset(new slim_sim_t(kernel_config, soc.getMemory()));
  for (size_t j = 0; j < kernel_config.num_harts; j++) {
    initializeHart(sim->get_hart(j));
  }
  return refsi_success;
}

refsi_result RefSiAccelerator::runKernelSlice(
    uint64_t num_instances, reg_t entry_point, reg_t return_addr,
    uint32_t num_harts, const hart_state_entry *hart_data) {
  if (!sim) {
    if (refsi_result result = createSim()) {
      return result;
    }
  }
  const uint32_t max_extra_args = 7;
  for (uint32_t i = 0; i < num_harts; i++) {
    if (hart_data[i].extra_args.size() > max_extra_args) {
      return refsi_failure;
    }
  }

  RefSiTrapHandler trap_handler;
  trap_handler.set_return_addr(return_addr);
  sim->set_trap_handler(&trap_handler);

  // Put a breakpoint on the kernel return address.
  sim->set_max_active_harts(num_harts);
  for (size_t j = 0; j < num_harts; j++) {
    processor_t *hart = sim->get_hart(j);
    hart->get_state()->bp_addr = return_addr;
  }

  // Run all instances on the simulator, one 'hart group' at a time.
  uint64_t instance_id = 0;
  size_t instances_left = num_instances;
  refsi_result result = refsi_success;
  sim->set_pre_run_callback(pre_run_callback);
  while (instances_left > 0) {
    size_t num_active_harts = std::min(instances_left, (size_t)num_harts);
    sim->set_max_active_harts(num_active_harts);

    // Set the per-hart state for the current 'hart group'.
    for (size_t j = 0; j < num_active_harts; j++) {
      const hart_state_entry &hart_entry(hart_data[j]);
      processor_t *hart = sim->get_hart(j);
      state_t *cpu_state = hart->get_state();
      cpu_state->pc = entry_point;
      // ra - return address
      cpu_state->XPR.write(1, return_addr);
      // sp - stack
      cpu_state->XPR.write(2, hart_entry.stack_top_addr);
      // a0 - instance ID
      cpu_state->XPR.write(10, instance_id);
      // a1 to a7 - extra arguments
      for (size_t i = 0; i < hart_entry.extra_args.size(); i++) {
        cpu_state->XPR.write(11 + i, hart_entry.extra_args[i]);
      }
      instance_id++;
      instances_left--;
    }

    // Run the 'hart group' on the simulator.
    if (sim->run() != 0) {
      result = refsi_failure;
      break;
    }
  }
  sim->set_trap_handler(nullptr);

  // Clear the breakpoint.
  sim->set_max_active_harts(num_harts);
  for (size_t j = 0; j < num_harts; j++) {
    processor_t *hart = sim->get_hart(j);
    hart->get_state()->bp_addr = ~0ull;
  }

  return result;
}

void RefSiAccelerator::initializeHart(processor_t *hart) {
  // Initialize mstatus.
  reg_t mstatus = hart->get_state()->mstatus->read();
  if (hart->extension_enabled('D') || hart->extension_enabled('F')) {
    mstatus |= MSTATUS_FS;  // Enable FPU.
  }
  if (hart->extension_enabled('V')) {
    mstatus |= MSTATUS_VS;  // Enable RVV.
  }
  hart->get_state()->mstatus->write( mstatus);

  // Enable user/supervisor use of perf counters.
  if (hart->extension_enabled('S')) {
    hart->put_csr(CSR_SCOUNTEREN, -1);
  }
  hart->put_csr(CSR_MCOUNTEREN, -1);

  // Disable paging.
  if (hart->extension_enabled('S')) {
    hart->put_csr(CSR_SATP, 0);
  }
}

refsi_result RefSiAccelerator::runKernelGeneric(uint32_t num_harts) {
  // Reset all the harts.
  setNumHarts(num_harts);
  if (refsi_result result = createSim()) {
    return result;
  }

  // Boot the harts and simulate them until they exit.
  RefSiTrapHandler trap_handler;
  trap_handler.set_return_addr(0xffffffff00defafaull);
  sim->set_max_active_harts(num_harts);
  sim->set_trap_handler(&trap_handler);
  int exit_code = sim->run();
  sim->set_trap_handler(nullptr);
  return exit_code == 0 ? refsi_success : refsi_failure;
}

refsi_result RefSiAccelerator::syncCache(uint32_t flags) {
  size_t old_max_harts = sim->get_max_active_harts();
  sim->set_max_active_harts(0);
  for (size_t i = 0; i < sim->get_hart_number(); i++) {
    processor_t *hart = sim->get_hart(i);
    if (flags & CMP_CACHE_SYNC_ACC_DCACHE) {
      hart->get_mmu()->flush_tlb();
    } else {
      hart->get_mmu()->flush_icache();
    }
  }
  sim->set_max_active_harts(old_max_harts);
  return refsi_success;
}

bool RefSiTrapHandler::handle_trap(trap_t &trap, reg_t pc, slim_sim_t &sim) {
  if ((trap.cause() == CAUSE_FETCH_ACCESS) && (pc == get_return_addr())) {
    if (handle_return(trap, pc, sim)) {
      return true;
    }
  } else if ((trap.cause() == CAUSE_BREAKPOINT) && (pc == get_return_addr())) {
    if (handle_return(trap, pc, sim)) {
      return true;
    }
  }
  return default_trap_handler::handle_trap(trap, pc, sim);
}

bool RefSiTrapHandler::handle_return(trap_t &trap, reg_t pc, slim_sim_t &sim) {
  // When a kernel returns, execution jumps to the return address set in the
  // 'ra' register prior to starting the kernel. This causes an instruction
  // access fault trap, which we can distinguish from other traps with the
  // specific return address.
  sim.set_exited(
      0);  // Let the simulator know the hart has exited gracefully.
  return true;
}
