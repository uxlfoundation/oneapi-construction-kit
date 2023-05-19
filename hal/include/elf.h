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

/// @file
///
/// @brief Device Hardware Abstraction Layer ELF file loader.

#ifndef HAL_ELF_H_INCLUDED
#define HAL_ELF_H_INCLUDED

#include <stdint.h>

#include <cstring>
#include <type_traits>

namespace hal {

// Primitive elf types
typedef uint32_t Elf32_Addr_t;
typedef uint32_t Elf32_Off_t;
typedef uint16_t Elf32_Half_t;
typedef uint32_t Elf32_Word_t;
typedef uint32_t Elf32_XWord_t;

typedef uint64_t Elf64_Addr_t;
typedef uint64_t Elf64_Off_t;
typedef uint16_t Elf64_Half_t;
typedef uint32_t Elf64_Word_t;
typedef uint64_t Elf64_XWord_t;

static inline constexpr uint8_t ELF32_ST_TYPE(uint8_t i) { return i & 0xf; }

enum {
  // ELF identification
  EI_MAG0 = 0,
  EI_MAG1 = 1,
  EI_MAG2 = 2,
  EI_MAG3 = 3,
  EI_CLASS = 4,
  EI_DATA = 5,
  EI_VERSION = 6,
  EI_OSABI = 7,
  EI_ABIVERSION = 8,
  EI_PAD = 9,
  EI_NIDENT = 16,

  // ELF architecture
  EM_RISCV = 243,

  // ELF machine size
  ELFCLASS32 = 1,
  ELFCLASS64 = 2,

  // Symbol type
  STT_FUNC = 2,

  // segment flag bits
  PF_X = 1,
  PF_W = 2,
  PF_R = 4,

  // segment types
  PT_NULL = 0,
  PT_LOAD = 1,
};

template <bool Is64>
struct ElfTypes {
  using Elf_Half_t =
      typename std::conditional<Is64, Elf64_Half_t, Elf32_Half_t>::type;
  using Elf_Word_t =
      typename std::conditional<Is64, Elf64_Word_t, Elf32_Word_t>::type;
  using Elf_XWord_t =
      typename std::conditional<Is64, Elf64_XWord_t, Elf32_XWord_t>::type;
  using Elf_Addr_t =
      typename std::conditional<Is64, Elf64_Addr_t, Elf32_Addr_t>::type;
  using Elf_Off_t =
      typename std::conditional<Is64, Elf64_Off_t, Elf32_Off_t>::type;
};

template <bool Is64>
struct Elf_Ehdr_t {
  using Elf_Off_t = typename ElfTypes<Is64>::Elf_Off_t;
  using Elf_Half_t = typename ElfTypes<Is64>::Elf_Half_t;
  using Elf_Word_t = typename ElfTypes<Is64>::Elf_Word_t;
  using Elf_XWord_t = typename ElfTypes<Is64>::Elf_XWord_t;
  using Elf_Addr_t = typename ElfTypes<Is64>::Elf_Addr_t;

  uint8_t e_ident[EI_NIDENT];
  Elf_Half_t e_type;
  Elf_Half_t e_machine;
  Elf_Word_t e_version;
  Elf_Addr_t e_entry;
  Elf_Off_t e_phoff;
  Elf_Off_t e_shoff;
  Elf_Word_t e_flags;
  Elf_Half_t e_ehsize;
  Elf_Half_t e_phentsize;
  Elf_Half_t e_phnum;
  Elf_Half_t e_shentsize;
  Elf_Half_t e_shnum;
  Elf_Half_t e_shstrndx;

  Elf_Ehdr_t() = default;
  Elf_Ehdr_t(const Elf_Ehdr_t<false> &other)
      : e_type(other.e_type),
        e_machine(other.e_machine),
        e_version(other.e_version),
        e_entry(other.e_entry),
        e_phoff(other.e_phoff),
        e_shoff(other.e_shoff),
        e_flags(other.e_flags),
        e_ehsize(other.e_ehsize),
        e_phentsize(other.e_phentsize),
        e_phnum(other.e_phnum),
        e_shentsize(other.e_shentsize),
        e_shnum(other.e_shnum),
        e_shstrndx(other.e_shstrndx) {
    memcpy(&e_ident, &other.e_ident, sizeof(e_ident));
  }
};

using Elf_Ehdr_wrapper_t = Elf_Ehdr_t<true>;

// program header
struct Elf32_Phdr_t {
  Elf32_Word_t p_type;
  Elf32_Off_t p_offset;
  Elf32_Addr_t p_vaddr;
  Elf32_Addr_t p_paddr;
  Elf32_XWord_t p_filesz;
  Elf32_XWord_t p_memsz;
  Elf32_Word_t p_flags;
  Elf32_XWord_t p_align;
};

struct Elf64_Phdr_t {
  Elf64_Word_t p_type;
  Elf64_Word_t p_flags;
  Elf64_Off_t p_offset;
  Elf64_Addr_t p_vaddr;
  Elf64_Addr_t p_paddr;
  Elf64_XWord_t p_filesz;
  Elf64_XWord_t p_memsz;
  Elf64_XWord_t p_align;

