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

#include <cargo/string_algorithm.h>
#include <host/device.h>
#include <host/executable.h>
#include <host/host.h>
#include <host/metadata_hooks.h>
#include <host/utils/jit_kernel.h>
#include <host/utils/relocations.h>
#include <loader/relocations.h>
#include <mux/utils/allocator.h>
#include <utils/system.h>

#include <memory>
#include <new>

host::executable_s::executable_s(mux_device_t device,
                                 utils::jit_kernel_s kernel,
                                 mux::allocator allocator)
    : jit_kernel_name(kernel.name),
      elf_contents(allocator),
      allocated_pages(allocator) {
  this->device = device;
  kernels.emplace(jit_kernel_name,
                  std::vector<binary_kernel_s>(
                      {{kernel.hook, kernel.name, kernel.local_memory_used,
                        kernel.min_work_width, kernel.pref_work_width,
                        kernel.sub_group_size}}));
}

host::executable_s::executable_s(
    mux_device_t device, mux::dynamic_array<uint64_t> elf_contents,
    mux::small_vector<loader::PageRange, 4> allocated_pages,
    kernel_variant_map binary_kernels)
    : elf_contents(std::move(elf_contents)),
      allocated_pages(std::move(allocated_pages)),
      kernels(std::move(binary_kernels)) {
  this->device = device;
}

mux_result_t hostCreateExecutable(mux_device_t device, const void *binary,
                                  uint64_t binary_length,
                                  mux_allocator_info_t allocator_info,
                                  mux_executable_t *out_executable) {
  mux::allocator allocator(allocator_info);

  // If we're passing through a JIT compiled kernel.
  if (host::utils::isJITKernel(binary, binary_length)) {
    cargo::optional<host::utils::jit_kernel_s> jit_kernel =
        host::utils::deserializeJITKernel(binary, binary_length);
    if (!jit_kernel) {
      return mux_error_invalid_binary;
    }

    auto executable = allocator.create<host::executable_s>(
        device, std::move(*jit_kernel), allocator);
    if (nullptr == executable) {
      return mux_error_out_of_memory;
    }

    *out_executable = executable;
    return mux_success;
  }

  mux::dynamic_array<uint64_t> elf_contents{allocator};
  if (elf_contents.alloc((binary_length / sizeof(uint64_t)) + 1)) {
    return mux_error_out_of_memory;
  }
  cargo::array_view<uint8_t> elf_bytes{
      reinterpret_cast<uint8_t *>(elf_contents.data()),
      static_cast<size_t>(binary_length)};
  std::copy_n(reinterpret_cast<const uint8_t *>(binary),
              static_cast<size_t>(binary_length), elf_bytes.begin());
  host::kernel_variant_map kernels;

  if (!loader::ElfFile::isValidElf(elf_bytes)) {
    return mux_error_invalid_binary;
  }
  std::unique_ptr<loader::ElfFile> elf_file{new loader::ElfFile(elf_bytes)};

  auto parsed_kernels = host::readBinaryMetadata(elf_file.get(), &allocator);
  if (parsed_kernels) {
    kernels = std::move(parsed_kernels.take().value());
  } else {
    return mux_error_invalid_binary;
  }

  mux::small_vector<loader::PageRange, 4> allocated_pages{allocator};
  mux::small_vector<loader::MemoryProtection, 4> page_protection{allocator};
  loader::ElfMap elf_map{elf_file.get()};

  // load
  for (auto &section : elf_file->sections()) {
    if (!(section.flags() & loader::ElfFields::SectionFlags::ALLOC)) {
      continue;
    }
    if (section.name().data() == host::MD_NOTES_SECTION) {
      continue;
    }

    // We map the section whether it has a non-zero size or not, but we only
    // allocate and protect pages if the size is greater than 0.
    if (section.sizeToAlloc() > 0) {
      if (allocated_pages.emplace_back()) {
        return mux_error_out_of_memory;
      }
      if (page_protection.emplace_back()) {
        return mux_error_out_of_memory;
      }
      if (allocated_pages.back().allocate(section.sizeToAlloc())) {
        return mux_error_out_of_memory;
      }
      page_protection.back() = loader::getSectionProtection(section);
      if (section.type() != loader::ElfFields::SectionType::NOBITS) {
        std::copy(section.data().begin(), section.data().end(),
                  allocated_pages.back().data().begin());
      }
      uint8_t *dataptr = allocated_pages.back().data().data();
      if (elf_map.addSectionMapping(section, dataptr,
                                    allocated_pages.back().data().end(),
                                    reinterpret_cast<uint64_t>(dataptr))) {
        return mux_error_out_of_memory;
      }
    } else {
      if (elf_map.addSectionMapping(section, nullptr, nullptr, 0)) {
        return mux_error_out_of_memory;
      }
    }
  }

  // Populate elf_map with all callbacks so the kernel can call those
  // functions
  for (const auto &reloc : host::utils::getRelocations()) {
    if (elf_map.addCallback(reloc.first, reloc.second)) {
      return mux_error_out_of_memory;
    }
  }

  // relocate
  // If this is failing (especially on Arm32), it may be that a required
  // callback isn't getting added. See the `elf_map.addCallback()`s above.
  // Callbacks are resolved in `loader::ElfMap::getSymbolTargetAddress()`.
  if (!loader::resolveRelocations(*elf_file, elf_map)) {
    return mux_error_internal;
  }

  // protect
  for (size_t i = 0; i < page_protection.size(); i++) {
    allocated_pages[i].protect(page_protection[i]);
  }

  // set hooks
  for (auto &p : kernels) {
    for (auto &variant : p.second) {
      auto hook = elf_map.getSymbolTargetAddress(
          {variant.kernel_name.data(), variant.kernel_name.size()});
      if (!hook) {
        return mux_error_invalid_binary;
      }
      variant.hook = *hook;
    }
  }

  auto executable = allocator.create<host::executable_s>(
      device, std::move(elf_contents), std::move(allocated_pages),
      std::move(kernels));
  if (nullptr == executable) {
    return mux_error_out_of_memory;
  }

  *out_executable = executable;
  return mux_success;
}

void hostDestroyExecutable(mux_device_t device, mux_executable_t executable,
                           mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);
  auto hostExecutable = static_cast<host::executable_s *>(executable);
  allocator.destroy(hostExecutable);
}
