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
/// @brief Host virtual memory mapping utilities used for memory protection.

#ifndef LOADER_MAPPER_H_INCLUDED
#define LOADER_MAPPER_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/error.h>
#include <loader/elf.h>

namespace loader {

/// @brief Permissions that can be set on virtual memory ranges to protect them.
enum MemoryProtection : uint32_t {
  MEM_READABLE = 0x1,
  MEM_WRITABLE = 0x2,
  MEM_EXECUTABLE = 0x4,

  MEM_CODE = MEM_READABLE | MEM_EXECUTABLE,
  MEM_DATA = MEM_READABLE | MEM_WRITABLE,
  MEM_RODATA = MEM_READABLE
};

/// @brief Get the required memory protection for an ELF section.
MemoryProtection getSectionProtection(const ElfFile::Section &section);

/// @brief Get the size in bytes of an OS memory page.
size_t getPageSize();

/// @brief Wraps and owns a range of pages in virtual memory.
struct PageRange {
  PageRange();
  PageRange(const PageRange &) = delete;
  PageRange(PageRange &&);
  ~PageRange();
  PageRange &operator=(PageRange &&);

  /// @brief Allocates OS pages for at least @p bytes bytes of storage.
  /// The pages are allocated with read+write permissions and guaranteed to be
  /// filled with zeroes.
  ///
  /// @param bytes Number of bytes to allocate, must be greater than 0.
  cargo::result allocate(size_t bytes);

  /// @brief Changes the protection of the allocated memory pages.
  cargo::result protect(MemoryProtection protection);

  /// @brief Gets the allocated memory range.
  cargo::array_view<uint8_t> data() const { return {pages_begin, pages_end}; }

 private:
  uint8_t *pages_begin;
  uint8_t *pages_end;
};

}  // namespace loader

#endif
