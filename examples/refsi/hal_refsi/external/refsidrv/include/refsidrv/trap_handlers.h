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
