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

#ifndef _REFSIDRV_COMMON_ELF_LOADER_H
#define _REFSIDRV_COMMON_ELF_LOADER_H

#include <map>
#include <vector>
#include <string>

#include "riscv/devices.h"
#include "common_devices.h"
#include "fesvr/elf.h"

using symbol_map = std::map<std::string, reg_t>;

enum class elf_machine {
  unknown = 0,
  riscv_rv32 = 1,
  riscv_rv64 = 2
};

struct elf_segment {
  uint64_t address;
  const uint8_t *data;
  uint64_t file_size;
  uint64_t memory_size;
};

class ELFProgram {
public:
  ~ELFProgram();

  elf_machine get_machine() const;

  bool read(MemoryDevice &src,
            unit_id_t unit = make_unit(unit_kind::external));
  bool load(MemoryDevice &dst,
            unit_id_t unit = make_unit(unit_kind::external));
  void clear();

  const std::vector<elf_segment> & get_segments() const { return segments; }
  const symbol_map & get_symbols() const { return symbols; }
  uint64_t get_entry_address() const { return entry_address; }

  reg_t find_symbol(const char *name) const;

  static constexpr uint64_t invalid_address = ~0ull;

private:
  bool read_header(MemoryDevice &src, unit_id_t unit);
  bool read_program_header(Elf64_Phdr &program_header, MemoryDevice &src,
                           unit_id_t unit, uint64_t offset);
  bool read_section_header(Elf64_Shdr &section_header, MemoryDevice &src,
                           unit_id_t unit, uint64_t offset);
  bool read_symbol(Elf64_Sym &sym, MemoryDevice &src, unit_id_t unit,
                   uint64_t offset);
  bool read_symbols(MemoryDevice &src, unit_id_t unit);
  bool read_c_string(MemoryDevice &src, unit_id_t unit, reg_t addr,
                     std::string &s);

  std::vector<elf_segment> segments;
  symbol_map symbols;
  Elf64_Ehdr header;
  uint64_t entry_address = invalid_address;
};

#endif
