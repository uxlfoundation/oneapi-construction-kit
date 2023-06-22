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

#ifndef _HAL_REFSI_LOADER_MACHINE_H
#define _HAL_REFSI_LOADER_MACHINE_H

#include "encoding.h"

#ifndef __ASSEMBLER__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "io.h"

#define read_const_csr(reg)              \
  ({                                     \
    unsigned long __tmp;                 \
    asm("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp;                               \
  })

static inline int supports_extension(char ext) {
  return read_const_csr(misa) & (1 << (ext - 'A'));
}

static inline int xlen() { return read_const_csr(misa) < 0 ? 64 : 32; }

#define assert(x)                              \
  ({                                           \
    if (!(x)) die("assertion failed: %s", #x); \
  })
#define die(str, ...)                                              \
  ({                                                               \
    printm("%s:%d: " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    shutdown(-1);                                                  \
  })

void setup_pmp();
void execute_nd_range();

uintptr_t report_machine_trap(uintptr_t *regs, uintptr_t dummy, uintptr_t mepc);

static inline void wfi() { asm volatile("wfi" ::: "memory"); }

#endif  // !__ASSEMBLER__

#define INTEGER_CONTEXT_SIZE (32 * REGBYTES)

#endif