  Elf64_Phdr_t() = default;
  Elf64_Phdr_t(const Elf32_Phdr_t &other)
      : p_type(other.p_type),
        p_flags(other.p_flags),
        p_offset(other.p_offset),
        p_vaddr(other.p_vaddr),
        p_paddr(other.p_paddr),
        p_filesz(other.p_filesz),
        p_memsz(other.p_memsz),
        p_align(other.p_align) {}
};

using Elf_Phdr_wrapper_t = Elf64_Phdr_t;

// section header
template <bool Is64>
struct Elf_Shdr_t {
  using Elf_Off_t = typename ElfTypes<Is64>::Elf_Off_t;
  using Elf_Half_t = typename ElfTypes<Is64>::Elf_Half_t;
  using Elf_Word_t = typename ElfTypes<Is64>::Elf_Word_t;
  using Elf_XWord_t = typename ElfTypes<Is64>::Elf_XWord_t;
  using Elf_Addr_t = typename ElfTypes<Is64>::Elf_Addr_t;
  Elf_Word_t sh_name;
  Elf_Word_t sh_type;
  Elf_XWord_t sh_flags;
  Elf_Addr_t sh_addr;
  Elf_Off_t sh_offset;
  Elf_XWord_t sh_size;
  Elf_Word_t sh_link;
  Elf_Word_t sh_info;
  Elf_XWord_t sh_addralign;
  Elf_XWord_t sh_entsize;

  Elf_Shdr_t() = default;
  Elf_Shdr_t(const Elf_Shdr_t<false> &other)
      : sh_name(other.sh_name),
        sh_type(other.sh_type),
        sh_flags(other.sh_flags),
        sh_addr(other.sh_addr),
        sh_offset(other.sh_offset),
        sh_size(other.sh_size),
        sh_link(other.sh_link),
        sh_info(other.sh_info),
        sh_addralign(other.sh_addralign),
        sh_entsize(other.sh_entsize) {}
};

using Elf_Shdr_wrapper_t = Elf_Shdr_t<true>;

// symbol def
struct Elf32_Sym_t {
  Elf32_Word_t st_name;
  Elf32_Addr_t st_value;
  Elf32_XWord_t st_size;
  uint8_t st_info;
  uint8_t st_other;
  Elf32_Half_t st_shndx;
};

struct Elf64_Sym_t {
  Elf64_Word_t st_name;
  uint8_t st_info;
  uint8_t st_other;
  Elf64_Half_t st_shndx;
  Elf64_Addr_t st_value;
  Elf64_XWord_t st_size;

  Elf64_Sym_t() = default;
  Elf64_Sym_t(const Elf32_Sym_t &other)
      : st_name(other.st_name),
        st_info(other.st_info),
        st_other(other.st_other),
        st_shndx(other.st_shndx),
        st_value(other.st_value),
        st_size(other.st_size) {}
};

using Elf_Sym_wrapper_t = Elf64_Sym_t;

struct elf_base_t {
  elf_base_t() : elf(nullptr) {}
  virtual bool load(const uint8_t *elf_ptr, std::size_t size) = 0;

  const uint8_t *data() const { return elf; }

  virtual bool get_header(Elf_Ehdr_wrapper_t &) const = 0;
  virtual bool get_phdr(int index, Elf_Phdr_wrapper_t &) const = 0;
  virtual bool get_shdr(int index, Elf_Shdr_wrapper_t &) const = 0;
  virtual bool find_symbol(const char *name, Elf_Sym_wrapper_t &) const = 0;

