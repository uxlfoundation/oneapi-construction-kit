// See LICENSE.spike for license details.
// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "debugger.h"
#include "slim_sim.h"
#include "riscv/disasm.h"
#include "riscv/mmu.h"
#include <termios.h>
#include <iostream>
#include <climits>
#include <cinttypes>
#include <unistd.h>
#include <cmath>

DECLARE_TRAP(-1, interactive)

debugger_t::debugger_t(slim_sim_t &sim) : sim(sim) {
  handlers["run"] = &debugger_t::do_run_noisy;
  handlers["r"] = handlers["run"];
  handlers["rs"] = &debugger_t::do_run_silent;
  handlers["vreg"] = &debugger_t::do_vreg;
  handlers["reg"] = &debugger_t::do_reg;
  handlers["freg"] = &debugger_t::do_freg;
  handlers["fregh"] = &debugger_t::do_fregh;
  handlers["fregs"] = &debugger_t::do_fregs;
  handlers["fregd"] = &debugger_t::do_fregd;
  handlers["pc"] = &debugger_t::do_pc;
  handlers["mem"] = &debugger_t::do_mem;
  handlers["str"] = &debugger_t::do_str;
  handlers["until"] = &debugger_t::do_until_silent;
  handlers["untiln"] = &debugger_t::do_until_noisy;
  handlers["while"] = &debugger_t::do_until_silent;
  handlers["quit"] = &debugger_t::do_quit;
  handlers["q"] = handlers["quit"];
  handlers["help"] = &debugger_t::do_help;
  handlers["h"] = handlers["help"];
}

static std::string readline(int fd) {
  struct termios tios;
  bool noncanonical = tcgetattr(fd, &tios) == 0 && (tios.c_lflag & ICANON) == 0;

  std::string s;
  for (char ch; read(fd, &ch, 1) == 1; ) {
    if (ch == '\x7f') {
      if (s.empty())
        continue;
      s.erase(s.end()-1);

      if (noncanonical && write(fd, "\b \b", 3) != 3) {
        ; // shut up gcc
      }
    } else if (noncanonical && write(fd, &ch, 1) != 1) {
      ; // shut up gcc
    }

    if (ch == '\n') {
      break;
    }
    if (ch != '\x7f') {
      s += ch;
    }
  }
  return s;
}

void debugger_t::read_command() {
  std::cerr << ": " << std::flush;
  std::string s = readline(2);

  std::stringstream ss(s);
  std::string tmp;

  cmd = "";
  args.clear();
  if (!(ss >> cmd)) {
    cmd = "run";
    args.push_back("1");
  }

  while (ss >> tmp) {
    args.push_back(tmp);
  }
}

bool debugger_t::run_command() {
  try {
    if (handlers.count(cmd)) {
      (this->*handlers[cmd])();
      return true;
    } else {
      fprintf(stderr, "Unknown command %s\n", cmd.c_str());
      return false;
    }
  }
  catch(trap_t& t) {
    return false;
  }
}

processor_t *debugger_t::get_core(const std::string& i) {
  char *ptr;
  unsigned long p = strtoul(i.c_str(), &ptr, 10);
  if (processor_t *core = sim.get_hart(p)) {
    return core;
  }
  throw trap_interactive();
}

reg_t debugger_t::get_pc(const std::vector<std::string>& args) {
  if (args.size() != 1) {
    throw trap_interactive();
  }

  processor_t *p = get_core(args[0]);
  return p->get_state()->pc;
}

reg_t debugger_t::get_reg(const std::vector<std::string>& args) {
  if (args.size() != 2) {
    throw trap_interactive();
  }

  processor_t *p = get_core(args[0]);

  unsigned long r = std::find(xpr_name, xpr_name + NXPR, args[1]) - xpr_name;
  if (r == NXPR) {
    char *ptr;
    r = strtoul(args[1].c_str(), &ptr, 10);
    if (*ptr) {
      #define DECLARE_CSR(name, number) if (args[1] == #name) return p->get_csr(number);
      #include "encoding.h"              // generates if's for all csrs
      r = NXPR;                          // else case (csr name not found)
      #undef DECLARE_CSR
    }
  }

  if (r >= NXPR) {
    throw trap_interactive();
  }

  return p->get_state()->XPR[r];
}

