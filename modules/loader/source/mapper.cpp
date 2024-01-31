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
#include <loader/mapper.h>

#include <algorithm>
#include <array>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef __MCOS_POSIX__
#include <emcos/emcos_device_info.h>
#endif

loader::MemoryProtection loader::getSectionProtection(
    const loader::ElfFile::Section &section) {
  MemoryProtection mp = MEM_READABLE;
  if (section.flags() & ElfFields::SectionFlags::Type::WRITE) {
    mp = static_cast<MemoryProtection>(mp | MEM_WRITABLE);
  }
  if (section.flags() & ElfFields::SectionFlags::Type::EXECINSTR) {
    mp = static_cast<MemoryProtection>(mp | MEM_EXECUTABLE);
  }
  return mp;
}

size_t loader::getPageSize() {
  static std::once_flag once;
  static size_t page_size;
  std::call_once(once, []() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = static_cast<size_t>(si.dwPageSize);
#elif defined(__linux__)
    page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
#elif defined(__MCOS_POSIX__)
        page_size = emcos::get_device_page_size();
#elif defined(__APPLE__)
        page_size = getpagesize();
#else
#error Unknown platform!
#endif
  });
  return page_size;
}

loader::PageRange::PageRange() : pages_begin(nullptr), pages_end(nullptr) {}

loader::PageRange::PageRange(PageRange &&rhs)
    : pages_begin(rhs.pages_begin), pages_end(rhs.pages_end) {
  rhs.pages_begin = nullptr;
  rhs.pages_end = nullptr;
}

loader::PageRange &loader::PageRange::operator=(loader::PageRange &&rhs) {
  if (this == &rhs) {
    return *this;
  }
  this->~PageRange();
  pages_begin = rhs.pages_begin;
  pages_end = rhs.pages_end;
  rhs.pages_begin = nullptr;
  rhs.pages_end = nullptr;
  return *this;
}

loader::PageRange::~PageRange() {
  if (pages_end != nullptr) {
#ifdef _WIN32
    // size must be zero, see MSDN docs for VirtualFree
    if (VirtualFree(reinterpret_cast<void *>(pages_begin), 0, MEM_RELEASE) ==
        0) {
      CARGO_ASSERT(0, "Failed to deallocate memory of a PageRange");
    }
#else
    if (munmap(pages_begin, pages_end - pages_begin) < 0) {
      CARGO_ASSERT(0, "Failed to deallocate memory of a PageRange");
    }
#endif
    pages_begin = nullptr;
    pages_end = nullptr;
  }
}

cargo::result loader::PageRange::allocate(size_t bytes) {
  if (0 == bytes) {
    return cargo::bad_argument;  // Can't map zero bytes.
  }
  if (pages_end != nullptr) {
    return cargo::bad_argument;
  }
  const size_t page_count = (bytes + getPageSize() - 1) / getPageSize();
  // round up to whole pages
  bytes = page_count * getPageSize();
#ifdef _WIN32
  pages_begin = reinterpret_cast<uint8_t *>(
      VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
  if (pages_begin == nullptr) {
    return cargo::bad_alloc;
  }
#else
  void *p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) {
    return cargo::bad_alloc;
  }
  pages_begin = reinterpret_cast<uint8_t *>(p);
#endif
  pages_end = pages_begin + bytes;
  return cargo::success;
}

cargo::result loader::PageRange::protect(MemoryProtection protection) {
  if (pages_end == nullptr) {
    return cargo::bad_argument;
  }
#ifdef _WIN32
  std::array<int, 8> vals;  // indexed by protection
  vals[0] = PAGE_NOACCESS;
  vals[MEM_READABLE] = PAGE_READONLY;
  vals[MEM_WRITABLE] = PAGE_READWRITE;  // no write-only variant
  vals[MEM_EXECUTABLE] = PAGE_EXECUTE;
  vals[MEM_READABLE | MEM_WRITABLE] = PAGE_READWRITE;
  vals[MEM_READABLE | MEM_EXECUTABLE] = PAGE_EXECUTE_READ;
  // no write+execute variant
  vals[MEM_WRITABLE | MEM_EXECUTABLE] = PAGE_EXECUTE_READWRITE;
  vals[MEM_READABLE | MEM_WRITABLE | MEM_EXECUTABLE] = PAGE_EXECUTE_READWRITE;
  DWORD oldProt;
  if (VirtualProtect(pages_begin, pages_end - pages_begin, vals[protection],
                     &oldProt) == 0) {
    return cargo::bad_alloc;
  }
#else
  int prot = 0;
  if (protection & MEM_READABLE) {
    prot |= PROT_READ;
  }
  if (protection & MEM_WRITABLE) {
    prot |= PROT_WRITE;
  }
  if (protection & MEM_EXECUTABLE) {
    prot |= PROT_EXEC;
  }
  if (mprotect(pages_begin, pages_end - pages_begin, prot) < 0) {
    return cargo::bad_alloc;
  }
#endif
  return cargo::success;
}
