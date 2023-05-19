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

#ifndef _RISCV_DEBUGGER_H
#define _RISCV_DEBUGGER_H

#include "riscv/devices.h"

#include <vector>
#include <string>
#include <map>

class slim_sim_t;
class debugger_t;
class processor_t;

typedef void (debugger_t::*debug_command_handler)();
using handler_map = std::map<std::string, debug_command_handler>;

struct debugger_t {
public:
  explicit debugger_t(slim_sim_t &sim);

  void read_command();
  bool run_command();

  void do_help();
  void do_quit();
  void do_pc();
  void do_run_noisy();
  void do_run_silent();
  void do_vreg();
  void do_reg();
  void do_freg();
  void do_fregh();
  void do_fregs();
  void do_fregd();
  void do_mem();
  void do_str();
  void do_until_silent();
  void do_until_noisy();

  void set_cmd(const std::string &s) { this->cmd = s; }
  void set_args(const std::vector<std::string> &v) { this->args = v; }

protected:
  void interactive_until(bool noisy);

  processor_t *get_core(const std::string& i);
  reg_t get_pc(const std::vector<std::string>& args);
  reg_t get_reg(const std::vector<std::string>& args);
  freg_t get_freg(const std::vector<std::string>& args);
  reg_t get_mem(const std::vector<std::string>& args);

  slim_sim_t &sim;
  handler_map handlers;
  std::string cmd;
  std::vector<std::string> args;
};

#endif