freg_t debugger_t::get_freg(const std::vector<std::string>& args) {
  if (args.size() != 2) {
    throw trap_interactive();
  }

  processor_t *p = get_core(args[0]);
  int r = std::find(fpr_name, fpr_name + NFPR, args[1]) - fpr_name;
  if (r == NFPR) {
    r = atoi(args[1].c_str());
  }
  if (r >= NFPR) {
    throw trap_interactive();
  }

  return p->get_state()->FPR[r];
}

reg_t debugger_t::get_mem(const std::vector<std::string>& args) {
  if (args.size() != 1) {
    throw trap_interactive();
  }

  std::string addr_str = args[0];
  reg_t addr = strtol(addr_str.c_str(), NULL, 16), val;
  if (addr == LONG_MAX) {
    addr = strtoul(addr_str.c_str(), NULL, 16);
  }

  // TODO Support different endianess
  val = 0;
  if (!sim.mmio_load(addr, sizeof(uint64_t), (uint8_t *)&val)) {
    throw trap_interactive();
  }

  return val;
}

void debugger_t::do_help() {
  std::cerr <<
    "Interactive commands:\n"
    "reg <core> [reg]                # Display [reg] (all if omitted) in <core>\n"
    "fregh <core> <reg>              # Display half precision <reg> in <core>\n"
    "fregs <core> <reg>              # Display single precision <reg> in <core>\n"
    "fregd <core> <reg>              # Display double precision <reg> in <core>\n"
    "vreg <core> [reg]               # Display vector [reg] (all if omitted) in <core>\n"
    "pc <core>                       # Show current PC in <core>\n"
    "mem <hex addr>                  # Show contents of physical memory\n"
    "str <hex addr>                  # Show NUL-terminated C string\n"
    "until reg <core> <reg> <val>    # Stop when <reg> in <core> hits <val>\n"
    "until pc <core> <val>           # Stop when PC in <core> hits <val>\n"
    "untiln pc <core> <val>          # Run noisy and stop when PC in <core> hits <val>\n"
    "until mem <addr> <val>          # Stop when memory <addr> becomes <val>\n"
    "while reg <core> <reg> <val>    # Run while <reg> in <core> is <val>\n"
    "while pc <core> <val>           # Run while PC in <core> is <val>\n"
    "while mem <addr> <val>          # Run while memory <addr> is <val>\n"
    "run [count]                     # Resume noisy execution (until CTRL+C, or [count] insns)\n"
    "r [count]                         Alias for run\n"
    "rs [count]                      # Resume silent execution (until CTRL+C, or [count] insns)\n"
    "quit                            # End the simulation\n"
    "q                                 Alias for quit\n"
    "help                            # This screen!\n"
    "h                                 Alias for help\n"
    "Note: Hitting enter is the same as: run 1\n"
    << std::flush;
}

void debugger_t::do_run_noisy() {
  size_t steps = args.size() ? atoll(args[0].c_str()) : -1;
  sim.run_single_step(true, steps);
}

void debugger_t::do_run_silent() {
  size_t steps = args.size() ? atoll(args[0].c_str()) : -1;
  sim.run_single_step(false, steps);
}

void debugger_t::do_quit() {
  exit(0);
}

void debugger_t::do_pc() {
  fprintf(stderr, "0x%016" PRIx64 "\n", get_pc(args));
}

