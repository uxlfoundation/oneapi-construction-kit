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

#include "elf_loader.h"

#include "common_devices.h"
#include "fesvr/byteorder.h"

#define SHT_SYMTAB 2
#define SHT_STRTAB 3

#define STB_GLOBAL 1

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

ELFProgram::~ELFProgram() { clear(); }

void ELFProgram::clear() {
  for (elf_segment segment : segments) {
    delete[] segment.data;
  }
  segments.clear();
  symbols.clear();
  memset(&header, 0, sizeof(Elf64_Ehdr));
  entry_address = 0;
}

elf_machine ELFProgram::get_machine() const {
  if (!IS_ELF(header) || !IS_ELF_RISCV(header)) {
    return elf_machine::unknown;
  } else if (IS_ELF32(header)) {
    return elf_machine::riscv_rv32;
  } else if (IS_ELF64(header)) {
    return elf_machine::riscv_rv64;
  } else {
    return elf_machine::unknown;
  }
}

bool ELFProgram::read(MemoryDevice &src, unit_id_t unit) {
  clear();

  // Read the ELF header.
  if (!read_header(src, unit)) {
    return false;
  }
  entry_address = header.e_entry;

  // Read the segment table.
  std::vector<Elf64_Phdr> program_headers(header.e_phnum);
  uint64_t offset = header.e_phoff;
  for (Elf64_Phdr &program_header : program_headers) {
    if (!read_program_header(program_header, src, unit, offset)) {
      return false;
    }
    offset += header.e_phentsize;
  }

  // Load segments.
  for (Elf64_Phdr &header : program_headers) {
    if (header.p_type == PT_LOAD && header.p_memsz) {
      elf_segment segment;
      segment.address = header.p_vaddr;
      segment.file_size = header.p_filesz;
      segment.memory_size = header.p_memsz;
      segment.data = nullptr;

      // Load segment data from the ELF file.
      if (header.p_filesz > 0) {
        uint8_t *segment_data = new uint8_t[header.p_filesz];
        if (!src.load(header.p_offset, header.p_filesz, segment_data, unit)) {
          delete[] segment_data;
          return false;
        }
        segment.data = segment_data;
      }

      segments.push_back(segment);
    }
  }

  // Load the symbol table.
  return read_symbols(src, unit);
}

bool ELFProgram::load(MemoryDevice &dst, unit_id_t unit) {
  if (entry_address == invalid_address) {
    return false;
  }
  for (elf_segment &segment : segments) {
    uint64_t address = segment.address;

    // Write initialized data (i.e. read from the ELF) for this segment.
    if (segment.file_size &&
        !dst.store(address, segment.file_size, segment.data, unit)) {
      return false;
    }
    address += segment.file_size;

    // Write uninitialized data (i.e. zeros) for this segment.
    if (segment.memory_size > segment.file_size) {
      uint64_t bss_size = segment.memory_size - segment.file_size;
      const uint64_t chunk_size = 256;
      uint8_t chunk[chunk_size];
      memset(chunk, 0, chunk_size);
      while (bss_size > 0) {
        uint64_t to_write = std::min(chunk_size, bss_size);
        if (!dst.store(address, to_write, chunk, unit)) {
          return false;
        }
        address += to_write;
        bss_size -= to_write;
      }
    }
  }
  return true;
}

bool ELFProgram::read_header(MemoryDevice &src, unit_id_t unit) {
  // First try to read a 32-bit ELF header. The first four fields (e_ident,
  // e_type, e_machine, e_version) have identical layout between 32-bit and
  // 64-bit ELF files. We can use them to validate that what we have read is a
  // valid ELF header and whether it is 32 or 64 bits.
  Elf32_Ehdr header32;
  if (!src.load(0, sizeof(Elf32_Ehdr), (uint8_t *)&header32, unit) ||
      !IS_ELF(header32) || !IS_ELF_RISCV(header32)) {
    return false;
  } else if (IS_ELF32(header32)) {
    // Copy fields from the 32-bit header to a 64-bit struct.
    memset(&header, 0, sizeof(Elf64_Ehdr));
    memcpy(header.e_ident, header32.e_ident, sizeof(header32.e_ident));
    header.e_type = header32.e_type;
    header.e_machine = header32.e_machine;
    header.e_version = header32.e_version;
    header.e_entry = header32.e_entry;
    header.e_phoff = header32.e_phoff;
    header.e_shoff = header32.e_shoff;
    header.e_flags = header32.e_flags;
    header.e_ehsize = header32.e_ehsize;
    header.e_phentsize = header32.e_phentsize;
    header.e_phnum = header32.e_phnum;
    header.e_shentsize = header32.e_shentsize;
    header.e_shnum = header32.e_shnum;
    header.e_shstrndx = header32.e_shstrndx;
    return true;
  } else if (IS_ELF64(header32)) {
    // Reload the header as 64-bit.
    return src.load(0, sizeof(Elf64_Ehdr), (uint8_t *)&header, unit);
  } else {
    return false;
  }
}

