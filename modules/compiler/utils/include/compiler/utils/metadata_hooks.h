// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Write kernel metadata into a binary.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved

#ifndef COMPILER_UTILS_METADATA_HOOKS_H_INCLUDED
#define COMPILER_UTILS_METADATA_HOOKS_H_INCLUDED

#include <llvm/IR/Module.h>
#include <metadata/metadata.h>

#include <string>

namespace compiler {
namespace utils {

constexpr const char MD_NOTES_SECTION[] = "notes";
constexpr const char MD_BLOCK_NAME[] = "KernelMetadata";

/// @brief Get hooks for writing metadata into an elf binary with the binary
/// metadata API.
///
/// @return md_hooks
md_hooks getElfMetadataWriteHooks();

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_METADATA_HOOKS_H_INCLUDED
