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

#include "machine.h"
#include <stdarg.h>

static void mstatus_init() {
  uintptr_t mstatus = 0;

  // Enable FPU
  if (supports_extension('D') || supports_extension('F'))
    mstatus |= MSTATUS_FS;

  // Enable vector extension
  if (supports_extension('V'))
    mstatus |= MSTATUS_VS;

  write_csr(mstatus, mstatus);

  // Enable user/supervisor use of perf counters
  if (supports_extension('S'))
    write_csr(scounteren, -1);
  write_csr(mcounteren, -1);

  // Enable software interrupts
  write_csr(mie, MIP_MSIP);

  // Disable paging
  if (supports_extension('S'))
    write_csr(satp, 0);
}

static void fp_init() {
  if (!supports_extension('D') && !supports_extension('F'))
    return;

  assert(read_csr(mstatus) & MSTATUS_FS);
  write_csr(fcsr, 0);
}

void init_hart(uintptr_t hartid) {
  mstatus_init();
  fp_init();
  setup_pmp();
  execute_nd_range();
}

void setup_pmp() {
  // Set up a PMP to permit access to all of memory.
  // Ignore the illegal-instruction trap if PMPs aren't supported.
  uintptr_t pmpc = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
  asm volatile ("la t0, 1f\n\t"
                "csrrw t0, mtvec, t0\n\t"
                "csrw pmpaddr0, %1\n\t"
                "csrw pmpcfg0, %0\n\t"
                ".align 2\n\t"
                "1: csrw mtvec, t0"
                : : "r" (pmpc), "r" (-1UL) : "t0");
}

uintptr_t report_machine_trap(uintptr_t* regs, uintptr_t dummy, uintptr_t mepc) {
  uintptr_t mcause = read_csr(mcause);
  switch (mcause) {
  case CAUSE_FETCH_ACCESS:
    printm("error: 'Instruction Access Fault' exception was raised @ %p\n",
           mepc);
    break;
  case CAUSE_ILLEGAL_INSTRUCTION:
    printm("error: 'Illegal Instruction' exception was raised @ %p\n",
           mepc);
    break;
  case CAUSE_LOAD_ACCESS:
    printm("error: 'Load Access Fault' exception was raised @ %p "
           "(badaddr = %p)\n", mepc, read_csr(mtval));
    break;
  case CAUSE_STORE_ACCESS:
    printm("error: 'Store/AMO Access Fault' exception was raised @ %p "
           "(badaddr = %p)\n", mepc, read_csr(mtval));
    break;
  default:
    printm("error: unknown exception was raised @ %p (cause = %zx)\n",
           mepc, mcause);
    break;
  }

  uintptr_t exit_code = 0x80000000 | mcause;
  return exit_code;
}
