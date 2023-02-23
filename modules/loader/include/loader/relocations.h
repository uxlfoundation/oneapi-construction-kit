// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Utilities for supporting ELF relocations.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef LOADER_RELOCATIONS_H_INCLUDED
#define LOADER_RELOCATIONS_H_INCLUDED

#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <loader/elf.h>
#include <array>

namespace loader {

/// @note Fields are stored in native endianness, not ELF file's endianness.
struct Relocation {
  /// @brief Relocation entry types present in ELF files - the A variants have
  /// an additional field with an explicit addend.
  enum class EntryType { Elf32Rel, Elf32RelA, Elf64Rel, Elf64RelA };
  /// @brief A map of stub locations, a separate one should be instantiated for
  /// each relocated section in the ELF file.
  ///
  /// Some architectures, like Arm and AArch64, require the linker to generate
  /// additional code in case a relocation exceeds the number of bits available
  /// in the relocated instruction. Such code constructs the target address
  /// piece by piece, and then jumps to it, and it is called a stub. The
  /// relocation is then relocated to point at the stub instead of the target
  /// symbol. Because stubs are appended to the end of a section, they should
  /// fit within the addressable space of the instruction, if they don't that's
  /// the fault of the compiler, as it generated a section that's too big for
  /// that architecture.
  struct StubMap {
    struct Entry {
      uint64_t value;
      uint64_t target;
    };
    cargo::small_vector<Entry, 4> stubs;
    cargo::optional<uint64_t> getTarget(uint64_t value) const;
  };

  /// @brief Platform-dependent type of the relocation.
  uint32_t type;
  /// @brief Index of the symbol the relocation points to in the ELF file.
  uint32_t symbol_index;
  /// @brief Offset at which the instruction/pointer to relocate is located in
  /// its section.
  uint64_t offset;
  /// @brief An addend used by some relocation types in calculating the target
  /// address.
  int64_t addend;
  /// @brief Index of the section in which the relocation is being performed
  int32_t section_index;

  /// @brief Constructs a relocation object from an entry in an ELF file.
  template <EntryType>
  static Relocation fromElfEntry(ElfFile &file, uint32_t section_id,
                                 const void *raw_entry);

  /// @brief Resolves this relocation in mapped memory of the ELF file.
  ///
  /// @note @p map must contain host CPU addresses to writable memory.
  ///
  /// @return Returns whether the relocation succeeded.
  bool resolve(ElfFile &file, ElfMap &map, StubMap &stubs);
};

template <>
Relocation Relocation::fromElfEntry<Relocation::EntryType::Elf32Rel>(
    ElfFile &file, uint32_t section_id, const void *raw_entry);

template <>
Relocation Relocation::fromElfEntry<Relocation::EntryType::Elf32RelA>(
    ElfFile &file, uint32_t section_id, const void *raw_entry);

template <>
Relocation Relocation::fromElfEntry<Relocation::EntryType::Elf64Rel>(
    ElfFile &file, uint32_t section_id, const void *raw_entry);

template <>
Relocation Relocation::fromElfEntry<Relocation::EntryType::Elf64RelA>(
    ElfFile &file, uint32_t section_id, const void *raw_entry);

/// @brief Resolves all relocations in the mapped memory of the ELF file.
///
/// @note @p map must contain host CPU addresses to writable memory.
///
/// @return Returns whether all the relocations succeeded.
bool resolveRelocations(ElfFile &file, ElfMap &map);

}  // namespace loader

#endif
