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

#include <cargo/endian.h>
#include <cargo/error.h>
#include <cargo/string_view.h>
#include <inttypes.h>
#include <loader/elf.h>
#include <loader/relocation_types.h>
#include <loader/relocations.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>

// There are many relocation types, but LLVM only emits a few, only those
// present in lib/ExecutionEngine/RuntimeDyld/RuntimeDyldELF.cpp are implemented

// Useful resources:
// General ELF information: https://wiki.osdev.org/ELF
// A nice overview of basic X86 and SPARC (not implemented here) relocations:
// https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-54839.html
// The i386 SysV ABI: http://www.sco.com/developers/devspecs/abi386-4.pdf
// The x86_64 SysV ABI: https://www.uclibc.org/docs/psABI-x86_64.pdf
// The Arm ELF specification:
// https://developer.arm.com/docs/ihi0044/h
// The AArch64 ELF specification:
// https://developer.arm.com/docs/ihi0056/f/elf-for-the-arm-64-bit-architecture-aarch64-abi-2019q2-documentation
// LLVM's RuntimeDyldELF implementation

// Terminology used here:
// * target address - address on the device the code will be run on
// * writable address - address on the host device which handles loading of the
//   ELF file
// * (relocation) offset - Number of bytes from the beginning of a section to
//   the relocated instruction
// * symbol address - address of the symbol that the relocation points to
// * relocated section base - target address of the section, the relocations of
//   which are being processed
// * relocated section begin - writable address counterpart of the above
// * value - the value or instruction code at the relocation target
// * real and truncated values are used when the relocation masks out some of
//   the bits of the symbol address to check if it's still valid
// * PC - program counter, means the relocation is relative to the target
//   address of the instruction being relocated
// * absolute - the relocation does not depend on where it's performed, but only
//   on the target symbol address

cargo::optional<uint64_t> loader::Relocation::StubMap::getTarget(
    uint64_t value) const {
  auto it = std::find_if(stubs.begin(), stubs.end(),
                         [value](const Entry &e) { return e.value == value; });
  return (it != stubs.end()) ? cargo::optional<uint64_t>(it->target)
                             : cargo::nullopt;
}

