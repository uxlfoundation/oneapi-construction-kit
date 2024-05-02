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

#include <compiler/utils/add_scheduling_parameters_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/PriorityWorklist.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/multi_llvm.h>

#define DEBUG_TYPE "add-sched-params"

using namespace llvm;

PreservedAnalyses compiler::utils::AddSchedulingParametersPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  auto &Ctx = M.getContext();
  auto *const I32Ty = Type::getInt32Ty(Ctx);

  SmallVector<std::string, 4> ParamDebugNames;
  const auto &SchedParamInfo = BI.getMuxSchedulingParameters(M);

  if (SchedParamInfo.empty()) {
    return PreservedAnalyses::all();
  }

  const auto NumSchedParams = SchedParamInfo.size();

  for (const auto &P : SchedParamInfo) {
    ParamDebugNames.push_back(P.ParamDebugName);
  }

  // Emit these scheduling parameters to the module, for reference.
  setSchedulingParameterModuleMetadata(M, ParamDebugNames);

  SmallPtrSet<Function *, 4> Visited;
  SmallPtrSet<Function *, 4> LeafBuiltins;
  SmallVector<Function *, 4> FuncsToClone;
  SmallPriorityWorklist<Function *, 4> Worklist;

  // Collect the leaf functions which require scheduling parameters.
  for (auto &F : M.functions()) {
    // Kernel entry points must present a consistent ABI to external users,
    // regardless of whether they call builtins that require scheduling
    // parameters or not.
    if (isKernelEntryPt(F)) {
      Visited.insert(&F);
      Worklist.insert(&F);
      FuncsToClone.push_back(&F);
      continue;
    }
    if (!F.isDeclaration() || F.isIntrinsic()) {
      continue;
    }
    const auto B = BI.analyzeBuiltin(F);
    if (B.isUnknown() || !B.isValid()) {
      continue;
    }
    if (BI.requiresSchedulingParameters(B.ID)) {
      Visited.insert(&F);
      Worklist.insert(&F);
      LeafBuiltins.insert(&F);
      FuncsToClone.push_back(&F);
    }
  }

  LLVM_DEBUG(dbgs() << "Leaf functions requiring scheduling parameters:\n";
             for (auto *F
                  : Visited) { dbgs() << "  " << F->getName() << "\n"; });

  if (Visited.empty()) {
    return PreservedAnalyses::all();
  }

  // Iterate over the functions that require scheduling parameters and
  // recursively register all callers of those functions as needing scheduling
  // parameters too.
  while (!Worklist.empty()) {
    Function *F = Worklist.pop_back_val();
    for (auto *U : F->users()) {
      if (auto *CB = dyn_cast<CallBase>(U)) {
        auto *Caller = CB->getFunction();
        if (Visited.insert(Caller).second) {
          Worklist.insert(Caller);
          FuncsToClone.push_back(Caller);
          LLVM_DEBUG(dbgs() << "Function '" << Caller->getName()
                            << "' requires scheduling parameters\n");
        }
      } else {
        report_fatal_error("unhandled user type");
      }
    }
  }

  SmallVector<Function *, 8> NewFuncs;
  ValueMap<Function *, Function *> OldToNewFnMap;

  for (auto *OldF : FuncsToClone) {
    ValueToValueMapTy VMap;
    auto *OldFTy = OldF->getFunctionType();
    const unsigned NumParams = OldFTy->getNumParams();
    SmallVector<Type *, 8> NewParamTypes(NumParams);

    for (unsigned i = 0; i < NumParams; i++) {
      NewParamTypes[i] = OldFTy->getParamType(i);
    }
    for (const auto &P : SchedParamInfo) {
      NewParamTypes.push_back(P.ParamTy);
    }

    auto *NewFTy = FunctionType::get(OldFTy->getReturnType(), NewParamTypes,
                                     /*isVarArg*/ false);

    // Create our new function, using the linkage from the old one
    // Note - we don't have to copy attributes or metadata over, as
    // CloneFunctionInto does that for us.
    auto *NewF = Function::Create(NewFTy, OldF->getLinkage(), "", &M);

    const StringRef BaseName = getBaseFnNameOrFnName(*OldF);
    if (LeafBuiltins.contains(OldF)) {
      // Leaf builtins need to retain their current names to keep builtin
      // recognition working. In this case, rename the old function instead.
      // Renaming a (mux) builtin but changing its prototype is acceptable,
      // whereas replacing a user function is less so.
      NewF->takeName(OldF);
      OldF->setName(NewF->getName() + ".old");
    } else {
      NewF->setName(OldF->getName() + ".mux-sched-wrapper");
    }
    NewF->setCallingConv(OldF->getCallingConv());

    // Scrub any old subprogram - CloneFunctionInto will create a new one for
    // us
    if (auto *const SP = OldF->getSubprogram()) {
      NewF->setSubprogram(nullptr);
    }

    // Map all original function arguments to the new function arguments
    for (auto it : zip(OldF->args(), NewF->args())) {
      auto *OldA = &std::get<0>(it);
      auto *NewA = &std::get<1>(it);
      VMap[OldA] = NewA;
      NewA->setName(OldA->getName());
    }

    auto ChangeType = CloneFunctionChangeType::LocalChangesOnly;
    SmallVector<ReturnInst *, 8> Returns;
    CloneFunctionInto(NewF, OldF, VMap, ChangeType, Returns);

    SmallVector<Metadata *, 4> MDOps(NumSchedParams);
    // Add in the new parameter attributes here, because CloneFunctionInto
    // wipes out pre-existing attributes on NewF which aren't in OldF.
    for (unsigned i = 0; i != NumSchedParams; i++) {
      auto *NewArg = NewF->getArg(NumParams + i);
      auto AB = AttrBuilder(Ctx, SchedParamInfo[i].ParamAttrs);
      NewArg->setName(SchedParamInfo[i].ParamName);
      NewArg->addAttrs(AB);
      MDOps[i] =
          ConstantAsMetadata::get(ConstantInt::get(I32Ty, NumParams + i));
    }

    // Steal the kernel information from the old function. Again, do this now
    // as dropping the info from OldF then calling CloneFunctionInto would wipe
    // it from NewF.
    dropIsKernel(*OldF);

    if (!BaseName.empty()) {
      setBaseFnName(*NewF, BaseName);
    }

    auto *const MDOpsTuple = MDTuple::get(Ctx, MDOps);
    NewF->setMetadata("mux_scheduled_fn", MDOpsTuple);

    // Mark the old function as internal - this new function takes its place.
    // Let the old one be cleaned up later if unused. Note that declarations
    // can't be marked internal.
    if (!OldF->isDeclaration()) {
      OldF->setLinkage(Function::InternalLinkage);
    }

    NewFuncs.push_back(NewF);
    OldToNewFnMap[OldF] = NewF;
  }

  // Once all functions are cloned, go through them and remap call-sites to
  // other cloned functions.
  SmallVector<Instruction *, 32> CallsToErase;

  for (auto *F : NewFuncs) {
    // We know the last #NumSchedParams parameters of FArgs are scheduling
    // parameters.
    SmallVector<Value *, 4> FArgs(make_pointer_range(F->args()));

    for (auto &BB : *F) {
      for (auto &I : BB) {
        auto *CB = dyn_cast<CallBase>(&I);
        if (!CB) {
          continue;
        }
        auto *const OldF = CB->getCalledFunction();
        if (auto *NewF = OldToNewFnMap.lookup(OldF)) {
          if (CB->getOpcode() != Instruction::Call) {
            llvm_unreachable("Unhandled CallBase sub-class");
          }
          // Construct the list of arguments with which to call the cloned
          // function.
          SmallVector<Value *, 8> NewArgs(CB->args());
          // Append all of our scheduling parameters to the old list of
          // parameters.
          append_range(
              NewArgs,
              ArrayRef(FArgs.data(), FArgs.size()).take_back(NumSchedParams));

          auto *NewCB = CallInst::Create(NewF, NewArgs, "", CB);
          NewCB->takeName(CB);
          NewCB->copyMetadata(*CB);
          // Copy over all the old attributes from the call
          auto OldAttrs = CB->getAttributes();
          // ... and append the new parameter attributes
          for (unsigned i = 0; i != NumSchedParams; i++) {
            OldAttrs = OldAttrs.addParamAttributes(
                Ctx, OldF->arg_size() + i,
                AttrBuilder(Ctx, SchedParamInfo[i].ParamAttrs));
          }
          NewCB->setAttributes(OldAttrs);
          NewCB->setDebugLoc(CB->getDebugLoc());

          CB->replaceAllUsesWith(NewCB);
          CallsToErase.push_back(CB);
        }
      }
    }
  }

  for (auto *I : CallsToErase) {
    I->eraseFromParent();
  }

  return PreservedAnalyses::none();
}
