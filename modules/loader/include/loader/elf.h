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
/// @brief A simple ELF file format parser and utilities.
/// It supports parsing basic header, sections and symbols - just enough to
/// load object files produced by LLVM.

#ifndef LOADER_ELF_H_INCLUDED
#define LOADER_ELF_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/endian.h>
#include <cargo/error.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>

#include <array>
#include <cstdint>

namespace loader {

/// @brief Enumeration types for various values found in the ELF headers.
/// @note Refer to the SystemV ABI specification for the meaning of values:
/// http://www.sco.com/developers/gabi/latest/contents.html
namespace ElfFields {
/// @brief Whether the ELF file is 32- or 64-bit.
enum class Bitness : uint8_t { B32 = 1, B64 = 2 };
/// @brief Whether the ELF file is little- or big-endian.
enum class Endianness : uint8_t { LITTLE = 1, BIG = 2 };
/// @brief Version of the ELF file specification.
enum class Version : uint8_t { INVALID = 0, V1 = 1 };
/// @brief Binary interface (defining meaning of sections, etc.) of the file.
enum class ABI : uint8_t { SYSV = 0 };
/// @brief Target machine of the code in the ELF file.
enum class Machine : uint16_t {
  UNKNOWN = 0x00,
  X86 = 0x03,
  MIPS = 0x08,
  ARM = 0x28,
  X86_64 = 0x3E,
  AARCH64 = 0xB7
};
/// @brief Type of code the file contains.
enum class Type : uint16_t {
  RELOCATABLE = 0x01,  ///< A relocatable object file
  EXECUTABLE = 0x02,   ///< A typical executable file
  SHARED = 0x03,       ///< A shared object file, usually a library
  CORE_DUMP = 0x04     ///< A core dump
};
/// @brief Type of a section, found in section headers.
enum class SectionType : uint32_t {
  NONE = 0x00,           ///< Undefined
  PROGBITS = 0x01,       ///< Contains executable code
  SYMTAB = 0x02,         ///< Symbol table
  STRTAB = 0x03,         ///< String table
  RELA = 0x04,           ///< Relocations with explicit addends
  HASH = 0x05,           ///< Symbol hash table
  DYNAMIC = 0x06,        ///< Section for the dynamic linker
  NOTE = 0x07,           ///< Vendor-provided notes
  NOBITS = 0x08,         ///< Section to be filled with zeros
  REL = 0x09,            ///< Relocations without explicit addends
  SHLIB = 0x0A,          ///< Shared library list
  DYNSYM = 0x0B,         ///< Dynamic symbols
  INIT_ARRAY = 0x0E,     ///< Static constructor array
  FINI_ARRAY = 0x0F,     ///< Static destructor array
  PREINIT_ARRAY = 0x10,  ///< Static preconstructor array
  GROUP = 0x11,          ///< Group of sections
  SYMTAB_SHNDX = 0x12    ///< Symbol table's section indices for >2^16 sections
};
/// @brief Flags found in section headers.
namespace SectionFlags {
enum Type : uint32_t {
  WRITE = 0x01,       ///< Writable memory
  ALLOC = 0x02,       ///< Needs to be mapped into memory for program execution
  EXECINSTR = 0x04,   ///< Executable memory
  MERGE = 0x10,       ///< Can be merged with identical sections
  STRINGS = 0x20,     ///< Contains strings
  INFO_LINK = 0x40,   ///< The info field contains a section header table index
  LINK_ORDER = 0x80,  ///< Special ordering requirements
  OS_NONCONFORMING = 0x100,  ///< Special OS-specific processing requirement
  GROUP = 0x200,             ///< Member of a group
  TLS = 0x400,               ///< Holds thread-local storage
  COMPRESSED = 0x800         ///< Compressed contents
};
}
/// @brief Special values for symbols' section index field.
namespace SymbolSpecialSection {
enum Type : uint16_t {
  UNDEFINED = 0x00,   ///< Undefined symbol
  ABSOLUTE = 0xFFF1,  ///< Absolute value symbol
  COMMON = 0xFFF2,    ///< Needs allocation
  XINDEX = 0xFFFF     ///< Index too big to fit, look in symtab_shndx section
};
/// @brief Checks if a section index of a symbol has a special meaning.
inline bool isSpecial(uint16_t sidx) {
  return sidx == UNDEFINED || sidx >= 0xFF00;
}
}  // namespace SymbolSpecialSection
/// @brief Controls the priorities of conflicting symbols.
enum class SymbolBinding : uint8_t { LOCAL = 0x0, GLOBAL = 0x1, WEAK = 0x2 };
/// @brief What the symbol represents.
enum class SymbolType : uint8_t {
  NONE = 0x00,
  OBJECT = 0x01,
  FUNCTION = 0x02,
  SECTION = 0x03,
  FILE = 0x04,
  COMMON = 0x05,
  TLS = 0x06
};
/// @brief Visibility of the symbol to other programs.
enum class SymbolVisibility : uint8_t {
  DEFAULT = 0x00,  // trailing underscore because it's a C++ keyword
  INTERNAL = 0x01,
  HIDDEN = 0x02,
  PROTECTED = 0x03  // same as above
};

/// @brief Extracts the SymbolBinding bitfield from the st_info ELF field.
inline SymbolBinding getSymbolBinding(uint8_t st_info) {
  return static_cast<SymbolBinding>(st_info >> 4);
}

/// @brief Extracts the SymbolType bitfield from the st_info ELF field.
inline SymbolType getSymbolType(uint8_t st_info) {
  return static_cast<SymbolType>(st_info & 0x0F);
}

/// @brief Extracts the SymbolVisibility bitfield from the st_other ELF field.
inline SymbolVisibility getSymbolVisibility(uint8_t st_other) {
  return static_cast<SymbolVisibility>(st_other & 0x03);
}

/// @brief Names of important special sections.
const char *const SYMBOL_TABLE_SECTION = ".symtab";
/// @brief Names of important special sections.
const char *const SYMBOL_NAMES_SECTION = ".strtab";
/// @brief Names of important special sections.
const char *const SECTION_NAMES_SECTION = ".shstrtab";

}  // namespace ElfFields

struct SectionIterator;
struct SectionIteratorPair;
struct SymbolIterator;
struct SymbolIteratorPair;

/// @brief Wrapper for an ELF file.
class ElfFile {
 public:
  /// @brief Default constructor.
  ElfFile();
  /// @brief Requires aligned_data to be aligned to an 8-byte boundary.
  ElfFile(cargo::array_view<uint8_t> aligned_data);

