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
#include <compiler/utils/dma.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <multi_llvm/opaque_pointers.h>

#define DEBUG_TYPE "define-mux-dma"

using namespace llvm;

PreservedAnalyses compiler::utils::DefineMuxDmaPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  // Define all mux builtins
  for (auto &F : M.functions()) {
    auto ID = BI.analyzeBuiltin(F).ID;
    if (!BI.isMuxDmaBuiltinID(ID)) {
      continue;
    }
    LLVM_DEBUG(dbgs() << "  Defining mux DMA builtin: " << F.getName()
                      << "\n";);

    // Define the builtin. If it declares any new dependent builtins, those
    // will be appended to the module's function list and so will be
    // encountered by later iterations.
    if (BI.defineMuxBuiltin(ID, M)) {
      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
