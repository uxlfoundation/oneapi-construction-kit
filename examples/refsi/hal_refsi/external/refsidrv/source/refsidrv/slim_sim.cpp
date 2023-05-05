// See LICENSE.spike for license details.
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

#include "slim_sim.h"
#include "debugger.h"
#include "common_devices.h"
#include "trap_handlers.h"
#include "riscv/mmu.h"
#include "fesvr/byteorder.h"
#include "profiler.h"
#include <fstream>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <signal.h>

slim_sim_config::slim_sim_config() {
  debug = false;
  if (const char *val = getenv("SPIKE_SIM_DEBUG")) {
    if (strcmp(val, "0") != 0) {
      debug = true;
    }
  }
  log = false;
  log_path = nullptr;
  if (const char *val = getenv("SPIKE_SIM_LOG")) {
    if (strcmp(val, "0") == 0) {
      log = false;
    } else {
      log = true;
      if (strcmp(val, "1") != 0) {
        log_path = val;
      }
    }
  }
  hal_debug = false;
  if (const char *val = getenv("CA_HAL_DEBUG")) {
    if (strcmp(val, "0") != 0) {
      hal_debug = true;
    }
  }

  num_harts = 1;
  pmp_num = 16;
  pmp_granularity = 4;
  log_commits = false;
  priv = DEFAULT_PRIV;
}

slim_sim_t::slim_sim_t(const slim_sim_config &config, MemoryInterface &mem_if)
    : harts(std::max(config.num_harts, size_t(1))),
      mem_if(mem_if),
      log_file(config.log_path),
      current_step(0),
      current_hart_id(0),
      debug(config.debug),
      log(false),
      isa_parser(config.isa, config.priv) {
  debugger.reset(new debugger_t(*this));

  for (size_t i = 0; i < config.num_harts; i++) {
    int hart_id = i;
    harts[i] = new processor_t(&isa_parser, config.varch.c_str(), this, hart_id,
                               /* halted */ false, log_file.get(), std::cout);
  }

  for (size_t i = 0; i < config.num_harts; i++) {
    harts[i]->set_pmp_num(config.pmp_num);
    harts[i]->set_pmp_granularity(config.pmp_granularity);
  }
  hart_barrier_address.resize(config.num_harts, 0);

  configure_log(config.log, config.log_commits);
}

slim_sim_t::~slim_sim_t() {
  for (size_t i = 0; i < harts.size(); i++) {
    delete harts[i];
  }
}

int slim_sim_t::run() {
  exit_code = 0;
  signal_exit = false;
  current_hart_id = 0;
  current_step = 0;
  for (size_t i = 0; i < harts.size(); i++) {
    is_hart_running[i] = (i < get_hart_number());
    hart_barrier_address[i] = 0;
    harts[i]->get_state()->profiler_mode = false;
  }
  if (!debug && log) {
    set_procs_debug(true);
  }

  // Execute the pre-run callback when set by the user. This can be used to do
  // tasks such as executing the program until a specific point or do some
  // specific initialization.
  if (pre_run_callback) {
    pre_run_callback(*this);
  }

  while (!signal_exit) {
    if (debug) {
      while (!signal_exit) {
        debugger->read_command();
        debugger->run_command();
      }
    } else {
      step(INTERLEAVE);
    }
  }

  return exit_code;
}

void slim_sim_t::step(size_t n) {
  for (size_t i = 0, steps = 0; i < n; i += steps) {
    steps = std::min(n - i, INTERLEAVE - current_step);
    if (is_hart_running[current_hart_id]) {
      processor_t *hart = harts[current_hart_id];
      state_t *hart_state = hart->get_state();
      hart->step(steps);
      if (hart_state->mcause->read() != 0 && trap_handler) {
        handle_trap(hart);
      } else if (hart_state->pc == hart_state->bp_addr) {
        handle_breakpoint(hart);
      }
    }

    current_step += steps;
    if (current_step == n) {
      current_step = 0;
      if (is_hart_running[current_hart_id]) {
        harts[current_hart_id]->get_mmu()->yield_load_reservation();
      }
      if (++current_hart_id == get_hart_number()) {
        // TODO find the next 'still running' hart.
        current_hart_id = 0;
      }
    }
  }
}

void slim_sim_t::run_single_step(bool noisy, size_t steps) {
  set_procs_debug(noisy);
  for (size_t i = 0; i < steps && !signal_exit; i++) {
    step(1);
  }
}

void slim_sim_t::handle_trap(processor_t *hart) {
  state_t *hart_state = hart->get_state();
  mem_trap_t trap(hart_state->mcause->read(), false,
                  hart_state->mtval->read(), hart_state->mtval2->read(),
                  hart_state->mtinst->read());
  if (trap_handler->handle_trap(trap, hart_state->mepc->read(), *this)) {
    // Calculate the PC of the instruction following the one that caused the
    // trap.
    reg_t old_pc = hart_state->mepc->read();
    reg_t new_pc = old_pc;
    try {
      insn_fetch_t fetch = hart->get_mmu()->load_insn(old_pc);
      new_pc += fetch.insn.length();
    }
    catch (trap_instruction_access_fault) {}

    // Restore the previous state and resume execution.
    return_from_trap(hart_state, new_pc);
  }
}

