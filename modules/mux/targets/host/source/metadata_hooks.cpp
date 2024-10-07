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

#include <cargo/string_view.h>
#include <host/metadata_hooks.h>
#include <metadata/detail/utils.h>
#include <metadata/handler/vectorize_info_metadata.h>
#include <metadata/metadata.h>

namespace {
/// @brief Struct used to pass arguments to metadata API.
struct ElfUserdata {
  loader::ElfFile *elf;
  mux::allocator *allocator;
};

md_hooks getHostMdReadHooks() {
  md_hooks md_hooks{};

  md_hooks.map = [](const void *userdata, size_t *n) -> const void * {
    auto *elfUserdata = static_cast<const ElfUserdata *>(userdata);
    auto *elf = elfUserdata->elf;
    auto notes_section = elf->section(host::MD_NOTES_SECTION);
    if (!notes_section.has_value()) {
      *n = 0;
      return nullptr;
    }
    auto sec_data = notes_section.value().data();
    *n = sec_data.size();
    return static_cast<void *>(sec_data.begin());
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

}  // namespace

namespace host {

cargo::optional<kernel_variant_map> readBinaryMetadata(loader::ElfFile *elf,
                                                       mux::allocator *alloc) {
  if (!elf->section(MD_NOTES_SECTION)) {
    return cargo::nullopt;
  }
  kernel_variant_map kernels;

  md_hooks hooks = getHostMdReadHooks();
  // The handler below uses this userdata in its destructor, so it must be
  // alive longer than the handler.
  ElfUserdata userdata{elf, alloc};
  handler::VectorizeInfoMetadataHandler handler;

  if (!handler.init(&hooks, &userdata)) {
    return cargo::nullopt;
  }

  handler::VectorizeInfoMetadata md;
  while (handler.read(md)) {
    // We don't expect scalable vectorization widths on host.
    if (md.min_work_item_factor.isScalable() ||
        md.pref_work_item_factor.isScalable()) {
      return cargo::nullopt;
    }
    const host::binary_kernel_s kernel{
        /*hook*/ 0,
        std::move(md.kernel_name),
        static_cast<uint32_t>(md.local_memory_usage),
        md.min_work_item_factor.getFixedValue(),
        md.pref_work_item_factor.getFixedValue(),
        md.sub_group_size.getFixedValue()};
    auto it = kernels.find(md.source_name);
    if (it != kernels.end()) {
      it->second.push_back(kernel);
    } else {
      kernels.insert(
          std::make_pair(std::move(md.source_name),
                         std::vector<host::binary_kernel_s>({kernel})));
    }
  }
  return kernels;
}

}  // namespace host
