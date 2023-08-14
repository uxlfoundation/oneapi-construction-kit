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
/// EncodeBuiltinRangeMetadataPass pass.

#ifndef COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED
#define COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

#include <array>
#include <optional>

namespace compiler {
namespace utils {

struct EncodeBuiltinRangeMetadataOptions {
  std::array<std::optional<uint64_t>, 3> MaxLocalSizes;
  std::array<std::optional<uint64_t>, 3> MaxGlobalSizes;
};

struct EncodeBuiltinRangeMetadataPass
    : public llvm::PassInfoMixin<EncodeBuiltinRangeMetadataPass> {
  EncodeBuiltinRangeMetadataPass(EncodeBuiltinRangeMetadataOptions Opts)
      : MaxLocalSizes(Opts.MaxLocalSizes),
        MaxGlobalSizes(Opts.MaxGlobalSizes) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  std::array<std::optional<uint64_t>, 3> MaxLocalSizes;
  std::array<std::optional<uint64_t>, 3> MaxGlobalSizes;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ENCODE_BUILTIN_RANGE_METADATA_PASS_H_INCLUDED
