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

#ifndef COMPILER_UTILS_LLD_LINKER_H_INCLUDED
#define COMPILER_UTILS_LLD_LINKER_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/string_algorithm.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>

#include <vector>

namespace compiler {
namespace utils {

/// @brief Helper function to prepare a list of options for using with LLD.
///
/// Takes a vector of command-line options and appends them to a vector of
/// command-line options but prepended with "-mllvm". Split flag/value options
/// are merged thus:
///
///   {"--foo", "x"}  -> {"-mllvm", "--foo=x"}
///
/// so as not to leave an invalid command-line option "x":
///
///   {"--foo", "x"}  -> {"-mllvm", "--foo", "x"}
///   {"--foo", "x"}  -> {"-mllvm", "--foo", "-mllvm", "x"}
void appendMLLVMOptions(cargo::array_view<cargo::string_view> options,
                        std::vector<std::string> &lld_args);

/// @brief link the binary using lld
/// @param rawBinary input binary to link
/// @param linkerScriptStr lld linker script as a string
/// @param linkerLib pointer to library object to link against. May be nullptr.
/// @param linkerLibBytes size on bytes of library object
/// @param additionalLinkArgs extra args over the basic ones
/// @return The final linked binary on success, error on failure
/// @note  This is kept as a header, so that targets are not forced to link with
/// LLVM LLD libraries. Preserves CA_LLVM_OPTIONS but no other previously
/// parsed command-line options.
llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>>
lldLinkToBinary(const llvm::ArrayRef<uint8_t> rawBinary,
                const std::string &linkerScriptStr, const uint8_t *linkerLib,
                unsigned int linkerLibBytes,
                const llvm::SmallVectorImpl<std::string> &additionalLinkArgs);

} // namespace utils
} // namespace compiler

#endif // COMPILER_UTILS_LLD_LINKER_H_INCLUDED
