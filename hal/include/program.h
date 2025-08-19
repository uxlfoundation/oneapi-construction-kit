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

/// @file
///
/// @brief A utility implementation of hal_program_t
///
/// The hal_program_t type is intentionally opaque to allow a HAL to represent
/// a program internally however they need to. This file provides a utility
/// class which contains a convenient implementation of a hal_program_t for
/// use by a HAL.
/// Note, currently this code works only with RV64 ELF files but could be
/// revised in the future.

#ifndef HAL_PROGRAM_H_INCLUDED
#define HAL_PROGRAM_H_INCLUDED

#include <cstdint>
#include <map>
#include <memory>

#include "elf.h"
#include "hal.h"

namespace hal {
/// @addtogroup hal
/// @{
namespace util {
/// @addtogroup util
/// @{

struct hal_program_impl_t {
  /// @brief Load an ELF file for retention by this hal_program_impl_t.
  ///
  /// Load performs a copy operation so the parameters can be discarded as
  /// needed after this call.
  ///
  /// @param data is the raw ELF file data.
  /// @param size is size in bytes of the data parameter.
  ///
  /// @return Returns true if the ELF file was loaded correctly.
  bool load(const uint8_t *data, size_t size);

  /// @brief Load a program into device memory.
  ///
  /// @param dev is the device to load this program into.
  ///
  /// @return Returns true if upload was successful otherwise false.
  bool upload(hal_device_t *dev);

  /// @brief Load the given ELF section into device memory.
  ///
  /// @param dev is the device to load this program into.
  /// @param data is the pointer to the beginning of the ELF data
  /// @param to_copy is the number of bytes to copy.
  /// @param phdr is the program header for this section.
  ///
  /// @return Returns true if upload was successful otherwise false.
  bool upload_section_copy(hal_device_t *dev, const uint8_t *data,
                           Elf64_XWord_t to_copy, Elf_Phdr_wrapper_t phdr);

  /// @brief Initialize the given ELF section in device memory by zeroing it.
  ///
  /// @param dev is the device to load this program into.
  /// @param offset is the offset into the section to begin zeroing
  /// @param to_zero is the number of bytes to initialize.
  /// @param phdr is the program header for this section.
  ///
  /// @return Returns true if upload was successful otherwise false.
  bool upload_section_zero(hal_device_t *dev, Elf64_XWord_t offset,
                           Elf64_XWord_t to_zero, Elf_Phdr_wrapper_t phdr);

  /// @brief Lookup a symbol address in this elf file.
  ///
  /// @param name is a c-string with the name of the symbol to find.
  ///
  /// @return Returns address of symbol or hal_nullptr.
  hal_addr_t find_symbol(const char *name) const {
    auto itt = symbols.find(name);
    return (itt == symbols.end()) ? hal_nullptr : itt->second;
  }

  /// @brief Lookup a symbol name from its address.
  ///
  /// @param addr is a device address for this symbol.
  /// @param kernel_name is the output parameter for the kernel name.
  ///
  /// @return Returns true if the symbol can be located.
  bool find_symbol(hal::hal_addr_t addr, std::string &kernel_name) const {
    for (const auto &k : symbols) {
      if (k.second == addr) {
        kernel_name = k.first;
        return true;
      }
    }
    return false;
  }

  /// @brief Check if this object encapsulates a valid program.
  ///
  /// @return true if a program has been loaded and is valid, else false.
  bool is_valid() const { return bool(data) && size > 0; }

  /// @brief Discard all data related to the currently loaded program.
  void unload() {
    data.reset();
    size = 0;
    symbols.clear();
  }

 protected:
  /// @brief Populate the symbol map from a given ELF file.
  ///
  /// @param elf is an ELF file to fill the symbol map from.
  ///
  /// @return Returns true on success otherwise false.
  bool populate_kernel_map(const elf64_file_t &elf);

  /// @brief Raw data of the ELF file.
  std::unique_ptr<uint8_t[]> data;

  /// @brief Size in bytes of the ELF file data.
  size_t size;

  /// @brief A mapping of ELF functions to addresses.
  std::map<std::string, hal_addr_t> symbols;
};

/// @}
}  // namespace util
/// @}
}  // namespace hal

#endif  // HAL_PROGRAM_H_INCLUDED
