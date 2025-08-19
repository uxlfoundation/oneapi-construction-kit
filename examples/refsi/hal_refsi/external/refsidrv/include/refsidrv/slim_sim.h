// See LICENSE.spike for license details.

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

#ifndef _RISCV_SLIM_SIM_H
#define _RISCV_SLIM_SIM_H

#include "riscv/devices.h"
#include "riscv/log_file.h"
#include "riscv/processor.h"
#include "riscv/simif.h"
#include "fesvr/memif.h"
#include "common_devices.h"

#include <vector>
#include <bitset>
#include <string>
#include <memory>
#include <functional>
#include <sys/types.h>

class mmu_t;
class debugger_t;
class trap_handler_t;

struct slim_sim_config {
  slim_sim_config();

  bool debug = false;
  bool log = false;
  bool hal_debug = false;
  size_t num_harts = 0;
  unsigned pmp_num = 0;
  unsigned pmp_granularity = 0;
  bool log_commits = false;
  const char *log_path = nullptr;
  const char* isa = nullptr;
  const char* priv = nullptr;
  std::string varch;
  uint32_t vlen = 0;
};

using slim_sim_callback = std::function<void (slim_sim_t &)>;

// this class encapsulates the processors and memory in a RISC-V machine.
class slim_sim_t : public simif_t
{
public:
  slim_sim_t(const slim_sim_config &config, MemoryInterface &mem_if);
  ~slim_sim_t();

  bool is_running() const { return !signal_exit; }

  debugger_t & get_debugger() { return *debugger; }
  slim_sim_callback get_pre_run_callback() const { return pre_run_callback; }
  void set_pre_run_callback(slim_sim_callback cb) { pre_run_callback = cb; }

  // run the simulation to completion
  int run();

  // run interactive
  void run_single_step(bool noisy, size_t steps);

  void set_debug(bool value);
  void set_procs_debug(bool value);

  // Configure logging
  //
  // If enable_log is true, an instruction trace will be generated. If
  // enable_commitlog is true, so will the commit results (if this
  // build was configured without support for commit logging, the
  // function will print an error message and abort).
  void configure_log(bool enable_log, bool enable_commitlog);

  size_t get_current_hart_id() const { return current_hart_id; }
  processor_t* get_hart(size_t index) const;
  size_t get_hart_number() const;
  size_t get_max_active_harts() const { return max_harts; }
  void set_max_active_harts(size_t max_harts);

  trap_handler_t* get_trap_handler() const { return trap_handler; }
  void set_trap_handler(trap_handler_t *handler) { trap_handler = handler; }

  void set_exited(reg_t exit_code);
  bool handle_barrier(reg_t link_address);

  // Callback for processors to let the simulation know they were reset.
  void proc_reset(unsigned id) override;

  // memory-mapped I/O routines
  char* addr_to_mem(reg_t addr) override;
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes) override;
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes) override;
  bool mmio_print(reg_t addr);
  const char *get_symbol(uint64_t addr) override { return "UNKNOWN_SYMBOL"; }

 private:
  std::vector<processor_t*> harts;
  size_t max_harts = 0;
  reg_t entry = DRAM_BASE;
  MemoryInterface &mem_if;
  log_file_t log_file;

  void step(size_t n); // step through simulation
  void handle_trap(processor_t *hart);
  void handle_breakpoint(processor_t *hart);
  void return_from_trap(state_t *hart_state, reg_t new_pc);

  static const size_t INTERLEAVE = 5000;
  static const size_t INSNS_PER_RTC_TICK = 100; // 10 MHz clock for 1 BIPS core
  static const size_t CPU_HZ = 1000000000; // 1GHz CPU
  size_t current_step;
  size_t current_hart_id;
  bool debug;
  bool log;
  bool signal_exit = false;
  std::bitset<REFSI_SIM_MAX_HARTS> is_hart_running;
  std::vector<reg_t> hart_barrier_address;
  int64_t exit_code = 0;
  trap_handler_t *trap_handler;
  isa_parser_t isa_parser;
  std::unique_ptr<debugger_t> debugger;
  slim_sim_callback pre_run_callback;
};

#endif