  /// @brief Checks if the specified data is a valid, 8-byte aligned ELF file.
  static bool isValidElf(cargo::array_view<uint8_t> aligned_data);

  /// @brief Identification header shared by both ELF formats.
  struct HeaderIdent {
    static const std::array<uint8_t, 4> ELF_MAGIC;
    uint8_t magic[4];
    ElfFields::Bitness bitness;
    ElfFields::Endianness endianness;
    ElfFields::Version version;
    ElfFields::ABI abi;
    uint8_t padding[8];
  };
  static_assert(sizeof(HeaderIdent) == 16,
                "Wrong ELF header identification section size");

  /// @brief ELF file header, instantiated with addr_t of either uint32_t or
  /// uint64_t for ELF32 and ELF64 respectively
  template <typename addr_t>
  struct Header {
    static_assert(sizeof(addr_t) == 4 || sizeof(addr_t) == 8,
                  "Wrong addr_t type.");
    HeaderIdent identifier;
    ElfFields::Type type;
    ElfFields::Machine machine;
    uint32_t version;
    addr_t entry_point;
    addr_t program_header_offset;
    addr_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t pht_entry_size;
    uint16_t pht_entry_count;
    uint16_t sht_entry_size;
    uint16_t sht_entry_count;
    uint16_t sht_names_index;
  };
  using Header32 = Header<uint32_t>;
  using Header64 = Header<uint64_t>;

  /// @brief Field accessors, choosing the right bitness and converting
  /// endianness if needed.
  inline ElfFields::Type type() const {
    return field(is32Bit() ? header32()->type : header64()->type);
  }
  /// @brief Field accessors, choosing the right bitness and converting
  /// endianness if needed.
  inline ElfFields::Machine machine() const {
    return field(is32Bit() ? header32()->machine : header64()->machine);
  }
  /// @brief Field accessors, choosing the right bitness and converting
  /// endianness if needed.
  inline uint32_t flags() const {
    return field(is32Bit() ? header32()->flags : header64()->flags);
  }