void debugger_t::do_vreg() {
  int rstart = 0;
  int rend = NVPR;

  if (args.empty()) {
    throw trap_interactive();
  }

  if (args.size() >= 2) {
    rstart = strtol(args[1].c_str(), NULL, 0);
    if (!(rstart >= 0 && rstart < NVPR)) {
      rstart = 0;
    } else {
      rend = rstart + 1;
    }
  }

  // Show all the regs!
  processor_t *p = get_core(args[0]);
  const int vlen = (int)(p->VU.get_vlen()) >> 3;
  const int elen = (int)(p->VU.get_elen()) >> 3;
  const int num_elem = vlen/elen;
  fprintf(stderr, "VLEN=%d bits; ELEN=%d bits\n", vlen << 3, elen << 3);

  for (int r = rstart; r < rend; ++r) {
    fprintf(stderr, "%-4s: ", vr_name[r]);
    for (int e = num_elem-1; e >= 0; --e) {
      uint64_t val;
      switch(elen){
        case 8:
          val = p->VU.elt<uint64_t>(r, e);
          fprintf(stderr, "[%d]: 0x%016" PRIx64 "  ", e, val);
          break;
        case 4:
          val = p->VU.elt<uint32_t>(r, e);
          fprintf(stderr, "[%d]: 0x%08" PRIx32 "  ", e, (uint32_t)val);
          break;
        case 2:
          val = p->VU.elt<uint16_t>(r, e);
          fprintf(stderr, "[%d]: 0x%08" PRIx16 "  ", e, (uint16_t)val);
          break;
        case 1:
          val = p->VU.elt<uint8_t>(r, e);
          fprintf(stderr, "[%d]: 0x%08" PRIx8 "  ", e, (uint8_t)val);
          break;
      }
    }
    fprintf(stderr, "\n");
  }
}

void debugger_t::do_reg() {
  if (args.size() == 1) {
    // Show all the regs!
    processor_t *p = get_core(args[0]);

    for (int r = 0; r < NXPR; ++r) {
      fprintf(stderr, "%-4s: 0x%016" PRIx64 "  ", xpr_name[r], p->get_state()->XPR[r]);
      if ((r + 1) % 4 == 0) {
        fprintf(stderr, "\n");
      }
    }
  } else {
    fprintf(stderr, "0x%016" PRIx64 "\n", get_reg(args));
  }
}

union fpr
{
  freg_t r;
  float s;
  double d;
};

void debugger_t::do_freg() {
  freg_t r = get_freg(args);
  fprintf(stderr, "0x%016" PRIx64 "%016" PRIx64 "\n", r.v[1], r.v[0]);
}

void debugger_t::do_fregh() {
  fpr f;
  f.r = freg(f16_to_f32(f16(get_freg(args))));
  fprintf(stderr, "%g\n", isBoxedF32(f.r) ? (double)f.s : NAN);
}

void debugger_t::do_fregs() {
  fpr f;
  f.r = get_freg(args);
  fprintf(stderr, "%g\n", isBoxedF32(f.r) ? (double)f.s : NAN);
}

void debugger_t::do_fregd() {
  fpr f;
  f.r = get_freg(args);
  fprintf(stderr, "%g\n", isBoxedF64(f.r) ? f.d : NAN);
}

void debugger_t::do_mem() {
  fprintf(stderr, "0x%016" PRIx64 "\n", get_mem(args));
}

void debugger_t::do_str() {
  if (args.size() != 1) {
    throw trap_interactive();
  }

  reg_t addr = strtol(args[0].c_str(), NULL, 16);

  sim.mmio_print(addr);
  putchar('\n');
}

void debugger_t::do_until_silent() {
  interactive_until(false);
}

void debugger_t::do_until_noisy() {
  interactive_until(true);
}

void debugger_t::interactive_until(bool noisy) {
  bool cmd_until = cmd == "until" || cmd == "untiln";

  if (args.size() < 3) {
    return;
  }

  reg_t val = strtol(args[args.size() - 1].c_str(), NULL, 16);
  if (val == LONG_MAX) {
    val = strtoul(args[args.size() - 1].c_str(), NULL, 16);
  }

  std::vector<std::string> args2;
  args2 = std::vector<std::string>(args.begin()+1,args.end()-1);

  auto func = args[0] == "reg" ? &debugger_t::get_reg :
              args[0] == "pc"  ? &debugger_t::get_pc :
              args[0] == "mem" ? &debugger_t::get_mem :
              NULL;

  if (func == NULL) {
    return;
  }

  while (sim.is_running()) {
    try {
      reg_t current = (this->*func)(args2);

      if (cmd_until == (current == val))
        break;
    }
    catch (trap_t& t) {}

    sim.run_single_step(noisy, 1);
  }
}