 protected:
  const uint8_t *elf;
};

template <bool Is64>
struct elf_file_t : elf_base_t {
  elf_file_t() : hdr(nullptr) {}

  using Elf_Phdr_t =
      typename std::conditional<Is64, Elf64_Phdr_t, Elf32_Phdr_t>::type;
  using Elf_Sym_t =
      typename std::conditional<Is64, Elf64_Sym_t, Elf32_Sym_t>::type;

  virtual bool get_header(Elf_Ehdr_wrapper_t &ehdr) const override {
    if (!hdr) {
      return false;
    }
    ehdr = *hdr;
    return true;
  }

  bool load(const uint8_t *elf_ptr, std::size_t size) override {
    (void)size;
    elf = elf_ptr;

    // parse the elf header
    hdr = (const Elf_Ehdr_t<Is64> *)data();

    bool valid = true;
    // check magic header
    valid = valid && hdr->e_ident[EI_MAG0] == 0x7f;
    valid = valid && hdr->e_ident[EI_MAG1] == 'E';
    valid = valid && hdr->e_ident[EI_MAG2] == 'L';
    valid = valid && hdr->e_ident[EI_MAG3] == 'F';
    // check target machine
    valid = valid && hdr->e_machine == EM_RISCV;
    // check elf class
    valid = valid && hdr->e_ident[EI_CLASS] == (Is64 ? ELFCLASS64 : ELFCLASS32);
    // check if any of these tests failed
    if (!valid) {
      unload();
      return false;
    }
    // success
    return true;
  }

  const char *get_shstrtab() const {
    Elf_Shdr_wrapper_t shstrtab;
    if (!get_shdr(hdr->e_shstrndx, shstrtab)) {
      return nullptr;
    }
    return (const char *)(data() + shstrtab.sh_offset);
  }

  bool find_section(const char *name, Elf_Shdr_wrapper_t &shdr) const {
    // get section header string table
    const char *shstrtab = get_shstrtab();
    if (!shstrtab) {
      return false;
    }
    // loop over all sections
    for (int i = 0; i < hdr->e_shnum; ++i) {
      // get section header
      if (!get_shdr(i, shdr)) {
        continue;
      }
      // get section name
      const char *shname = shstrtab + shdr.sh_name;
      if (strcmp(name, shname) == 0) {
        return true;
      }
    }
    return false;
  }

  const char *get_strtab() const {
    Elf_Shdr_wrapper_t strtab;
    if (!find_section(".strtab", strtab)) {
      return nullptr;
    }
    return (const char *)(data() + strtab.sh_offset);
  }

  bool find_symbol(const char *name, Elf_Sym_wrapper_t &sym) const override {
    const char *strtab = get_strtab();
    if (!strtab) {
      return false;
    }
    Elf_Shdr_wrapper_t symtab;
    if (!find_section(".symtab", symtab)) {
      return false;
    }
    // look at each symbol in turn
    const uint8_t *ptr = data() + symtab.sh_offset;
    const uint8_t *end = ptr + symtab.sh_size;
    while (ptr < end) {
      const Elf_Sym_t *sym_ptr = (const Elf_Sym_t *)ptr;
      // check symbol name
      const char *sym_name = strtab + sym_ptr->st_name;
      if (strcmp(name, sym_name) == 0) {
        sym = Elf_Sym_wrapper_t(*sym_ptr);
        return true;
      }
      // move onto next symbol
      ptr += symtab.sh_entsize;
    }
    // no match
    return false;
  }

  void unload() {
    elf = nullptr;
    hdr = nullptr;
  }

  virtual bool get_phdr(int index, Elf_Phdr_wrapper_t &phdr) const override {
    const auto offset = hdr->e_phoff + index * hdr->e_phentsize;
    phdr = Elf_Phdr_wrapper_t(*(const Elf_Phdr_t *)(data() + offset));
    return true;
  }

  virtual bool get_shdr(int index, Elf_Shdr_wrapper_t &shdr) const override {
    const auto offset = hdr->e_shoff + index * hdr->e_shentsize;
    shdr = Elf_Shdr_wrapper_t(*(const Elf_Shdr_t<Is64> *)(data() + offset));
    return true;
  }

 protected:
  const Elf_Ehdr_t<Is64> *hdr;
};

using elf64_file_t = elf_file_t<true>;
using elf32_file_t = elf_file_t<false>;

/// @}
}  // namespace hal

#endif  // HAL_ELF_H_INCLUDED