  /// @brief Section header, instantiated with addr_t of either uint32_t or
  /// uint64_t for ELF32 and ELF64 respectively
  template <typename addr_t>
  struct SectionHeader {
    static_assert(sizeof(addr_t) == 4 || sizeof(addr_t) == 8,
                  "Wrong addr_t type.");
    uint32_t name_offset;
    ElfFields::SectionType type;
    ElfFields::SectionFlags::Type flags;
    addr_t virtual_address;
    addr_t file_offset;
    addr_t size;
    uint32_t link;
    uint32_t info;
    addr_t alignment;
    addr_t entry_size;
  };
  using SectionHeader32 = SectionHeader<uint32_t>;
  using SectionHeader64 = SectionHeader<uint64_t>;

  /// @brief ELF32 symbol table entry
  struct Symbol32 {
    using addr_t = uint32_t;
    uint32_t name_offset;
    addr_t value;
    addr_t size;
    uint8_t info;
    uint8_t other;
    uint16_t section;
  };
  /// @brief ELF64 symbol table entry
  struct Symbol64 {
    using addr_t = uint64_t;
    uint32_t name_offset;
    uint8_t info;
    uint8_t other;
    uint16_t section;
    addr_t value;
    addr_t size;
  };

  /// @brief Wrapper for a section in the ELF file.
  struct Section {
    inline Section() : file(nullptr), header(nullptr) {}
    /// @brief Constructs the object with the section header starting at ptr.
    inline Section(ElfFile *file, const void *ptr) : file(file), header(ptr) {}

    ElfFile *file;
    const void *header;

    /// @brief Gets the ELF32 section header.
    inline const SectionHeader32 *header32() const {
      return reinterpret_cast<const SectionHeader32 *>(header);
    }
    /// @brief Gets the ELF64 section header.
    inline const SectionHeader64 *header64() const {
      return reinterpret_cast<const SectionHeader64 *>(header);
    }

