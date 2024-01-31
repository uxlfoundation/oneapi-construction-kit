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

#include <base/combine_fpext_fptrunc_pass.h>
#include <compiler/utils/device_info.h>
#include <llvm/IR/Instructions.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

PreservedAnalyses compiler::CombineFPExtFPTruncPass::run(
    Function &F, FunctionAnalysisManager &AM) {
  const auto &MAMProxy = AM.getResult<ModuleAnalysisManagerFunctionProxy>(F);
  const auto *DI =
      MAMProxy.getCachedResult<compiler::utils::DeviceInfoAnalysis>(
          *F.getParent());
  bool modified = false;

  // Check if doubles are supported, in which case there's nothing to do.
  if (DI && DI->double_capabilities != 0) {
    return PreservedAnalyses::all();
  }

  SmallVector<Instruction *, 8> to_delete;

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *fpext = dyn_cast<FPExtInst>(&I)) {
        // if the fpext is unused, remove it, this happens when a printf call
        // has extra floating point arguments, clang will still expand them to
        // double but the printf pass will ignore them
        if (fpext->hasNUses(0)) {
          to_delete.push_back(fpext);

          modified = true;
          continue;
        }

        if (fpext->getType()->isVectorTy()) {
          // Printf can take a vector floating point argument type
          SmallVector<ExtractElementInst *, 8> extracts;
          for (auto *user : fpext->users()) {
            if (auto *extract = dyn_cast<ExtractElementInst>(user)) {
              if (extract->hasOneUse() && extract->getType()->isDoubleTy()) {
                if (auto *fptrunc =
                        dyn_cast<FPTruncInst>(extract->user_back())) {
                  // It shouldn't cause any problems if it encounters a mixture
                  // of single and half precision, but not sure if/how that
                  // could actually happen.
                  if (fptrunc->getDestTy() == fpext->getSrcTy()) {
                    extracts.push_back(extract);
                  }
                }
              }
            }
          }

          // Verify that all fpext users are extract element insts
          if (extracts.size() != fpext->getNumUses()) {
            continue;
          }

          for (auto old_extract : extracts) {
            // Create new extract instruction from single/half precision vector
            auto *new_extract = ExtractElementInst::Create(
                fpext->getOperand(0), old_extract->getIndexOperand(), "",
                old_extract);
            auto *fptrunc = cast<FPTruncInst>(old_extract->user_back());
            fptrunc->replaceAllUsesWith(new_extract);

            to_delete.push_back(fptrunc);
            to_delete.push_back(old_extract);
          }

          // Delete vectorized float to double promotion
          to_delete.push_back(fpext);
          modified = true;
        } else if (fpext->getDestTy()->isDoubleTy()) {
          // we might have multiple fptruncs from the same fpext sometimes
          unsigned remaining = fpext->getNumUses();
          const unsigned src_bits = fpext->getSrcTy()->getPrimitiveSizeInBits();
          auto *fp_op = fpext->getOperand(0);
          for (auto *user : fpext->users()) {
            if (auto *fptrunc = dyn_cast<FPTruncInst>(user)) {
              // and if the operation is lossless
              const unsigned dst_bits =
                  fptrunc->getDestTy()->getPrimitiveSizeInBits();

              if (src_bits <= dst_bits) {
                if (src_bits == dst_bits) {
                  // shortcut the instructions
                  fptrunc->replaceAllUsesWith(fp_op);
                } else {
                  auto new_ext = CastInst::CreateFPCast(
                      fp_op, fptrunc->getDestTy(), "", fpext);

                  // shortcut the instructions
                  fptrunc->replaceAllUsesWith(new_ext);
                }

                // and remember to delete them
                to_delete.push_back(fptrunc);
                --remaining;
              }
            }
          }

          if (remaining == 0) {
            to_delete.push_back(fpext);
          }
          modified = true;
        }
      }
    }
  }

  // delete the unnecessary instructions
  for (auto I : to_delete) {
    I->dropAllReferences();
    I->eraseFromParent();
  }

  return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
