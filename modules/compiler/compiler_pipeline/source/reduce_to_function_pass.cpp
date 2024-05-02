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

#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/reduce_to_function_pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

static void runOnFunction(Module &M, Function *const F,
                          DenseSet<Function *> &FnsToKeep) {
  for (BasicBlock &BB : *F) {
    for (Instruction &I : BB) {
      if (CallInst *call = dyn_cast<CallInst>(&I)) {
        Function *const Callee = call->getCalledFunction();

        // Ignore indirect function calls
        if (!Callee) {
          continue;
        }

        // Only recurse if we haven't seen this function before and its not an
        // intrinsic.
        if (FnsToKeep.insert(Callee).second && !Callee->isIntrinsic()) {
          auto Err = M.materialize(Callee);

          if (Err) {
            assert(0 && "Bitcode materialization failed!");
          }

          runOnFunction(M, Callee, FnsToKeep);
        }
      }
    }
  }
}

PreservedAnalyses compiler::utils::ReduceToFunctionPass::run(
    Module &M, ModuleAnalysisManager &) {
  DenseSet<Function *> FnsToKeep;

  for (auto &F : M) {
    // If there are any other users of the function which are not a call
    // instructions, then we need to keep this function. This is the case,
    // e.g., with coverage mapping where we have pointers to functions.
    if (any_of(F.users(), [&](const User *U) { return !isa<CallInst>(U); }) &&
        FnsToKeep.insert(&F).second) {
      runOnFunction(M, &F, FnsToKeep);
    }
  }

  // An explicit list of names can be provided to us, else we keep all
  // kernels.
  std::function<bool(const Function &)> checkFn;
  if (!Names.empty()) {
    checkFn = [&](const Function &F) {
      return is_contained(Names, F.getName());
    };
  } else {
    checkFn = [](const Function &F) { return isKernel(F); };
  }

  for (auto &F : M.functions()) {
    if (checkFn(F)) {
      if (FnsToKeep.insert(&F).second) {
        runOnFunction(M, &F, FnsToKeep);
        // Check any derived vectorized forms of this function we know we want
        // to keep, and keep those too.
        SmallVector<LinkMetadataResult, 4> Results;
        if (parseOrigToVeczFnLinkMetadata(F, Results)) {
          for (auto &R : Results) {
            if (R.first && FnsToKeep.insert(R.first).second) {
              runOnFunction(M, R.first, FnsToKeep);
            }
          }
        }
        // If we have a vectorized function pertaining to a function we want to
        // keep, we want to keep the vectorized function too. It may be called
        // in the Barrier pass.
        if (auto Result = parseVeczToOrigFnLinkMetadata(F)) {
          if (Result->first && FnsToKeep.insert(Result->first).second) {
            runOnFunction(M, Result->first, FnsToKeep);
          }
        }
      }
      continue;
    }

    // If we don't want to keep this function, but via vectorization it links
    // to other functions we do, then keep this one too. Since we don't mandate
    // bidirectional metadata links in our spec, this mops up any functions
    // with broken links that would have otherwise been handled above.

    // The case that we have a function A linking to vectorized function
    // B,C,D, etc. If we want to keep any of B,C,D, keep A in case it becomes
    // a scalar tail for any of them.
    SmallVector<LinkMetadataResult, 4> Results;
    if (parseOrigToVeczFnLinkMetadata(F, Results)) {
      if (any_of(Results,
                 [&](const LinkMetadataResult &R) {
                   return R.first && checkFn(*R.first);
                 }) &&
          FnsToKeep.insert(&F).second) {
        runOnFunction(M, &F, FnsToKeep);
      }
    }
    // The case that we have a vectorized function B linking to an original
    // function A. We want to keep A, so keep B in case it becomes a vector
    // main loop.
    if (auto Result = parseVeczToOrigFnLinkMetadata(F)) {
      if (Result->first && checkFn(*Result->first) &&
          FnsToKeep.insert(&F).second) {
        runOnFunction(M, &F, FnsToKeep);
      }
    }
  }

  if (FnsToKeep.empty()) {
    // We couldn't find the function!
    return PreservedAnalyses::all();
  }

  SmallPtrSet<Function *, 16> toDelete;

  for (Function &F : M) {
    if (0 == FnsToKeep.count(&F)) {
      F.dropAllReferences();
      toDelete.insert(&F);
    }
  }

  for (auto f : toDelete) {
    f->eraseFromParent();
  }

  FnsToKeep.clear();

  return toDelete.empty() ? PreservedAnalyses::all()
                          : PreservedAnalyses::none();
}
