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

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/define_mux_dma_pass.h>
#include <llvm/IR/Module.h>

#define DEBUG_TYPE "define-mux-dma"

using namespace llvm;

PreservedAnalyses compiler::utils::DefineMuxDmaPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  // Define all mux dma builtins
  for (auto &F : M.functions()) {
    auto Builtin = BI.analyzeBuiltin(F);
    if (!BI.isMuxDmaBuiltinID(Builtin.ID)) {
      continue;
    }
    LLVM_DEBUG(dbgs() << "  Defining mux DMA builtin: " << F.getName()
                      << "\n";);

    // Define the builtin. If it declares any new dependent builtins, those
    // will be appended to the module's function list and so will be
    // encountered by later iterations.
    if (BI.defineMuxBuiltin(Builtin.ID, M, Builtin.mux_overload_info)) {
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
