// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Mux read binary metadata.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_HOST_METADATA_HOOKS_H_INCLUDED
#define MUX_HOST_METADATA_HOOKS_H_INCLUDED
#include <cargo/optional.h>
#include <cargo/string_view.h>
#include <host/executable.h>
#include <loader/elf.h>

namespace host {
constexpr const char MD_NOTES_SECTION[] = "notes";

cargo::optional<kernel_variant_map> readBinaryMetadata(loader::ElfFile *elf,
                                                       mux::allocator *alloc);

}  // namespace host

#endif  // MUX_HOST_METADATA_HOOKS_H_INCLUDED
