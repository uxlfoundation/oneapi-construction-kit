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

#include "riscv/executable.h"

#include <metadata/detail/utils.h>
#include <metadata/handler/vectorize_info_metadata.h>

#include "loader/relocations.h"
#include "riscv/device.h"

namespace {

struct ElfUserdata {
  loader::ElfFile *elf;
  mux::allocator *allocator;
};

constexpr const char MD_NOTES_SECTION[] = "notes";

md_hooks getElfMetadataReadHooks() {
  md_hooks md_hooks{};

  md_hooks.map = [](const void *userdata, size_t *n) -> const void * {
    const auto *const elfUserdata = static_cast<const ElfUserdata *>(userdata);
    const auto section = elfUserdata->elf->section(MD_NOTES_SECTION);
    if (!section.has_value()) {
      *n = 0;
      return nullptr;
    }
    auto sec_data = section.value().data();
    *n = sec_data.size();
    return sec_data.begin();
  };

  md_hooks.allocate = [](size_t size, size_t align, void *userdata) {
    auto *elfUserdata = static_cast<ElfUserdata *>(userdata);
    return elfUserdata->allocator->alloc(size, align);
  };

  md_hooks.deallocate = [](void *ptr, void *userdata) {
    auto *elfUserdata = static_cast<ElfUserdata *>(userdata);
    elfUserdata->allocator->free(ptr);
  };

  return md_hooks;
}

mux_result_t readBinaryMetadata(
    cargo::array_view<uint8_t> elf_view,
    cargo::small_vector<handler::VectorizeInfoMetadata, 4> &kernel_info_vec,
    mux::allocator *allocator) {
  if (!loader::ElfFile::isValidElf(elf_view)) {
    return false;
  }
  loader::ElfFile elf{elf_view};

  md_hooks hooks = getElfMetadataReadHooks();
  // The handler below uses this userdata in its destructor, so it must be
  // alive longer than the handler.
  ElfUserdata userdata{&elf, allocator};
  handler::VectorizeInfoMetadataHandler handler;
  if (!handler.init(&hooks, &userdata)) {
    return mux_error_failure;
  }

  handler::VectorizeInfoMetadata md;
  while (handler.read(md)) {
    cargo::dynamic_array<char> kernel_name;
    if (kernel_name.alloc(md.kernel_name.size())) {
      return mux_error_out_of_memory;
    }
    std::copy_n(md.kernel_name.begin(), md.kernel_name.size(),
                kernel_name.begin());

    if (kernel_info_vec.push_back(md)) {
      return mux_error_out_of_memory;
    }
  }
  return mux_success;
}
}  // namespace

namespace riscv {
// Executable from binary
executable_s::executable_s(mux::hal::device *device,
                           mux::dynamic_array<uint8_t> &&object_code)
    : mux::hal::executable(device, std::move(object_code)) {}

cargo::expected<riscv::executable_s *, mux_result_t> executable_s::create(
    riscv::device_s *device, const void *binary, uint64_t binary_length,
    mux::allocator allocator) {
  auto executable = mux::hal::executable::create<riscv::executable_s>(
      device, binary, binary_length, allocator);
  if (!executable) {
    return cargo::make_unexpected(executable.error());
  }
  if (auto error =
          readBinaryMetadata(executable.value()->object_code,
                             executable.value()->kernel_info, &allocator)) {
    allocator.destroy(executable.value());
    return cargo::make_unexpected(error);
  }
  return executable.value();
}

void executable_s::destroy(device_s *device, executable_s *executable,
                           mux::allocator allocator) {
  (void)device;
  allocator.destroy(executable);
}
}  // namespace riscv

mux_result_t riscvCreateExecutable(mux_device_t device, const void *binary,
                                   uint64_t binary_length,
                                   mux_allocator_info_t allocator_info,
                                   mux_executable_t *out_executable) {
  auto executable =
      riscv::executable_s::create(static_cast<riscv::device_s *>(device),
                                  binary, binary_length, allocator_info);
  if (!executable) {
    return executable.error();
  }
  *out_executable = *executable;
  return mux_success;
}

void riscvDestroyExecutable(mux_device_t device, mux_executable_t executable,
                            mux_allocator_info_t allocator_info) {
  riscv::executable_s::destroy(static_cast<riscv::device_s *>(device),
                               static_cast<riscv::executable_s *>(executable),
                               allocator_info);
}
