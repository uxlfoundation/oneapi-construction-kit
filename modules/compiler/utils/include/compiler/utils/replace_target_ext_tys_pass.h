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

#ifndef COMPILER_UTILS_REPLACE_TARGET_EXT_TYS_PASS_H_INCLUDED
#define COMPILER_UTILS_REPLACE_TARGET_EXT_TYS_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

struct ReplaceTargetExtTysOptions {
  /// @brief Set to true if the pass should replace "spirv.Image" types.
  bool ReplaceImages = true;
  /// @brief Set to true if the pass should replace "spirv.Sampler" types.
  bool ReplaceSamplers = true;
  /// @brief Set to true if the pass should replace "spirv.Event" types.
  bool ReplaceEvents = true;
};

/// @brief This pass replaces LLVM target extension types with types appropriate
/// for the ComputeMux target.
///
/// It can replace any subset of the following target extension types,
/// module-wide:
/// * "spirv.Image"
/// * "spirv.Event"
/// * "spirv.Sampler"
///
/// The ComputeMux target's implementation of BuiltinInfo is ultimately
/// responsible for the precise mapping of types - there is nothing to say that
/// the target can't introduce further target extension types if it wishes.
class ReplaceTargetExtTysPass final
    : public llvm::PassInfoMixin<ReplaceTargetExtTysPass> {
 public:
  ReplaceTargetExtTysPass(const ReplaceTargetExtTysOptions &Options)
      : ReplaceImages(Options.ReplaceImages),
        ReplaceSamplers(Options.ReplaceSamplers),
        ReplaceEvents(Options.ReplaceEvents) {}

  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);

 private:
  bool ReplaceImages;
  bool ReplaceSamplers;
  bool ReplaceEvents;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_REPLACE_TARGET_EXT_TYS_PASS_H_INCLUDED