bool ELFProgram::read_program_header(Elf64_Phdr &program_header,
                                     MemoryDevice &src, unit_id_t unit,
                                     uint64_t offset) {
  if (get_machine() == elf_machine::riscv_rv64) {
    return src.load(offset, sizeof(Elf64_Phdr), (uint8_t *)&program_header,
                    unit);
  }
  Elf32_Phdr program_header32;
  if (!src.load(offset, sizeof(Elf32_Phdr), (uint8_t *)&program_header32,
                unit)) {
    return false;
  }
  program_header.p_type = program_header32.p_type;
  program_header.p_offset = program_header32.p_offset;
  program_header.p_vaddr = program_header32.p_vaddr;
  program_header.p_paddr = program_header32.p_paddr;
  program_header.p_filesz = program_header32.p_filesz;
  program_header.p_memsz = program_header32.p_memsz;
  program_header.p_flags = program_header32.p_flags;
  program_header.p_align = program_header32.p_align;
  return true;
}

bool ELFProgram::read_section_header(Elf64_Shdr &section_header,
                                     MemoryDevice &src, unit_id_t unit,
                                     uint64_t offset) {
  if (get_machine() == elf_machine::riscv_rv64) {
    return src.load(offset, sizeof(Elf64_Shdr), (uint8_t *)&section_header,
                    unit);
  }
  Elf32_Shdr section_header32;
  if (!src.load(offset, sizeof(Elf32_Shdr), (uint8_t *)&section_header32,
                unit)) {
    return false;
  }
  section_header.sh_name = section_header32.sh_name;
  section_header.sh_type = section_header32.sh_type;
  section_header.sh_flags = section_header32.sh_flags;
  section_header.sh_addr = section_header32.sh_addr;
  section_header.sh_offset = section_header32.sh_offset;
  section_header.sh_size = section_header32.sh_size;
  section_header.sh_link = section_header32.sh_link;
  section_header.sh_info = section_header32.sh_info;
  section_header.sh_addralign = section_header32.sh_addralign;
  section_header.sh_entsize = section_header32.sh_entsize;
  return true;
}

bool ELFProgram::read_symbol(Elf64_Sym &sym, MemoryDevice &src, unit_id_t unit,
                             uint64_t offset) {
  if (get_machine() == elf_machine::riscv_rv64) {
    return src.load(offset, sizeof(Elf64_Sym), (uint8_t *)&sym, unit);
  }
  Elf32_Sym sym32;
  if (!src.load(offset, sizeof(Elf32_Sym), (uint8_t *)&sym32, unit)) {
    return false;
  }
  sym.st_name = sym32.st_name;
  sym.st_value = sym32.st_value;
  sym.st_size = sym32.st_size;
  sym.st_info = sym32.st_info;
  sym.st_other = sym32.st_other;
  sym.st_shndx = sym32.st_shndx;
  return true;
}

// Read an ELF's symbol table.
bool ELFProgram::read_symbols(MemoryDevice &src, unit_id_t unit) {
  if (header.e_shnum < header.e_shstrndx) {
    return false;
  }

  // Read the section table.
  std::vector<Elf64_Shdr> sections(header.e_shnum);
  uint64_t section_offset = header.e_shoff;
  for (Elf64_Shdr &section : sections) {
    if (!read_section_header(section, src, unit, section_offset)) {
      return false;
    }
    section_offset += header.e_shentsize;
  }

  // Identify sections.
  const Elf64_Shdr *symtab = nullptr;
  const Elf64_Shdr *strtab = nullptr;
  reg_t shstrtab_offset = sections[header.e_shstrndx].sh_offset;
  for (const Elf64_Shdr &section : sections) {
    std::string name;
    if (!read_c_string(src, unit, shstrtab_offset + section.sh_name, name)) {
      continue;
    }
    switch (section.sh_type) {
      default:
        break;
      case SHT_SYMTAB:
        if (name == ".symtab") {
          symtab = &section;
        }
        break;
      case SHT_STRTAB:
        if (name == ".strtab") {
          strtab = &section;
        }
        break;
    }
  }

  // Read the symbol table.
  size_t symbol_size = IS_ELF64(header) ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym);
  if (!symtab || !strtab || (symtab->sh_entsize < symbol_size)) {
    return false;
  }
  uint64_t symbol_offset = symtab->sh_offset;
  reg_t end_offset = symbol_offset + symtab->sh_size;
  while (symbol_offset < end_offset) {
    Elf64_Sym sym;
    if (!read_symbol(sym, src, unit, symbol_offset)) {
      return false;
    }
    if (ELF32_ST_BIND(sym.st_info) == STB_GLOBAL) {
      std::string name;
      if (read_c_string(src, unit, strtab->sh_offset + sym.st_name, name)) {
        symbols[name] = sym.st_value;
      }
    }
    symbol_offset += symtab->sh_entsize;
  }
  return true;
}

bool ELFProgram::read_c_string(MemoryDevice &src, unit_id_t unit, reg_t addr,
                               std::string &s) {
  const unsigned chunk_size = 8;
  uint8_t chunk[chunk_size];
  reg_t current_addr = addr;
  bool found_terminator = false;
  bool ok = src.load(current_addr, chunk_size, chunk, unit);
  s.clear();
  while (ok && !found_terminator) {
    size_t num_chars = 0;
    for (size_t i = 0; i < chunk_size; i++) {
      if (chunk[i] == 0) {
        found_terminator = true;
        break;
      }
      num_chars++;
    }
    s.insert(s.end(), chunk, chunk + num_chars);
    current_addr += chunk_size;
    if (found_terminator) {
      break;
    }
    ok = src.load(current_addr, chunk_size, chunk, unit);
  }
  return ok;
}

reg_t ELFProgram::find_symbol(const char *name) const {
  if (!name) {
    return 0;
  }
  auto it = symbols.find(std::string(name));
  return (it == symbols.end()) ? 0 : it->second;
}