namespace {
// Gets the [first, first+size) bits of value as a size-bit integer.
template <typename Integer>
inline Integer getBitRange(const Integer &value, int first, int size) {
  return (value >> first) & ((1 << size) - 1);
}

// Sets the [first, first+size) bits of value to subvalue's [0, size) bits.
template <typename Integer>
inline Integer setBitRange(Integer value, Integer subvalue, int first,
                           int size) {
  Integer mask = ((1 << size) - 1) << first;
  return (value & ~mask) | ((subvalue << first) & mask);
}

// Returns a triple of common variables used in relocation calculation:
// * Writable position in for relocations (host memory)
// * P:    : position of the relocation (in target memory)
// * S + A : value of the symbol in the symbol table plus any relocation addend
//           (in target memory)
// See the link below for an explanation of the symbols - described in the
// RISC-V ELF psABI document, but relevant for other architectures:
// https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc#calculation-symbols
template <typename T>
cargo::optional<std::tuple<uint8_t *, T, T>> decomposeRelocation(
    const loader::Relocation &r, loader::ElfMap &map) {
  static_assert(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>,
                "Invalid relocation size");
  using SignedT = std::make_signed_t<T>;
  const T relocation_offset = r.offset & 0xFFFFFFFF;
  const SignedT addend = static_cast<SignedT>(r.addend);

  T symbol_target_address =
      static_cast<T>(map.getSymbolTargetAddress(r.symbol_index).value_or(0));
  if (symbol_target_address == 0) {
#ifndef NDEBUG
    auto name = map.getSymbolName(r.symbol_index).value_or("<unknown symbol>");
    (void)fprintf(stderr, "Missing symbol: %.*s\n", (int)name.size(),
                  name.data());
#endif
    return cargo::nullopt;
  }
  symbol_target_address += addend;
  const T relocated_section_base =
      static_cast<T>(map.getSectionTargetAddress(r.section_index).value_or(0));
  if (relocated_section_base == 0) {
    return cargo::nullopt;
  }
  uint8_t *relocated_section_begin =
      map.getSectionWritableAddress(r.section_index).value_or(nullptr);
  if (relocated_section_begin == nullptr) {
    return cargo::nullopt;
  }

  uint8_t *relocation_address = relocated_section_begin + relocation_offset;

  const T relocation_target_address =
      relocated_section_base + relocation_offset;

  return std::make_tuple(relocation_address, relocation_target_address,
                         symbol_target_address);
}

bool resolveX86_32(const loader::Relocation &r, loader::ElfMap &map) {
  auto relocation_data = decomposeRelocation<uint32_t>(r, map);
  if (!relocation_data) {
    return false;
  }
  auto [relocation_address, relocated_target_address, symbol_target_address] =
      *relocation_data;

  using namespace loader::RelocationTypes::X86_32;
  switch (r.type) {
    default:
      CARGO_ASSERT(0, "Unsupported relocation type.");
      return false;
    case R_386_NONE:
      break;
    // absolute 32-bit relocation
    // x86 relocates only values, thanks to its variable length encoding no
    // instruction parsing needs to be performed
    case R_386_32: {
      // R_386_32 stores an addend at the relocation target. The exact format of
      // the addend is unclear, but it's at least 10 bits unsigned. It's unknown
      // whether negative values are possible.
      uint32_t implicit_addend;
      cargo::read_little_endian(&implicit_addend, relocation_address);
#ifndef NDEBUG
      // If this warning is ever triggered, then we can investigate further
      if (implicit_addend & uint32_t(0x80008000)) {
        (void)fprintf(stderr,
                      "WARNING: Relocation with possibly negative offset\n");
      }
#endif
      const uint32_t value = symbol_target_address + implicit_addend;
      cargo::write_little_endian(value, relocation_address);
      break;
    }
    // PC-relative 32-/16-/8-bit relocations
    case R_386_PC32: {
      // R_386_PC32 stores an addend at the relocation target as an int8_t
      uint8_t implicit_addend;
      cargo::read_little_endian(&implicit_addend, relocation_address);
      const uint32_t value = symbol_target_address - relocated_target_address +
                             static_cast<int8_t>(implicit_addend);
      cargo::write_little_endian(value, relocation_address);
      break;
    }
    case R_386_PC16: {
      const uint32_t real_value =
          symbol_target_address - relocated_target_address;
      const uint16_t trunc_value = real_value & 0xFFFF;
      if (static_cast<int32_t>(real_value) !=
          static_cast<int16_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    case R_386_PC8: {
      const uint32_t real_value =
          symbol_target_address - relocated_target_address;
      const uint8_t trunc_value = real_value & 0xFF;
      if (static_cast<int32_t>(real_value) !=
          static_cast<int8_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
  }
  return true;
}

bool resolveX86_64(const loader::Relocation &r, loader::ElfMap &map) {
  auto relocation_data = decomposeRelocation<uint64_t>(r, map);
  if (!relocation_data) {
    return false;
  }
  auto [relocation_address, relocated_target_address, symbol_target_address] =
      *relocation_data;

  using namespace loader::RelocationTypes::X86_64;
  switch (r.type) {
    default:
      CARGO_ASSERT(0, "Unsupported relocation type.");
      return false;
    case R_X86_64_NONE:
      break;
    // absolute 64-bit relocation
    // x86_64 relocates only values, thanks to its variable length encoding no
    // instruction parsing needs to be performed
    case R_X86_64_64: {
      cargo::write_little_endian(symbol_target_address, relocation_address);
      break;
    }
    // PC-relative 64-bit relocation
    case R_X86_64_PC64: {
      const uint64_t value = symbol_target_address - relocated_target_address;
      cargo::write_little_endian(value, relocation_address);
      break;
    }
    // absolute 32-bit relocation asserting its zero-extension is valid
    case R_X86_64_32: {
      const uint64_t real_value = symbol_target_address;
      const uint32_t trunc_value = real_value & 0xFFFFFFFF;
      if (real_value != trunc_value) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    // absolute 32-bit relocation asserting its sign-extension is valid
    case R_X86_64_32S: {
      const uint64_t real_value = symbol_target_address;
      const uint32_t trunc_value = real_value & 0xFFFFFFFF;
      if (static_cast<int64_t>(real_value) !=
          static_cast<int32_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    // 32-, 16- and 8-bit PC-relative relocations asserting their
    // sign-extensions are valid
    case R_X86_64_PC32: {
      const uint64_t real_value =
          symbol_target_address - relocated_target_address;
      const uint32_t trunc_value = real_value & 0xFFFFFFFF;
      if (static_cast<int64_t>(real_value) !=
          static_cast<int32_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    case R_X86_64_PC16: {
      const uint64_t real_value =
          symbol_target_address - relocated_target_address;
      const uint16_t trunc_value = real_value & 0xFFFF;
      if (static_cast<int64_t>(real_value) !=
          static_cast<int16_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    case R_X86_64_PC8: {
      const uint64_t real_value =
          symbol_target_address - relocated_target_address;
      const uint8_t trunc_value = real_value & 0xFF;
      if (static_cast<int64_t>(real_value) !=
          static_cast<int8_t>(trunc_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
  }
  return true;
}

// assumes little-endian Arm, because big-endian is extremely rare
bool resolveArm(const loader::Relocation &r, loader::ElfMap &map,
                loader::Relocation::StubMap &stubs) {
  auto relocation_data = decomposeRelocation<uint32_t>(r, map);
  if (!relocation_data) {
    return false;
  }
  auto [relocation_address, relocation_target_address, symbol_tgt_address] =
      *relocation_data;
  // We can't auto-capture structured bindings (lambda, below) so make an
  // explicit copy of it here.
  auto symbol_target_address = symbol_tgt_address;

  uint32_t value;
  cargo::read_little_endian(&value, relocation_address);

  // returns the address of the stub to jump to
  auto getOrCreateStub = [&]() -> uint32_t {
    auto found_stub_target = stubs.getTarget(symbol_target_address);
    if (found_stub_target) {
      return static_cast<uint32_t>(*found_stub_target);
    }
    auto remaining = map.getRemainingStubSpace(r.section_index);
    if (!remaining || remaining->size() < 4) {
      return 0;
    }
    // This generates the following instruction followed by a 32-bit address (8
    // bytes in total):
    // ldr pc, [pc, #-4]
    // ^ pc points 8 bytes ahead of the current instruction, so this loads the
    // next dword into the program counter.
    cargo::write_little_endian(uint32_t{0xE51FF004}, remaining->begin());
    cargo::write_little_endian(symbol_target_address, remaining->begin() + 4);
    auto target = *map.getStubTargetAddress(r.section_index);
    map.shrinkRemainingStubSpace(r.section_index, 8);
    return static_cast<uint32_t>(target);
  };

  using namespace loader::RelocationTypes::Arm;
  switch (r.type) {
    default:
      CARGO_ASSERT(0, "Unsupported relocation type.");
      return false;
    case R_ARM_NONE:
      break;
    // PC-relative 31-bit relocation
    case R_ARM_PREL31:
      cargo::write_little_endian(
          setBitRange(value, symbol_target_address - relocation_target_address,
                      0, 31),
          relocation_address);
      break;
    // absolute 32-bit relocation
    case R_ARM_TARGET1:
    case R_ARM_ABS32:
      uint32_t implicit_addend;
      cargo::read_little_endian(&implicit_addend, relocation_address);
      cargo::write_little_endian(symbol_target_address + implicit_addend,
                                 relocation_address);
      break;
    // absolute 16-bit relocations used to store the high and low 16 bits of a
    // 32-bit address
    case R_ARM_MOVW_ABS_NC:
    case R_ARM_MOVT_ABS: {
      const uint32_t bits = (r.type == R_ARM_MOVW_ABS_NC)
                                ? getBitRange(symbol_target_address, 0, 16)
                                : getBitRange(symbol_target_address, 16, 16);
      value = setBitRange(value, bits >> 12, 16, 4);
      value = setBitRange(value, bits, 0, 12);
      cargo::write_little_endian(value, relocation_address);
      break;
    }
    // A 24-bit PC+8-relative branch relocation requiring stub generation
    case R_ARM_PC24:
    case R_ARM_CALL:
    case R_ARM_JUMP24: {
      const uint32_t stub_target = getOrCreateStub();
      int32_t relative_value =
          static_cast<int32_t>(stub_target - relocation_target_address - 8);
      // ARM branch target encoding: 24 bits to store a 4-byte-granular address
      relative_value = (relative_value & 0x03FFFFFC) >> 2;
      // Ensure the implicit addend is -4, which is what LLVM always generates
      if ((value & 0xFFFFFF) != 0xFFFFFE) {
        return false;
      }
      cargo::write_little_endian(
          setBitRange(value, static_cast<uint32_t>(relative_value), 0, 24),
          relocation_address);
      break;
    }
  }
  return true;
}

// assumes little-endian AArch64, because big-endian is extremely rare
bool resolveAArch64(const loader::Relocation &r, loader::ElfMap &map,
                    loader::Relocation::StubMap &stubs) {
#ifndef NDEBUG
  auto symbol_name =
      map.getSymbolName(r.symbol_index).value_or("<unknown symbol>");
#endif

  auto relocation_data = decomposeRelocation<uint64_t>(r, map);
  if (!relocation_data) {
    return false;
  }
  auto [relocation_address, relocation_target_address, symbol_target_address] =
      *relocation_data;

  uint64_t value;
  cargo::read_little_endian(&value, relocation_address);
  uint32_t value32;
  cargo::read_little_endian(&value32, relocation_address);

  // returns the address of the stub to jump to
  auto getOrCreateStub = [&](uint64_t symbol_target_address) -> uint64_t {
    auto found_stub_target = stubs.getTarget(symbol_target_address);
    if (found_stub_target) {
      return *found_stub_target;
    }
    auto remaining = map.getRemainingStubSpace(r.section_index);
    if (!remaining || remaining->size() < 4) {
      return 0;
    }

    // Generates four moves of the four 16-bit parts of the target address into
    // ip0 (the assembler temporary register, which is always free to use),
    // followed by a branch on ip0. This stub has a total size of 20 bytes.

    // movz ip0, #:abs_g3:<addr>
    cargo::write_little_endian(setBitRange(uint32_t{0xD2E00010},
                                           static_cast<uint32_t>(getBitRange(
                                               symbol_target_address, 48, 16)),
                                           5, 16),
                               remaining->begin());
    // movk ip0, #:abs_g2_nc:<addr>
    cargo::write_little_endian(setBitRange(uint32_t{0xF2C00010},
                                           static_cast<uint32_t>(getBitRange(
                                               symbol_target_address, 32, 16)),
                                           5, 16),
                               remaining->begin() + 4);
    // movk ip0, #:abs_g1_nc:<addr>
    cargo::write_little_endian(setBitRange(uint32_t{0xF2A00010},
                                           static_cast<uint32_t>(getBitRange(
                                               symbol_target_address, 16, 16)),
                                           5, 16),
                               remaining->begin() + 8);
    // movk ip0, #:abs_g0_nc:<addr>
    cargo::write_little_endian(
        setBitRange(uint32_t{0xF2800010},
                    static_cast<uint32_t>(symbol_target_address & 0xFFFF), 5,
                    16),
        remaining->begin() + 12);
    // br ip0
    cargo::write_little_endian(uint32_t{0xD61F0200}, remaining->begin() + 16);
    auto target = *map.getStubTargetAddress(r.section_index);
    map.shrinkRemainingStubSpace(r.section_index, 20);
    return target + 4;
  };

  using namespace loader::RelocationTypes::AArch64;
  switch (r.type) {
    default:
      CARGO_ASSERT(0, "Unsupported relocation type.");
      return false;
    case R_AARCH64_NONE:
      break;
    // absolute 16-bit relocation asserting validity of sign-extension
    case R_AARCH64_ABS16: {
      const uint64_t real_value = symbol_target_address;
      const uint16_t trunc_value = real_value & 0xFFFF;
      if (static_cast<int16_t>(trunc_value) !=
          static_cast<int64_t>(real_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    // absolute 32-bit relocation asserting validity of sign-extension
    case R_AARCH64_ABS32: {
      const uint64_t real_value = symbol_target_address;
      const uint32_t trunc_value = real_value & 0xFFFFFFFF;
      if (static_cast<int32_t>(trunc_value) !=
          static_cast<int64_t>(real_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    // absolute 64-bit relocation
    case R_AARCH64_ABS64:
      cargo::write_little_endian(symbol_target_address, relocation_address);
      break;
    // PC-relative 64-bit relocation
    case R_AARCH64_PREL64:
      cargo::write_little_endian(
          symbol_target_address - relocation_target_address,
          relocation_address);
      break;
    // PC-relative 32-/16-bit relocations asserting sign-extension validity
    case R_AARCH64_PREL32: {
      const uint64_t real_value =
          symbol_target_address - relocation_target_address;
      const uint32_t trunc_value = real_value & 0xFFFFFFFF;
      if (static_cast<int32_t>(trunc_value) !=
          static_cast<int64_t>(real_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    case R_AARCH64_PREL16: {
      const uint64_t real_value =
          symbol_target_address - relocation_target_address;
      const uint16_t trunc_value = real_value & 0xFFFF;
      if (static_cast<int16_t>(trunc_value) !=
          static_cast<int64_t>(real_value)) {
        return false;
      }
      cargo::write_little_endian(trunc_value, relocation_address);
      break;
    }
    // PC-relative branch relocation
    case R_AARCH64_CALL26:
    case R_AARCH64_JUMP26: {
      // first try relocating a short branch (only possible if the jump is
      // within +/- 128MiB)
      uint64_t relative_value =
          symbol_target_address - relocation_target_address;
      if (static_cast<int64_t>(relative_value) >= -(1LL << 27) &&
          static_cast<int64_t>(relative_value) < (1LL << 27)) {
        // The jump is stored as 26-bit signed integer. It can jump to 4B
        // boundaries, hence the `>> 2u`.
        const uint32_t imm26 = (relative_value >> 2u) & 0x03FFFFFF;
        value32 |= imm26;
        cargo::write_little_endian(value32, relocation_address);
        break;
      }
      // If that fails because the target is too far, generate a stub (a small
      // section of code that sets up an absolute jump to a 64-bit location)
      const uint64_t stub_target = getOrCreateStub(symbol_target_address);
      if (0LU == stub_target) {
#ifndef NDEBUG
        (void)fprintf(
            stderr,
            "Out of stub space when constructing linker veneer for: %.*s\n",
            (int)symbol_name.size(), symbol_name.data());
#endif
        return false;
      }
      relative_value = stub_target - relocation_target_address;
      // If the stub is too far to jump to, then give up
      if (static_cast<int64_t>(relative_value) < -(1LL << 27) ||
          static_cast<int64_t>(relative_value) >= (1LL << 27)) {
#ifndef NDEBUG
        (void)fprintf(
            stderr,
            "Linker veneer for symbol %.*s beyond addressable span of "
            "branch instruction",
            (int)symbol_name.size(), symbol_name.data());
#endif
        return false;
      }
      // Jump to the stub
      const uint32_t imm26 = (relative_value >> 2u) & 0x03FFFFFF;
      value32 |= imm26;
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    // absolute relocations for 16-bit immediate move instructions for 4 parts
    // of a 64-bit address
    case R_AARCH64_MOVW_UABS_G0_NC:
      cargo::write_little_endian(
          setBitRange(value32, static_cast<uint32_t>(symbol_target_address), 5,
                      16),
          relocation_address);
      break;
    case R_AARCH64_MOVW_UABS_G1_NC:
      cargo::write_little_endian(
          setBitRange(
              value32,
              static_cast<uint32_t>(getBitRange(symbol_target_address, 16, 16)),
              5, 16),
          relocation_address);
      break;
    case R_AARCH64_MOVW_UABS_G2_NC:
      cargo::write_little_endian(
          setBitRange(
              value32,
              static_cast<uint32_t>(getBitRange(symbol_target_address, 32, 16)),
              5, 16),
          relocation_address);
      break;
    case R_AARCH64_MOVW_UABS_G3:
      cargo::write_little_endian(
          setBitRange(
              value32,
              static_cast<uint32_t>(getBitRange(symbol_target_address, 48, 16)),
              5, 16),
          relocation_address);
      break;
    // PC-relative page-granular 21-bit relocation
    // From ELF for the Arm® 64-bit Architecture (AArch64) 2023Q3:
    // "Set an ADRP immediate value to bits [32:12] of [the result of the
    // relocation operation] X; check that -2^32 <= X < 2^32"
    case R_AARCH64_ADR_PREL_PG_HI21: {
      uint64_t page_difference = (symbol_target_address & ~0xFFFULL) -
                                 (relocation_target_address & ~0xFFFULL);
      if (static_cast<int64_t>(page_difference) >= (1LL << 32) ||
          static_cast<int64_t>(page_difference) < -(1LL << 32)) {
        return false;
      }
      page_difference >>= 12;
      value32 = setBitRange(
          value32, getBitRange(static_cast<uint32_t>(page_difference), 0, 2),
          29, 2);
      value32 = setBitRange(
          value32, getBitRange(static_cast<uint32_t>(page_difference), 2, 19),
          5, 19);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    // LD/ST immediate value relocations for bits [0/1/2/3/4; 11]
    case R_AARCH64_ADD_ABS_LO12_NC:
    case R_AARCH64_LDST8_ABS_LO12_NC: {
      value32 = setBitRange(
          value32,
          static_cast<uint32_t>(getBitRange(symbol_target_address, 0, 12)), 10,
          12);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    case R_AARCH64_LDST16_ABS_LO12_NC: {
      value32 = setBitRange(
          value32,
          static_cast<uint32_t>(getBitRange(symbol_target_address, 1, 11)), 10,
          12);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    case R_AARCH64_LDST32_ABS_LO12_NC: {
      value32 = setBitRange(
          value32,
          static_cast<uint32_t>(getBitRange(symbol_target_address, 2, 10)), 10,
          12);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    case R_AARCH64_LDST64_ABS_LO12_NC: {
      value32 = setBitRange(
          value32,
          static_cast<uint32_t>(getBitRange(symbol_target_address, 3, 9)), 10,
          12);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
    case R_AARCH64_LDST128_ABS_LO12_NC: {
      value32 = setBitRange(
          value32,
          static_cast<uint32_t>(getBitRange(symbol_target_address, 4, 8)), 10,
          12);
      cargo::write_little_endian(value32, relocation_address);
      break;
    }
  }
  return true;
}

// Sets the immediate field of an I-type instruction [31:20]
void writeITypeImm(uint32_t imm, uint8_t *insn_addr) {
  uint32_t insn;
  cargo::read_little_endian(&insn, insn_addr);
  insn = setBitRange(insn, imm, 20, 12);
  cargo::write_little_endian(insn, insn_addr);
}

// Sets the immediate field of an S-type instruction [31:25, 11:7]
void writeSTypeImm(uint32_t imm, uint8_t *insn_addr) {
  uint32_t insn;
  cargo::read_little_endian(&insn, insn_addr);
  insn = setBitRange(insn, imm >> 5, 25, 7);
  insn = setBitRange(insn, imm, 7, 5);
  cargo::write_little_endian(insn, insn_addr);
}

// Sets the immediate field of an U-type instruction [31:12]
void writeUTypeImm(uint32_t imm, uint8_t *insn_addr) {
  uint32_t insn;
  cargo::read_little_endian(&insn, insn_addr);
  insn = setBitRange(insn, imm, 12, 20);
  cargo::write_little_endian(insn, insn_addr);
}

uint32_t getLo12(uint64_t addr) {
  return static_cast<uint32_t>(getBitRange(addr, 0, 12));
}

// Sign extends a number in the bottom 'b' bits of a 64-bit number back up to
// a 64-bit number.
int64_t signExtendN(uint64_t val, unsigned b) {
  CARGO_ASSERT(b > 0, "bit-width cannot be zero");
  CARGO_ASSERT(b <= 64, "bit-width out of range");
  return static_cast<int64_t>(val << (64 - b)) >> (64 - b);
}

cargo::optional<uint32_t> getHi20(int64_t addr) {
  // See documentation for why we add 0x800 here:
  // https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc#absolute-addresses
  // https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc#pc-relative-symbol-addresses
  addr += 0x800;
  // Check that the value, when sign-extended back up to a 64-bit number (e.g.,
  // through LUI) will not lose bits.
  if ((addr >> 12) != signExtendN(addr >> 12, 20)) {
    (void)fprintf(
        stderr,
        "Value %" PRId64 " is out of the range of a 20-bit immediate.\n", addr);
    return cargo::nullopt;
  }
  return static_cast<uint32_t>(getBitRange(addr, 12, 20));
}

// assumes little-endian RISCV
bool resolveRISCV(const loader::Relocation &r, loader::ElfMap &map,
                  const std::vector<loader::Relocation> &relocations) {
  // The ALIGN relocation is special and is related to keeping alignment with
  // nops after linking. Currently skip this as we believe this is not required.
  // Additionally it has no symbol which decomposeRelocation does not handle, so
  // this would need fixed if this is ever implemented.
  if (r.type == loader::RelocationTypes::RISCV::R_RISCV_ALIGN) {
    // TODO: Check that we fit the alignment constraints as is.
    return true;
  }
  const auto relocation_data = decomposeRelocation<uint64_t>(r, map);
  if (!relocation_data) {
    return false;
  }
  auto [relocation_address, relocation_target_address, symbol_target_address] =
      *relocation_data;

  // A PC-relative offset from the position of the relocation (S + A - P)
  const uint64_t relative_value =
      symbol_target_address - relocation_target_address;

  auto getPCRelHi20 =
      [&relocations](const loader::Relocation &r,
                     int64_t offset) -> cargo::optional<loader::Relocation> {
    auto label_offset = r.offset + offset;
    // Search for the matching relocation in the same section, with the correct
    // offset and the PCREL_HI20 type.
    for (auto &other_r : relocations) {
      if (r.section_index == other_r.section_index &&
          label_offset == other_r.offset &&
          other_r.type == loader::RelocationTypes::RISCV::R_RISCV_PCREL_HI20) {
        return other_r;
      }
    }
    return cargo::nullopt;
  };

  using namespace loader::RelocationTypes::RISCV;
  switch (r.type) {
    default: {
#ifndef NDEBUG
      auto symbol_name =
          map.getSymbolName(r.symbol_index).value_or("<unknown symbol>");
      (void)fprintf(stderr, "Missing symbol: %.*s\n", (int)symbol_name.size(),
                    symbol_name.data());
      (void)fprintf(stderr, "   with relocation type: %u\n", r.type);
#endif
      CARGO_ASSERT(0, "Unsupported relocation type.");
      return false;
    }
    case R_RISCV_64:
      // 64-bit relocation: S + A
      cargo::write_little_endian(static_cast<uint32_t>(symbol_target_address),
                                 relocation_address);
      cargo::write_little_endian(
          static_cast<uint32_t>(symbol_target_address >> 32),
          relocation_address + 4);
      break;
    case R_RISCV_PCREL_HI20: {
      // High 20 bits of 32-bit PC-relative reference -- S + A - P -- in the
      // immediate field of an U-type instruction
      auto rel_hi20 = getHi20(relative_value);
      if (!rel_hi20) {
        return false;
      }
      writeUTypeImm(*rel_hi20, relocation_address);
      break;
    }
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S: {
      // Low 12 bits of 32-bit PC-relative reference -- S + A - P (A must be 0)
      // -- in the immediate field of an I-type or an S-type instruction
      CARGO_ASSERT(r.addend == 0, "Invalid PCREL_LO12 relocation");
      // This relocation points to a label, for which there is a corresponding
      // PCREL_HI20 relocation pointing to the actual symbol. Find that
      // relocation now.
      auto hi20_reloc = getPCRelHi20(r, static_cast<int64_t>(relative_value));
      if (!hi20_reloc) {
        (void)fprintf(stderr,
                      "Could not retrieve corresponding PCREL_HI20 relocation "
                      "for PCREL_LO12 relocation.\n");
        return false;
      }
      const auto hi20_reloc_data =
          decomposeRelocation<uint64_t>(*hi20_reloc, map);
      if (!hi20_reloc_data) {
        return false;
      }
      auto [_, hi20_reloc_target_address, hi20_symbol_target_address] =
          *hi20_reloc_data;
      const uint64_t hi20_relative_value =
          hi20_symbol_target_address - hi20_reloc_target_address;
      switch (r.type) {
        case R_RISCV_PCREL_LO12_I:
          writeITypeImm(getLo12(hi20_relative_value), relocation_address);
          break;
        case R_RISCV_PCREL_LO12_S:
          writeSTypeImm(getLo12(hi20_relative_value), relocation_address);
          break;
        default:
          CARGO_ASSERT(0, "Unexpected relocation type.");
      }
      break;
    }
    case R_RISCV_HI20: {
      // High 20 bits of 32-bit absolute address -- S + A -- in the immediate
      // field of an U-type instruction
      auto abs_hi20 = getHi20(symbol_target_address);
      if (!abs_hi20) {
        return false;
      }
      writeUTypeImm(*abs_hi20, relocation_address);
      break;
    }
    case R_RISCV_LO12_I:
      // Low 12 bits of 32-bit absolute address -- S + A -- in the immediate
      // field of an I-type instruction
      writeITypeImm(getLo12(symbol_target_address), relocation_address);
      break;
    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT: {
      // 32-bit PC-relative function call -- S + A - P -- in the immediate
      // fields of a U-type and I-type instruction pair (e.g., 'call' or 'tail'
      // macros).
      auto rel_hi20 = getHi20(relative_value);
      if (!rel_hi20) {
        return false;
      }
      writeUTypeImm(*rel_hi20, relocation_address);
      writeITypeImm(getLo12(relative_value), relocation_address + 4);
      break;
    }
    case R_RISCV_BRANCH: {
      // R_RISCV_BRANCH immediate is for B-Type instructions and is split
      // across 12 bits spread across the instruction and skips the bottom bit
      // of the input relative offset. See conditional branches in the isa spec.

      // Check that the value, when sign-extended back up to a 64-bit number
      // will not lose bits.
      if (signExtendN(relative_value, 13) !=
          static_cast<int64_t>(relative_value & (~1))) {
#ifndef NDEBUG
        auto name =
            map.getSymbolName(r.symbol_index).value_or("<unknown symbol>");
        (void)fprintf(stderr,
                      "Error: relocation R_RISCV_BRANCH branch > 13 bits or "
                      "not even for symbol '%.*s'\n",
                      (int)name.size(), name.data());
#endif
        return false;
      }
      uint32_t value;
      const uint32_t trunc_value = static_cast<uint32_t>(relative_value);
      cargo::read_little_endian(&value, relocation_address);
      // imm bits 1-4 at bit 8
      value = setBitRange(value, trunc_value >> 1, 8, 4);
      // imm bits 5-10 at bit 25
      value = setBitRange(value, trunc_value >> 5, 25, 6);
      // imm bit 11 at bit 7
      value = setBitRange(value, trunc_value >> 11, 7, 1);
      // imm bit 12 at bit 31
      value = setBitRange(value, trunc_value >> 12, 31, 1);
      cargo::write_little_endian(value, relocation_address);
      break;
    }

    case R_RISCV_RVC_JUMP: {
      // R_RISCV_RVC_JUMP is for compressed instructions and fits in bits 2-12
      // of a 16 bit value and skips the bottom bit of the input. See CJ format
      // in the isa spec.

      // Check that the value, when sign-extended back up to a 64-bit number
      // will not lose bits.
      if (signExtendN(relative_value, 12) !=
          static_cast<int64_t>(relative_value & (~1))) {
#ifndef NDEBUG
        auto name =
            map.getSymbolName(r.symbol_index).value_or("<unknown symbol>");
        (void)fprintf(stderr,
                      "Error: relocation R_RISCV_RVC_JUMP > 12 bits or not "
                      "even for symbol '%.*s'\n",
                      (int)name.size(), name.data());
#endif
        return false;
      }

      uint16_t value;
      cargo::read_little_endian(&value, relocation_address);
      const uint16_t offset_16 = static_cast<uint16_t>(relative_value) >> 1;
      value = setBitRange(value, offset_16, 2, 11);
      cargo::write_little_endian(value, relocation_address);
      break;
    }

    case R_RISCV_ADD32: {
      // 32-bit label addition: V + S + A
      uint32_t value;
      cargo::read_little_endian(&value, relocation_address);
      cargo::write_little_endian(
          value + static_cast<uint32_t>(symbol_target_address),
          relocation_address);
      break;
    }
    case R_RISCV_SUB32: {
      // 32-bit label subtraction: V - (S + A)
      uint32_t value;
      cargo::read_little_endian(&value, relocation_address);
      cargo::write_little_endian(
          value - static_cast<uint32_t>(symbol_target_address),
          relocation_address);
      break;
    }
  }

  return true;
}

}  // namespace

template <loader::Relocation::EntryType ET32,
          loader::Relocation::EntryType ET64>
std::vector<loader::Relocation> loader::collectSectionRelocations(
    ElfFile &file, ElfMap &map, const loader::ElfFile::Section &section,
    cargo::string_view prefix, ElfFields::SectionType type) {
  if (!section.name().starts_with(prefix) || section.type() != type) {
    return {};
  }
  // xxx.text -> .text
  auto sname = section.name().substr(prefix.size());
  if (!sname) {
    return {};
  }
  auto section_id = file.section(*sname).map(&ElfFile::Section::index);
  if (!section_id) {
    return {};
  }
  if (!map.getSectionTargetAddress(section_id.value())) {
    return {};
  }
  std::vector<loader::Relocation> relocations;
  for (auto *it = section.data().begin(); it != section.data().end();
       it += section.entrySize()) {
    if (file.is32Bit()) {
      relocations.push_back(
          Relocation::fromElfEntry<ET32>(file, *section_id, it));
    } else {
      relocations.push_back(
          Relocation::fromElfEntry<ET64>(file, *section_id, it));
    }
  }
  return relocations;
}

bool loader::resolveRelocations(loader::ElfFile &file, loader::ElfMap &map) {
  std::vector<Relocation> relocations;

  for (auto it = file.sectionsBegin(); it != file.sectionsEnd(); ++it) {
    auto section = *it;
    // if a section has no entries, it cannot hold relocations
    if (!section.entrySize()) {
      continue;
    }
    {
      // Explicit addends
      auto relas = collectSectionRelocations<Relocation::EntryType::Elf32RelA,
                                             Relocation::EntryType::Elf64RelA>(
          file, map, section, ".rela", ElfFields::SectionType::RELA);
      relocations.insert(relocations.end(), std::begin(relas), std::end(relas));
    }

    {
      // No explicit addends (implicits may be present depending on
      // architecture)
      auto rels = collectSectionRelocations<Relocation::EntryType::Elf32Rel,
                                            Relocation::EntryType::Elf64Rel>(
          file, map, section, ".rel", ElfFields::SectionType::REL);
      relocations.insert(relocations.end(), std::begin(rels), std::end(rels));
    }
  }

  loader::Relocation::StubMap stubs;
  cargo::optional<int32_t> curr_section_index;
  for (auto &r : relocations) {
    if (curr_section_index != r.section_index) {
      // Reset the StubMap when entering a new section
      stubs.reset();
      curr_section_index = r.section_index;
    }
    if (!r.resolve(file, map, stubs, relocations)) {
      return false;
    }
  }

  return true;
}

bool loader::Relocation::resolve(
    loader::ElfFile &file, loader::ElfMap &map,
    loader::Relocation::StubMap &stubs,
    const std::vector<loader::Relocation> &relocations) {
  switch (file.machine()) {
    case ElfFields::Machine::X86:
      return resolveX86_32(*this, map);
    case ElfFields::Machine::X86_64:
      return resolveX86_64(*this, map);
    case ElfFields::Machine::ARM:
      return resolveArm(*this, map, stubs);
    case ElfFields::Machine::AARCH64:
      return resolveAArch64(*this, map, stubs);
    case ElfFields::Machine::RISCV:
      return resolveRISCV(*this, map, relocations);
    default:
      CARGO_ASSERT(0, "Unrecognised ELF machine.");
  }
  return false;
}

template <>
loader::Relocation
loader::Relocation::fromElfEntry<loader::Relocation::EntryType::Elf32Rel>(
    loader::ElfFile &file, uint32_t section_id, const void *raw_entry) {
  struct Entry {
    uint32_t offset;
    uint32_t info;
  };
  auto entry = reinterpret_cast<const Entry *>(raw_entry);
  Relocation r;
  r.type = file.field(entry->info) & 0xFF;
  r.symbol_index = file.field(entry->info) >> 8;
  r.offset = file.field(entry->offset);
  r.addend = 0;
  r.section_index = section_id;
  return r;
}
template <>
loader::Relocation
loader::Relocation::fromElfEntry<loader::Relocation::EntryType::Elf32RelA>(
    loader::ElfFile &file, uint32_t section_id, const void *raw_entry) {
  struct Entry {
    uint32_t offset;
    uint32_t info;
    uint32_t addend;
  };
  auto entry = reinterpret_cast<const Entry *>(raw_entry);
  Relocation r;
  r.type = file.field(entry->info) & 0xFF;
  r.symbol_index = file.field(entry->info) >> 8;
  r.offset = file.field(entry->offset);
  r.addend = static_cast<int32_t>(file.field(entry->addend));
  r.section_index = section_id;
  return r;
}
template <>
loader::Relocation
loader::Relocation::fromElfEntry<loader::Relocation::EntryType::Elf64Rel>(
    loader::ElfFile &file, uint32_t section_id, const void *raw_entry) {
  struct Entry {
    uint64_t offset;
    uint64_t info;
  };
  auto entry = reinterpret_cast<const Entry *>(raw_entry);
  Relocation r;
  r.type = file.field(entry->info) & 0xFFFFFFFF;
  r.symbol_index = file.field(entry->info) >> 32;
  r.offset = file.field(entry->offset);
  r.addend = 0;
  r.section_index = section_id;
  return r;
}
template <>
loader::Relocation
loader::Relocation::fromElfEntry<loader::Relocation::EntryType::Elf64RelA>(
    loader::ElfFile &file, uint32_t section_id, const void *raw_entry) {
  struct Entry {
    uint64_t offset;
    uint64_t info;
    uint64_t addend;
  };
  auto entry = reinterpret_cast<const Entry *>(raw_entry);
  Relocation r;
  r.type = file.field(entry->info) & 0xFFFFFFFF;
  r.symbol_index = file.field(entry->info) >> 32;
  r.offset = file.field(entry->offset);
  r.addend = static_cast<int64_t>(file.field(entry->addend));
  r.section_index = section_id;
  return r;
}
