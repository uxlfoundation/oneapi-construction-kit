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

#include <host/disable_neon_attribute_pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/triple.h>
#include <multi_llvm/vector_type_helper.h>

using namespace llvm;

// Set attribute disabling Neon on all functions.
// We need to do this for every functions since '-neon' affects the ABI
// calling convention. Currently LLVM(4.0) doesn't account for this
// inconsistency between callee & caller which leads to incorrect results.
//
// Currently this solution isn't performant and will be reviewed in the
// future, see JIRA CA-343.
static PreservedAnalyses disableNEONAttr(Module &M) {
  for (Function &F : M) {
    F.addFnAttr("target-features", "-neon");
  }

  return PreservedAnalyses::all();
}

PreservedAnalyses host::DisableNeonAttributePass::run(Module &M,
                                                      ModuleAnalysisManager &) {
  // This pass is a workaround for an issue specific to 64-bit ARM. Where Neon
  // vector conversions from i64 -> float do the conversion in two stages,
  // i64 -> double then double -> float. This loses precision because of
  // incorrect rounding in the intermediate value.
  const Triple triple(M.getTargetTriple());
  if (Triple::aarch64 != triple.getArch()) {
    return PreservedAnalyses::all();
  }

  // Find UIToFP/SIToFP vector cast instructions, casting [u]long -> float.
  for (Function &F : M) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *cast = dyn_cast<CastInst>(&I)) {
          // Check this cast is an int to float cast
          auto opCode = cast->getOpcode();
          if (opCode != Instruction::UIToFP && opCode != Instruction::SIToFP) {
            continue;
          }

          // Check the cast has vector operand types and is i64 -> float
          auto *vecDstType = dyn_cast<FixedVectorType>(cast->getDestTy());
          auto *vecSrcType = dyn_cast<FixedVectorType>(cast->getSrcTy());
          if (vecDstType && vecDstType->getElementType()->isFloatTy() &&
              vecSrcType && vecSrcType->getElementType()->isIntegerTy(64)) {
            return disableNEONAttr(M);
          }
        }
      }
    }
  }

  // No cast found, exit without disabling Neon
  return PreservedAnalyses::all();
}