void slim_sim_t::handle_breakpoint(processor_t *hart) {
  state_t *hart_state = hart->get_state();
  trap_t trap(CAUSE_BREAKPOINT);
  if (trap_handler) {
    trap_handler->handle_trap(trap, hart_state->pc, *this);
  } else {
    set_exited(0x80000000 | CAUSE_BREAKPOINT);
  }
}

void slim_sim_t::return_from_trap(state_t *hart_state, reg_t new_pc) {
  hart_state->mcause->write(0);
  hart_state->mtval->write(0);
  hart_state->mtval2->write(0);
  hart_state->mtinst->write(0);
  hart_state->mepc->write(0);
  hart_state->pc = new_pc;
  // TODO: restore mstatus for completeness
}

processor_t* slim_sim_t::get_hart(size_t index) const {
  return (index < get_hart_number()) ? harts[index] : nullptr;
}

size_t slim_sim_t::get_hart_number() const {
  return (max_harts > 0) ? std::min(harts.size(), max_harts) : harts.size();
}

void slim_sim_t::set_max_active_harts(size_t new_max_harts) {
  max_harts = new_max_harts;
}

void slim_sim_t::set_debug(bool value) {
  debug = value;
}

void slim_sim_t::set_procs_debug(bool value) {
  for (size_t i = 0; i < get_hart_number(); i++) {
    harts[i]->set_debug(value);
  }
}

void slim_sim_t::configure_log(bool enable_log, bool enable_commitlog) {
  log = enable_log;

  if (!enable_commitlog)
    return;

#ifndef RISCV_ENABLE_COMMITLOG
  fputs("Commit logging support has not been properly enabled; "
        "please re-build the riscv-isa-sim project using "
        "\"configure --enable-commitlog\".\n",
        stderr);
  abort();
#else
  for (processor_t *proc : procs) {
    proc->enable_log_commits();
  }
#endif
}

static bool paddr_ok(reg_t addr) {
  return (addr >> MAX_PADDR_BITS) == 0;
}

bool slim_sim_t::mmio_load(reg_t addr, size_t len, uint8_t* bytes) {
  if (addr + len < addr || !paddr_ok(addr + len - 1)) {
    return false;
  }
  unit_id_t unit = make_unit(unit_kind::acc_hart, get_current_hart_id());
  return mem_if.load(addr, len, bytes, unit);
}

bool slim_sim_t::mmio_store(reg_t addr, size_t len, const uint8_t* bytes) {
  if (addr + len < addr || !paddr_ok(addr + len - 1)) {
    return false;
  }
  unit_id_t unit = make_unit(unit_kind::acc_hart, get_current_hart_id());
  return mem_if.store(addr, len, bytes, unit);
}

char* slim_sim_t::addr_to_mem(reg_t addr) {
  if (!paddr_ok(addr)) {
    return NULL;
  }
  unit_id_t unit = make_unit(unit_kind::acc_hart, get_current_hart_id());
  return (char *)mem_if.addr_to_mem(addr, sizeof(uint8_t), unit);
}

void slim_sim_t::proc_reset(unsigned id) {
}

void slim_sim_t::set_exited(reg_t exit_code) {
  if (exit_code != 0) {
    // When a thread exits with a non-zero code, abort simulation.
    is_hart_running.reset();
  } else {
    // When a thread exits gracefully, wait for other threads to have finished
    // executing before stopping the simulator.
    is_hart_running[current_hart_id] = false;
    if (is_hart_running.any()) {
      return;
    }
  }
  this->exit_code = exit_code;
  signal_exit = true;
}

bool slim_sim_t::handle_barrier(reg_t link_address) {
  // Put the hart to sleep and record the link address. It is used to identify
  // the call site of the barrier in user code and error when different harts
  // hit different barriers at the same time.
  hart_barrier_address[current_hart_id] = link_address;
  is_hart_running[current_hart_id] = false;

  // Wait for all harts to be asleep.
  if (is_hart_running.any()) {
    return true;
  }

  // Ensure that all harts hit the same barrier.
  reg_t barrier_address = hart_barrier_address[0];
  for (size_t i = 1; i < get_hart_number(); i++) {
    if (hart_barrier_address[i] != barrier_address) {
      fprintf(stderr, "error: all threads must hit the same barrier\n");
      set_exited(-1);
      return false;
    }
  }

  // Reset the barrier state and wake up all harts.
  for (size_t i = 0; i < get_hart_number(); i++) {
    hart_barrier_address[i] = 0;
    is_hart_running[i] = true;
  }
  return true;
}

bool slim_sim_t::mmio_print(reg_t addr) {
  // Fast path: the message to print is stored in regular memory.
  char *data = addr_to_mem(addr);
  if (data) {
    printf("%s", data);
    return true;
  }

  // Slow path: the message to print is stored in special memory, like hart
  // local storage or ROM.
  const unsigned chunk_size = 8;
  uint8_t chunk[chunk_size];
  std::string message;
  reg_t current_addr = addr;
  bool found_terminator = false;
  if (!mmio_load(current_addr, chunk_size, chunk)) {
    return false;
  }
  while (!found_terminator) {
    size_t num_chars = 0;
    for (size_t i = 0; i < chunk_size; i++) {
      if (chunk[i] == 0) {
        found_terminator = true;
        break;
      }
      num_chars++;
    }
    message.insert(message.end(), chunk, chunk + num_chars);
    current_addr += chunk_size;
    if (found_terminator) {
      break;
    }
    if (!mmio_load(current_addr, chunk_size, chunk)) {
      return false;
    }
  }
  printf("%s", message.c_str());
  return true;
}
