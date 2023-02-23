// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "program.h"

#include <cassert>

#include "elf.h"

namespace hal {
/// @addtogroup hal
/// @{

namespace util {
/// @addtogroup util
/// @{

bool hal_program_impl_t::populate_kernel_map(const elf64_file_t &elf) {
  // clear any previously set symbols
  symbols.clear();
  // get the string table
  const char *strtab = elf.get_strtab();
  if (!strtab) {
    return false;
  }
  // get the symbol table
  Elf_Shdr_wrapper_t symtab;
  if (!elf.find_section(".symtab", symtab)) {
    return false;
  }
  // look at each symbol in turn
  const uint8_t *ptr = elf.data() + symtab.sh_offset;
  const uint8_t *end = ptr + symtab.sh_size;
  while (ptr < end) {
    const Elf64_Sym_t *sym_ptr = (const Elf64_Sym_t *)ptr;
    // check symbol name
    const char *sym_name = strtab + sym_ptr->st_name;
    // store functions
    if (sym_name && *sym_name) {
      if (ELF32_ST_TYPE(sym_ptr->st_info) == STT_FUNC) {
        symbols[sym_name] = sym_ptr->st_value;
      }
    }
    // move onto next symbol
    ptr += symtab.sh_entsize;
  }
  return true;
}

bool hal_program_impl_t::upload_section_copy(hal_device_t *dev,
                                             const uint8_t *data,
                                             Elf64_XWord_t to_copy,
                                             Elf_Phdr_wrapper_t phdr) {
  return dev->mem_write(phdr.p_vaddr, data + phdr.p_offset, to_copy);
}

bool hal_program_impl_t::upload_section_zero(hal_device_t *dev,
                                             Elf64_XWord_t offset,
                                             Elf64_XWord_t to_zero,
                                             Elf_Phdr_wrapper_t phdr) {
  const uint8_t zero = '\0';
  return dev->mem_fill(phdr.p_vaddr + offset, &zero, 1, to_zero);
}

// upload this program to the target
bool hal_program_impl_t::upload(hal_device_t *dev) {
  assert(data && size && dev);
  // parse the given ELF file
  elf64_file_t elf;
  if (!elf.load(data.get(), size)) {
    return false;
  }
  // get the ELF header
  Elf_Ehdr_wrapper_t ehdr;
  if (!elf.get_header(ehdr)) {
    return false;
  }
  // loop over all of the program headers
  for (uint32_t p = 0; p < ehdr.e_phnum; ++p) {
    // get this program header
    Elf_Phdr_wrapper_t phdr;
    if (!elf.get_phdr(p, phdr)) {
      return false;
    }
    // check this section should be loaded
    if (phdr.p_type != PT_LOAD) {
      continue;
    }
    // section has data to copy across
    const auto to_copy = std::min(phdr.p_memsz, phdr.p_filesz);
    if (to_copy) {
      if (!upload_section_copy(dev, elf.data(), to_copy, phdr)) {
        return false;
      }
    }
    // section has zero filled segments
    const auto to_zero = std::max(phdr.p_memsz, phdr.p_filesz) - to_copy;
    if (to_zero) {
      if (!upload_section_zero(dev, to_copy, to_zero, phdr)) {
        return false;
      }
    }
  }
  return true;
}

bool hal_program_impl_t::load(const uint8_t *elf_data, size_t elf_size) {
  data.reset(new uint8_t[elf_size]);
  memcpy(data.get(), elf_data, elf_size);
  size = elf_size;
  // parse the given ELF file
  elf64_file_t elf;
  if (!elf.load(data.get(), size)) {
    return false;
  }
  // get a list of all the kernels available
  if (!populate_kernel_map(elf)) {
    return false;
  }
  return true;
}

/// @}
}  // namespace util
/// @}
}  // namespace hal