    /// @brief Gets the index of this section in the section header table.
    inline uint32_t index() const {
      auto sht_entry_size = static_cast<size_t>(
          file->is32Bit() ? file->field(file->header32()->sht_entry_size)
                          : file->field(file->header64()->sht_entry_size));
      auto bytes_from_first_header =
          reinterpret_cast<size_t>(header) -
          reinterpret_cast<size_t>(file->section(0).header);
      return static_cast<uint32_t>(bytes_from_first_header / sht_entry_size);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    const cargo::string_view name() const;
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline ElfFields::SectionType type() const {
      return file->field(file->is32Bit() ? header32()->type : header64()->type);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline ElfFields::SectionFlags::Type flags() const {
      return file->field(file->is32Bit() ? header32()->flags
                                         : header64()->flags);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline uint64_t virtual_address() const {
      return file->field(file->is32Bit() ? header32()->virtual_address
                                         : header64()->virtual_address);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline uint64_t file_offset() const {
      return file->field(file->is32Bit() ? header32()->file_offset
                                         : header64()->file_offset);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline uint64_t size() const {
      return file->field(file->is32Bit() ? header32()->size : header64()->size);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline uint64_t entrySize() const {
      return file->field(file->is32Bit() ? header32()->entry_size
                                         : header64()->entry_size);
    }

    /// @brief Returns an estimate of what size to allocate for section, may be
    /// different from size() because relocations may need generated stubs.
    inline uint64_t sizeToAlloc() const {
      if (file->machine() == ElfFields::Machine::X86 ||
          file->machine() == ElfFields::Machine::X86_64) {
        // no need to write new code on x86 - all relocations fit into target
        // instructions
        return size();
      } else {
        // conservatively reserve additional 2 kilobytes
        // this is enough for at least 100 far relocations on AArch64 (which are
        // the biggest of all architectures)
        return size() + 2048;
      }
    }

    /// @brief Gets a view of the bytes in this section.
    inline const cargo::array_view<uint8_t> data() const {
      CARGO_ASSERT(type() != ElfFields::SectionType::NOBITS,
                   "Trying to get a data view for a nobits section.");
      const size_t offset = file->field(
          file->is32Bit() ? header32()->file_offset : header64()->file_offset);
      return {file->bytes.begin() + offset, static_cast<size_t>(size())};
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    inline uint64_t alignment() const {
      return file->field(file->is32Bit() ? header32()->alignment
                                         : header64()->alignment);
    }
  };

  /// @brief Wrapper for a symbol in the ELF file.
  struct Symbol {
    inline Symbol() : file(nullptr), entry(nullptr) {}
    /// @brief Constructs the object with the symbol entry starting at ptr.
    inline Symbol(ElfFile *file, const void *ptr) : file(file), entry(ptr) {}

    ElfFile *file;
    const void *entry;

    /// @brief Gets the ELF32 symbol entry.
    inline const Symbol32 *symbol32() const {
      return reinterpret_cast<const Symbol32 *>(entry);
    }
    /// @brief Gets the ELF64 symbol entry.
    inline const Symbol64 *symbol64() const {
      return reinterpret_cast<const Symbol64 *>(entry);
    }

    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    cargo::optional<cargo::string_view> name() const;
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    ///
    /// @note The index might be one of ElfFields::SymbolSpecialSection values
    int sectionIndex() const {
      return file->field(file->is32Bit() ? symbol32()->section
                                         : symbol64()->section);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    uint64_t value() const {
      return file->field(file->is32Bit() ? symbol32()->value
                                         : symbol64()->value);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    uint64_t size() const {
      return file->field(file->is32Bit() ? symbol32()->size : symbol64()->size);
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    ElfFields::SymbolBinding binding() const {
      return ElfFields::getSymbolBinding(
          file->field(file->is32Bit() ? symbol32()->info : symbol64()->info));
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    ElfFields::SymbolType type() const {
      return ElfFields::getSymbolType(
          file->field(file->is32Bit() ? symbol32()->info : symbol64()->info));
    }
    /// @brief Field accessors, choosing the right bitness and converting
    /// endianness if needed.
    ElfFields::SymbolVisibility visibility() const {
      return ElfFields::getSymbolVisibility(
          file->field(file->is32Bit() ? symbol32()->other : symbol64()->other));
    }
  };

  /// @brief View on the whole ELF file.
  /// @note The begin() pointer of this has to be aligned to an 8-byte boundary.
  cargo::array_view<uint8_t> bytes;

  /// @brief The section with the symbol table in it.
  cargo::optional<Section> symbol_section;

  /// @brief Gets the identification part of the ELF header, shared across both
  /// ELF formats.
  inline const HeaderIdent *headerIdent() const {
    return reinterpret_cast<const HeaderIdent *>(bytes.begin());
  }

  /// @brief Gets the ELF32 file header.
  inline const Header32 *header32() const {
    return reinterpret_cast<const Header32 *>(bytes.begin());
  }

  /// @brief Gets the ELF64 file header.
  inline const Header64 *header64() const {
    return reinterpret_cast<const Header64 *>(bytes.begin());
  }

  /// @brief Checks if the ELF file is in the 32-bit format.
  inline bool is32Bit() const {
    CARGO_ASSERT(!bytes.empty(), "Using a null ElfFile instance");
    return headerIdent()->bitness == ElfFields::Bitness::B32;
  }

  /// @brief Gets the number of sections in the ELF file.
  inline size_t sectionCount() const {
    return static_cast<size_t>(is32Bit() ? header32()->sht_entry_count
                                         : header64()->sht_entry_count);
  }

  /// @brief Gets the nth section in the ELF file.
  Section section(size_t index);
  /// @brief Finds a section in the ELF file by its name.
  cargo::optional<Section> section(cargo::string_view name);

  /// @brief The sections iterators.
  SectionIterator sectionsBegin();
  /// @brief The sections iterators.
  SectionIterator sectionsEnd();
  /// @brief The sections range.
  SectionIteratorPair sections();

  /// @brief Gets the number of symbols in the ELF file.
  inline size_t symbolCount() const {
    return static_cast<size_t>(symbol_section ? symbol_section->size() /
                                                    symbol_section->entrySize()
                                              : 0);
  }

  /// @brief Gets the nth symbol in the ELF file.
  Symbol symbol(size_t index);
  /// @brief Finds a symbol in the ELF file by its name.
  cargo::optional<Symbol> symbol(cargo::string_view name);

  /// @brief The symbols iterators.
  SymbolIterator symbolsBegin();
  /// @brief The symbols iterators.
  SymbolIterator symbolsEnd();
  /// @brief The symbols range.
  SymbolIteratorPair symbols();

  /// @brief Use this wrapper if reading directly from ELF structures, it
  /// converts the values in memory to the right endianness for the CPU.
  template <typename Integer>
  inline std::enable_if_t<std::is_unsigned_v<Integer>, Integer> field(
      Integer v) const {
    CARGO_ASSERT(!bytes.empty(), "Using a null ElfFile instance");
    return (headerIdent()->endianness == (cargo::is_little_endian()
                                              ? ElfFields::Endianness::LITTLE
                                              : ElfFields::Endianness::BIG))
               ? v
               : cargo::byte_swap(v);
  }

  /// @brief Use this wrapper if reading directly from ELF structures, it
  /// converts the values in memory to the right endianness for the CPU.
  template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
  inline std::enable_if_t<std::is_unsigned_v<std::underlying_type_t<Enum>>,
                          Enum>
  field(Enum v) const {
    using Integer = std::underlying_type_t<Enum>;
    CARGO_ASSERT(!bytes.empty(), "Using a null ElfFile instance");
    return (headerIdent()->endianness == (cargo::is_little_endian()
                                              ? ElfFields::Endianness::LITTLE
                                              : ElfFields::Endianness::BIG))
               ? v
               : static_cast<Enum>(cargo::byte_swap(static_cast<Integer>(v)));
  }
};

/// @brief A map of ELF sections and symbols to virtual memory.
struct ElfMap {
  /// @brief A mapping for a single ELF section.
  struct Mapping {
    /// @brief Index of the ELF section.
    uint32_t section_index;
    /// @brief Address in the host memory where relocations can be written to.
    uint8_t *writable_address;
    /// @brief Current writable location free for writing relocation stubs to.
    uint8_t *stub_address;
    /// @brief Pointer to one beyond the last writable byte.
    uint8_t *writable_end;
    /// @brief Address in the device memory where relocations will point to.
    uint64_t target_address;
  };

  /// @brief A mapping for a callback under a symbol name.
  struct Callback {
    /// @brief Name of the symbol.
    std::string name;
    /// @brief Address it is mapped to.
    uint64_t target_address;
  };

  inline ElfMap() : file(nullptr) {}
  inline ElfMap(ElfFile *file) : file(file) {}

  /// @brief The symbol mappings iterators.
  inline Mapping *sectionMappingsBegin() { return sectionMappings.begin(); }
  /// @brief The symbol mappings iterators.
  inline Mapping *sectionMappingsEnd() { return sectionMappings.end(); }

  /// @brief Adds a new ELF section mapping.
  inline cargo::result addSectionMapping(const ElfFile::Section &section,
                                         uint8_t *writable_address,
                                         uint8_t *writable_end,
                                         uint64_t target_address) {
    return sectionMappings.push_back({section.index(), writable_address,
                                      writable_address + section.size(),
                                      writable_end, target_address});
  }

  /// @brief Adds a new callback, which allows to define undefined symbols that
  /// are present outside of the ELF file.
  [[nodiscard]] inline cargo::result addCallback(cargo::string_view name,
                                                 uint64_t target_address) {
    return callbacks.push_back(
        {std::string{name.data(), name.size()}, target_address});
  }

  /// @brief Gets the address where the section with a given index is mapped in
  /// host memory.
  cargo::optional<uint8_t *> getSectionWritableAddress(uint32_t index) const;
  /// @brief Gets the address where the section with a given index is mapped in
  /// target memory.
  cargo::optional<uint64_t> getSectionTargetAddress(uint32_t index) const;

  /// @brief Gets the remaining space available for writing out relocation stubs
  /// in an ELF section.
  cargo::optional<cargo::array_view<uint8_t>> getRemainingStubSpace(
      uint32_t section_index) const;
  /// @brief Gets the address on the target device to the beginning of the space
  /// available for writing out relocation stubs in an ELF section.
  cargo::optional<uint64_t> getStubTargetAddress(uint32_t section_index) const;
  /// @brief Shrinks the relocation stub space for a given ELF section by a
  /// given number of bytes.
  void shrinkRemainingStubSpace(uint32_t section_index, uint64_t bytes);

  /// @brief Gets the address where the symbol with a given index is mapped in
  /// device memory.
  cargo::optional<uint64_t> getSymbolTargetAddress(uint32_t index) const;
  /// @brief Gets the address where the symbol with a given name is mapped in
  /// device memory.
  cargo::optional<uint64_t> getSymbolTargetAddress(
      cargo::string_view name) const;

#ifndef NDEBUG
  /// @brief Look up a symbol's name from its index.
  // Note that `symbol()` only performs bounds checking in debug builds, so
  // further checks may be required if this function is used in non-debug
  // builds.
  inline cargo::optional<cargo::string_view> getSymbolName(
      uint32_t index) const {
    return file->symbol(index).name();
  }
#endif

 private:
  ElfFile *file;
  cargo::small_vector<Mapping, 8> sectionMappings;
  cargo::small_vector<Callback, 8> callbacks;
};

/// @brief Iterates over the sections in an ELF file.
struct SectionIterator {
  ElfFile *file;
  size_t index;

  inline SectionIterator(ElfFile *file, size_t index = 0)
      : file(file), index(index) {}

  inline SectionIterator &operator++() {
    ++index;
    return *this;
  }

  inline SectionIterator operator++(int) {
    SectionIterator old = *this;
    ++index;
    return old;
  }

  inline SectionIterator &operator--() {
    --index;
    return *this;
  }

  inline SectionIterator operator--(int) {
    SectionIterator old = *this;
    --index;
    return old;
  }

  inline ElfFile::Section &operator*() {
    CARGO_ASSERT(file != nullptr,
                 "Section iterator using a null ElfFile instance");
    CARGO_ASSERT(index < file->sectionCount(),
                 "Section iterator out of bounds");
    updateCurrent();
    return current;
  }

  inline ElfFile::Section &operator->() { return operator*(); }

  inline bool operator==(const SectionIterator &rhs) {
    return rhs.file == file && rhs.index == index;
  }

  inline bool operator!=(const SectionIterator &rhs) {
    return rhs.file != file || rhs.index != index;
  }

  inline void updateCurrent() { current = file->section(index); }

 private:
  ElfFile::Section current;
};

/// @brief Iterates over the symbols in an ELF file.
struct SymbolIterator {
  ElfFile *file;
  size_t index;

  inline SymbolIterator(ElfFile *file, size_t index = 0)
      : file(file), index(index) {}

  inline SymbolIterator &operator++() {
    ++index;
    return *this;
  }

  inline SymbolIterator operator++(int) {
    SymbolIterator old = *this;
    ++index;
    return old;
  }

  inline SymbolIterator &operator--() {
    --index;
    return *this;
  }

  inline SymbolIterator operator--(int) {
    SymbolIterator old = *this;
    --index;
    return old;
  }

  inline ElfFile::Symbol &operator*() {
    CARGO_ASSERT(file != nullptr,
                 "Symbol iterator using a null ElfFile instance");
    CARGO_ASSERT(index < file->symbolCount(), "Symbol iterator out of bounds");
    updateCurrent();
    return current;
  }

  inline ElfFile::Symbol &operator->() { return operator*(); }

  inline bool operator==(const SymbolIterator &rhs) {
    return rhs.file == file && rhs.index == index;
  }

  inline bool operator!=(const SymbolIterator &rhs) {
    return rhs.file != file || rhs.index != index;
  }

  inline void updateCurrent() { current = file->symbol(index); }

 private:
  ElfFile::Symbol current;
};

/// @brief Wraps the begin and end section iterators to allow range for loops.
struct SectionIteratorPair {
  inline SectionIteratorPair(SectionIterator beginIter, SectionIterator endIter)
      : beginIter(beginIter), endIter(endIter) {}

  inline SectionIterator &begin() { return beginIter; }
  inline const SectionIterator &begin() const { return beginIter; }

  inline SectionIterator &end() { return endIter; }
  inline const SectionIterator &end() const { return endIter; }

  SectionIterator beginIter;
  SectionIterator endIter;
};

/// @brief Wraps the begin and end section iterators to allow range for loops.
struct SymbolIteratorPair {
  inline SymbolIteratorPair(SymbolIterator beginIter, SymbolIterator endIter)
      : beginIter(beginIter), endIter(endIter) {}

  inline SymbolIterator &begin() { return beginIter; }
  inline const SymbolIterator &begin() const { return beginIter; }

  inline SymbolIterator &end() { return endIter; }
  inline const SymbolIterator &end() const { return endIter; }

  SymbolIterator beginIter;
  SymbolIterator endIter;
};

}  // namespace loader

namespace std {
template <>
struct iterator_traits<loader::SectionIterator> {
  using difference_type = decltype(loader::SectionIterator::index);
  using value_type = loader::ElfFile::Section;
  using pointer = loader::ElfFile::Section *;
  using reference = loader::ElfFile::Section &;
  using iterator_category = std::bidirectional_iterator_tag;
};
template <>
struct iterator_traits<loader::SymbolIterator> {
  using difference_type = decltype(loader::SymbolIterator::index);
  using value_type = loader::ElfFile::Symbol;
  using pointer = loader::ElfFile::Symbol *;
  using reference = loader::ElfFile::Symbol &;
  using iterator_category = std::bidirectional_iterator_tag;
};
}  // namespace std

static_assert(cargo::is_input_iterator<loader::SectionIterator>::value,
              "SectionIterator is not an input iterator");
static_assert(cargo::is_input_iterator<loader::SymbolIterator>::value,
              "SymbolIterator is not an input iterator");

#endif
