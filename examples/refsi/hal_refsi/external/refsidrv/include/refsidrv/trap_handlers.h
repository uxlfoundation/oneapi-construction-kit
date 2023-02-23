// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_TRAP_HANDLERS_H
#define _REFSIDRV_TRAP_HANDLERS_H

#include "trap.h"

class slim_sim_t;

class trap_handler_t {
public:
  virtual ~trap_handler_t();

  virtual bool handle_trap(trap_t &trap, reg_t pc, slim_sim_t &sim);
};

class default_trap_handler : public trap_handler_t {
  bool debug = true;

public:
  bool handle_trap(trap_t &trap, reg_t pc, slim_sim_t &sim) override;
  virtual bool handle_ecall(trap_t &trap, reg_t pc, slim_sim_t &sim);

  virtual void print_trap(trap_t &trap, reg_t pc);

  bool get_debug() const { return debug; }
  void set_debug(bool val) { debug = val; }
};

#endif
