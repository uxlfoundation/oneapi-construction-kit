// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "trap_handlers.h"
#include "slim_sim.h"
#include "device/host_io_regs.h"

trap_handler_t::~trap_handler_t() {}

bool trap_handler_t::handle_trap(trap_t &trap, reg_t pc, slim_sim_t &sim) {
  (void)trap;
  (void)pc;
  (void)sim;
  return false;
}

bool default_trap_handler::handle_trap(trap_t &trap, reg_t pc,
                                       slim_sim_t &sim) {
  if (trap.cause() == CAUSE_MACHINE_ECALL) {
    if (handle_ecall(trap, pc, sim)) {
      return true;
    }
  }

  // Traps cause the simulation to be aborted by setting a non-zero exit code.
  if (debug) {
    print_trap(trap, pc);
  }
  sim.set_exited(0x80000000 | trap.cause());
  return false;
}

bool default_trap_handler::handle_ecall(trap_t &trap, reg_t pc,
                                        slim_sim_t &sim) {
  processor_t *hart = sim.get_hart(sim.get_current_hart_id());
  if (!hart) {
    return false;
  }
  reg_t opc = hart->get_state()->XPR[17]; // a7 - opcode
  reg_t val = hart->get_state()->XPR[10]; // a0 - argument
  switch (opc) {
  default:
    return false;
  case HOST_IO_CMD_EXIT:
    sim.set_exited(val);
    return true;
  case HOST_IO_CMD_PUTSTRING:
    return sim.mmio_print(val);
  case HOST_IO_CMD_BARRIER:
    return sim.handle_barrier(val);
  }
}

void default_trap_handler::print_trap(trap_t &trap, reg_t pc) {
  switch (trap.cause()) {
  case CAUSE_FETCH_ACCESS:
    fprintf(stderr,
            "error: 'Instruction Access Fault' exception was raised"
            " @ 0x%zx\n",
            pc);
    break;
  case CAUSE_ILLEGAL_INSTRUCTION:
    fprintf(stderr,
            "error: 'Illegal Instruction' exception was raised @"
            " 0x%zx\n",
            pc);
    break;
  case CAUSE_LOAD_ACCESS:
    fprintf(stderr,
            "error: 'Load Access Fault' exception was raised @ 0x%zx"
            " (badaddr = 0x%zx)\n",
            pc, trap.get_tval());
    break;
  case CAUSE_STORE_ACCESS:
    fprintf(stderr,
            "error: 'Store/AMO Access Fault' exception was raised @"
            " 0x%zx (badaddr = 0x%zx)\n",
            pc, trap.get_tval());
    break;
  case CAUSE_MISALIGNED_LOAD:
    fprintf(stderr, "error: 'Misaligned Load' exception was raised @ 0x%zx"
            " (badaddr = 0x%zx)\n", pc, trap.get_tval());
    break;
  case CAUSE_MISALIGNED_STORE:
    fprintf(stderr, "error: 'Misaligned Store' exception was raised @"
            " 0x%zx (badaddr = 0x%zx)\n", pc, trap.get_tval());
    break;
  default:
    fprintf(stderr,
            "error: unknown exception was raised @ 0x%zx "
            "(cause = %zx)\n",
            pc, trap.cause());
    break;
  }
}
