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
#include <loader/elf.h>

#include <algorithm>

using loader::ElfFile;

const std::array<uint8_t, 4> ElfFile::HeaderIdent::ELF_MAGIC = {
    {0x7F, 'E', 'L', 'F'}};

const cargo::string_view ElfFile::Section::name() const {
  CARGO_ASSERT(file != nullptr, "Using a null ElfFile instance");
  const Section sh = file->section(
      file->field(file->is32Bit() ? file->header32()->sht_names_index
                                  : file->header64()->sht_names_index));
  const size_t name_offset = file->is32Bit()
                                 ? file->field(this->header32()->name_offset)
                                 : file->field(this->header64()->name_offset);
  return reinterpret_cast<const char *>(file->bytes.begin() + sh.file_offset() +
                                        name_offset);
}

cargo::optional<cargo::string_view> ElfFile::Symbol::name() const {
  CARGO_ASSERT(file != nullptr, "Using a null ElfFile instance");
  auto sh = file->section(ElfFields::SYMBOL_NAMES_SECTION);
  if (!sh) {
    return cargo::nullopt;
  }
  const size_t name_offset = file->is32Bit()
                                 ? file->field(this->symbol32()->name_offset)
                                 : file->field(this->symbol64()->name_offset);
  if (name_offset == 0) {
    return cargo::nullopt;
  }
  return cargo::string_view(reinterpret_cast<const char *>(
      file->bytes.begin() + sh->file_offset() + name_offset));
}

ElfFile::ElfFile() : bytes() {}

ElfFile::ElfFile(cargo::array_view<uint8_t> aligned_data) {
  const bool is_aligned =
      (reinterpret_cast<size_t>(aligned_data.data()) & 0x7) == 0;
  if (is_aligned) {
    bytes = aligned_data;
    symbol_section = section(ElfFields::SYMBOL_TABLE_SECTION);
  } else {
    bytes = {};
  }
}

bool ElfFile::isValidElf(cargo::array_view<uint8_t> aligned_data) {
  const bool is_aligned =
      (reinterpret_cast<size_t>(aligned_data.data()) & 0x7) == 0;
  if (!is_aligned) {
    return false;
  }
  if (aligned_data.size() < sizeof(Header32)) {
    return false;
  }
  if (!std::equal(HeaderIdent::ELF_MAGIC.begin(), HeaderIdent::ELF_MAGIC.end(),
                  aligned_data.begin())) {
    return false;
  }
  return true;
}

loader::SectionIterator ElfFile::sectionsBegin() {
  return SectionIterator(this, 0);
}

loader::SectionIterator ElfFile::sectionsEnd() {
  return SectionIterator(this, sectionCount());
}

loader::SectionIteratorPair ElfFile::sections() {
  return {sectionsBegin(), sectionsEnd()};
}

ElfFile::Section ElfFile::section(size_t index) {
  CARGO_ASSERT(!bytes.empty(), "Using a null ElfFile instance");
  void *start;
  if (is32Bit()) {
    start = reinterpret_cast<void *>(bytes.begin() +
                                     field(header32()->section_header_offset) +
                                     index * field(header32()->sht_entry_size));
  } else {
    start = reinterpret_cast<void *>(bytes.begin() +
                                     field(header64()->section_header_offset) +
                                     index * field(header64()->sht_entry_size));
  }
  return Section(this, start);
}

cargo::optional<ElfFile::Section> ElfFile::section(cargo::string_view name) {
  auto result =
      std::find_if(sectionsBegin(), sectionsEnd(),
                   [name](const Section &v) { return name == v.name(); });
  return (result == sectionsEnd()) ? cargo::nullopt
                                   : cargo::optional<ElfFile::Section>(*result);
}

loader::SymbolIterator ElfFile::symbolsBegin() {
  return SymbolIterator(this, 0);
}

loader::SymbolIterator ElfFile::symbolsEnd() {
  return SymbolIterator(this, symbolCount());
}

loader::SymbolIteratorPair ElfFile::symbols() {
  return {symbolsBegin(), symbolsEnd()};
}

ElfFile::Symbol ElfFile::symbol(size_t index) {
  CARGO_ASSERT(index < symbolCount(), "Symbol index out of bounds");
  return Symbol(this, symbol_section->data().begin() +
                          symbol_section->entrySize() * index);
}

