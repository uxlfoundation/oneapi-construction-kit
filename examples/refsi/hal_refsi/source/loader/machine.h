// See LICENSE.spike for license details.
// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _HAL_REFSI_LOADER_MACHINE_H
#define _HAL_REFSI_LOADER_MACHINE_H

#include "encoding.h"

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "io.h"

#define read_const_csr(reg) ({ unsigned long __tmp; \
  asm ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

static inline int supports_extension(char ext)
{
  return read_const_csr(misa) & (1 << (ext - 'A'));
}

static inline int xlen()
{
  return read_const_csr(misa) < 0 ? 64 : 32;
}

#define assert(x) ({ if (!(x)) die("assertion failed: %s", #x); })
#define die(str, ...) ({ printm("%s:%d: " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); shutdown(-1); })

void setup_pmp();
void execute_nd_range();

uintptr_t report_machine_trap(uintptr_t* regs, uintptr_t dummy, uintptr_t mepc);

static inline void wfi()
{
  asm volatile ("wfi" ::: "memory");
}

#endif // !__ASSEMBLER__

#define INTEGER_CONTEXT_SIZE (32 * REGBYTES)

#endif
