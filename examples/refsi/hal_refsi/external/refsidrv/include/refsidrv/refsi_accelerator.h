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

#ifndef _REFSIDRV_REFSI_ACCELERATOR_H
#define _REFSIDRV_REFSI_ACCELERATOR_H

#include "refsidrv.h"
#include "slim_sim.h"
#include "trap_handlers.h"

#include <memory>

struct RefSiDevice;

class processor_t;
class csr_t;

/// @brief Contains per-hart data needed to execute a kernel entry point on
/// accelerator cores.
struct hart_state_entry {
  /// @brief Address of the Kernel Thread Block for the given hart.
  uint64_t ktb_addr;
  /// @brief Address to use for the hart's stack pointer.
  uint64_t stack_top_addr;
  /// @brief Extra arguments to pass to the entry point function.
  std::vector<uint64_t> extra_args;
};

/// @brief Represents a RefSi accelerator in a RefSi platform. The accelerator
/// contains several RISC-V cores, each containing several harts, and can be
/// used to execute kernels in parallel fashion.
struct RefSiAccelerator {
  RefSiAccelerator(RefSiDevice &soc);

  /// @brief String that describes the ISA exposed by accelerator cores.
  const char * getISA() const { return isa.empty() ? nullptr : isa.c_str(); }
  void setISA(std::string new_isa) { isa = new_isa; }

  /// @brief String that describes the vector ISA exposed by accelerator cores.
  std::string getVectorArch() const;

  /// @brief Width of the accelerator cores' vector registers, in bits.
  unsigned getVectorLen() const { return vlen; }
  void setVectorLen(unsigned new_len) { vlen = new_len; }

  /// @brief Maximum width of an element in a vector register, in bits.
  unsigned getVectorElemLen() const { return elen; }
  void setVectorElemLen(unsigned new_len) { elen = new_len; }

  /// @brief Total number of RISC-V harts in the accelerator.
  unsigned getNumHarts() const { return total_harts; }
  void setNumHarts(unsigned num_harts) { total_harts = num_harts; }

  slim_sim_callback get_pre_run_callback() const { return pre_run_callback; }
  void set_pre_run_callback(slim_sim_callback cb) { pre_run_callback = cb; }

  refsi_result createSim();

  /// @brief Run a kernel slice command on the RefSi accelerator. The kernel's
  /// entry point function is executed @p num_instances times, distributed
  /// between the harts in the accelerator.
  /// @param num_instances Number of times to execute the entry point function.
  /// @param entry_point Address of the entry point function.
  /// @param return_addr Address to jump to when the kernel function returns.
  /// @param extra_args Extra arguments to pass to the entry point function.
  /// @param num_harts Number of harts to use for executing the kernel slice.
  /// @param hart_data Array of hart-specific data needed to execute the entry
  /// point function on each hart.
  refsi_result runKernelSlice(uint64_t num_instances, reg_t entry_point,
                              reg_t return_addr, uint32_t num_harts,
                              const hart_state_entry *hart_data);

  /// @brief Run a kernel on the RefSi G1 accelerator. This resets all of the
  /// accelerator's harts, so that the bootloader can execute the kernel. It is
  /// the bootloader's responsibility to schedule the work between the harts.
  refsi_result runKernelGeneric(uint32_t num_harts);

  /// @brief Synchronize the RefSi M1 accelerator with the rest of the system
  /// by flushing and/or invalidating its caches.
  refsi_result syncCache(uint32_t flags);

  /// @brief Read a specific hart's performance counter.
  /// @param counter_id Index of the counter to read.
  /// @param hart_id Index of the hart that owns the counter.
  /// @param value On success, value read from the performance counter.
  refsi_result readPerfCounter(uint32_t counter_id, uint32_t hart_id,
                               uint64_t &value);

  /// @brief Write a value to a specific hart's performance counter.
  /// @param counter_id Index of the counter to write to.
  /// @param hart_id Index of the hart that owns the counter.
  /// @param value Value to write to the performance counter.
  refsi_result writePerfCounter(uint32_t counter_id, uint32_t hart_id,
                                uint64_t &value);

 private:
  /// @brief Perform common hart initialization.
  void initializeHart(processor_t *hart);
  /// @brief Maps a performance counter index to a CSR register index.
  refsi_result getPerfCounterReg(uint32_t counter_idx, reg_t &reg_idx);
  csr_t *getCSR(uint32_t hart_id, uint32_t csr_idx);

  RefSiDevice &soc;
  std::string isa;
  unsigned vlen;
  unsigned elen;
  unsigned total_harts;
  slim_sim_callback pre_run_callback;
  slim_sim_config kernel_config;
  std::unique_ptr<slim_sim_t> sim;
};

/// @brief Trap handler that detects traps which are the result of returning
/// from the kernel's entry point function. When such a trap is detected, the
/// simulator is notified that the currently-executing hart has exited
/// gracefully.
class RefSiTrapHandler : public default_trap_handler {
public:
  bool handle_trap(trap_t &trap, reg_t pc, slim_sim_t &sim) override;
  bool handle_return(trap_t &trap, reg_t pc, slim_sim_t &sim);

  reg_t get_return_addr() const { return return_addr; }
  void set_return_addr(reg_t new_addr) { return_addr = new_addr; }

private:
  reg_t return_addr = 0;
};

#endif  // _REFSIDRV_REFSI_ACCELERATOR_H