cargo::optional<ElfFile::Symbol> ElfFile::symbol(cargo::string_view name) {
  auto result =
      std::find_if(symbolsBegin(), symbolsEnd(),
                   [name](const Symbol &v) { return name == v.name(); });
  return (result == symbolsEnd()) ? cargo::nullopt
                                  : cargo::optional<ElfFile::Symbol>(*result);
}

cargo::optional<uint64_t> loader::ElfMap::getSectionTargetAddress(
    uint32_t index) const {
  auto it = std::find_if(
      sectionMappings.begin(), sectionMappings.end(),
      [index](const Mapping &m) { return m.section_index == index; });
  return (it == sectionMappings.end())
             ? cargo::nullopt
             : cargo::optional<uint64_t>{it->target_address};
}

cargo::optional<uint8_t *> loader::ElfMap::getSectionWritableAddress(
    uint32_t index) const {
  auto it = std::find_if(
      sectionMappings.begin(), sectionMappings.end(),
      [index](const Mapping &m) { return m.section_index == index; });
  return (it == sectionMappings.end())
             ? cargo::nullopt
             : cargo::optional<uint8_t *>{it->writable_address};
}

cargo::optional<cargo::array_view<uint8_t>>
loader::ElfMap::getRemainingStubSpace(uint32_t section_index) const {
  auto it = std::find_if(sectionMappings.begin(), sectionMappings.end(),
                         [section_index](const Mapping &m) {
                           return m.section_index == section_index;
                         });
  return (it == sectionMappings.end())
             ? cargo::nullopt
             : cargo::make_optional<cargo::array_view<uint8_t>>(
                   it->stub_address, it->writable_end);
}

cargo::optional<uint64_t> loader::ElfMap::getStubTargetAddress(
    uint32_t section_index) const {
  auto it = std::find_if(sectionMappings.begin(), sectionMappings.end(),
                         [section_index](const Mapping &m) {
                           return m.section_index == section_index;
                         });
  return (it == sectionMappings.end())
             ? cargo::nullopt
             : cargo::optional<uint64_t>{
                   it->target_address +
                   (it->stub_address - it->writable_address)};
}

void loader::ElfMap::shrinkRemainingStubSpace(uint32_t section_index,
                                              uint64_t bytes) {
  auto it = std::find_if(sectionMappings.begin(), sectionMappings.end(),
                         [section_index](const Mapping &m) {
                           return m.section_index == section_index;
                         });
  if (it != sectionMappings.end()) {
    it->stub_address += static_cast<size_t>(bytes);
  }
}

cargo::optional<uint64_t> loader::ElfMap::getSymbolTargetAddress(
    uint32_t index) const {
  auto sym = file->symbol(index);
  auto name = sym.name();

  auto cb = std::find_if(callbacks.begin(), callbacks.end(),
                         [&name](const Callback &c) { return c.name == name; });
  if (cb != callbacks.end()) {
    return cb->target_address;
  }
  if (sym.sectionIndex() == ElfFields::SymbolSpecialSection::ABSOLUTE) {
    return sym.value();
  }
  if (ElfFields::SymbolSpecialSection::isSpecial(sym.sectionIndex())) {
    return cargo::nullopt;
  }
  return getSectionTargetAddress(sym.sectionIndex()).map([&sym](uint64_t sa) {
    return sa + sym.value();
  });
}

cargo::optional<uint64_t> loader::ElfMap::getSymbolTargetAddress(
    cargo::string_view name) const {
  auto cb = std::find_if(callbacks.begin(), callbacks.end(),
                         [&name](const Callback &c) { return c.name == name; });
  if (cb != callbacks.end()) {
    return cb->target_address;
  }
  auto sym = file->symbol(name);
  if (!sym) {
    return cargo::nullopt;
  }
  if (sym->sectionIndex() == ElfFields::SymbolSpecialSection::ABSOLUTE) {
    return sym->value();
  }
  if (ElfFields::SymbolSpecialSection::isSpecial(sym->sectionIndex())) {
    return cargo::nullopt;
  }
  return getSectionTargetAddress(sym->sectionIndex()).map([&sym](uint64_t sa) {
    return sa + sym->value();
  });
}
