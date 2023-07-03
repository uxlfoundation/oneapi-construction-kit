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

#include <base/program_metadata.h>
#include <base/spir_fixup_pass.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/ModuleSummaryAnalysis.h>
#include <llvm/Analysis/ScalarEvolutionAliasAnalysis.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

static const SmallDenseSet<StringRef, 8> WorkItemFuncs{{
    "_Z15get_global_sizej",
    "_Z13get_global_idj",
    "_Z17get_global_offsetj",
    "_Z14get_local_sizej",
    "_Z12get_local_idj",
    "_Z14get_num_groupsj",
    "_Z12get_group_idj",
}};

static bool fixupCC(Function &F) {
  if (!F.isIntrinsic()) {
    return false;
  }
  F.setCallingConv(CallingConv::C);
  for (User *U : F.users()) {
    if (auto *CI = dyn_cast<CallInst>(U)) {
      CI->setCallingConv(CallingConv::C);
    }
  }
  return true;
}

static bool markNoUnwind(Function &F) {
  // we don't use exceptions
  if (F.hasFnAttribute(Attribute::NoUnwind)) {
    return false;
  }
  F.addFnAttr(Attribute::NoUnwind);
  return true;
}

static bool markReadOnly(Function &F) {
  // replace readnone with readonly for WorkItemFuncs
  if (!F.doesNotAccessMemory() || !WorkItemFuncs.contains(F.getName())) {
    return false;
  }
#if LLVM_VERSION_LESS(16, 0)
  F.removeFnAttr(Attribute::ReadNone);
#else
  F.removeFnAttr(Attribute::Memory);
#endif
  F.setOnlyReadsMemory();
  for (User *U : F.users()) {
    if (auto *CI = dyn_cast<CallInst>(U)) {
      if (CI->doesNotAccessMemory()) {
#if LLVM_VERSION_LESS(16, 0)
        CI->removeFnAttr(Attribute::ReadNone);
#else
        CI->removeFnAttr(Attribute::Memory);
#endif
      }
      CI->setOnlyReadsMemory();
    }
  }
  return true;
}

static bool fixAtomic(Function &F) {
  // the SPIR kernels we get have the wrong signature for atomics (because
  // of mangling order bug between const & volatile address spaces), so we
  // need to fix them up
  auto Name = F.getName().str();

  bool Modified = false;
  auto DoFixup = [&Name, &Modified](const std::string &Pattern, const char C) {
    // if we found the offending pattern
    if (const size_t Pos = Name.find(Pattern); Pos != std::string::npos) {
      Modified = true;
      // get the address space char
      const char ASChar = Name[Pos + Pattern.size()];

      auto Replacement = std::string("PU3AS");
      Replacement += ASChar;
      Replacement += C;

      Name.replace(Pos, Replacement.size(), Replacement);
    }
  };

  // fixup the broken const and volatile pointer patterns
  DoFixup("PKU3AS", 'K');
  DoFixup("PVU3AS", 'V');
  F.setName(Name);
  return Modified;
}

PreservedAnalyses compiler::spir::SpirFixupPass::run(llvm::Module &M,
                                                     ModuleAnalysisManager &) {
  bool ModifiedCFG = false, ModifiedSCEV = false, ModifiedAttrs = false;

  // sometimes LLVM intrinsics will be passed with the incorrect calling
  // convention and SPIR functions may have incorrect attributes.
  for (auto &F : M) {
    ModifiedCFG |= fixupCC(F);
    ModifiedAttrs |= markNoUnwind(F);
    ModifiedAttrs |= markReadOnly(F);
  }
  for (auto &F : M) {
    ModifiedCFG |= fixAtomic(F);
  }

  // TODO CA-1212: Document why this is necessary and what is going on.
  using MetadataReplacementMap = DenseMap<MDNode *, MDNode *>;
  auto MDB = MDBuilder(M.getContext());
  MetadataReplacementMap MRM;
  std::function<void(MDNode *, unsigned)> TBAAOperandPatternMatch =
      [&TBAAOperandPatternMatch, &MDB, &MRM](MDNode *Node, unsigned i) {
        auto MDOp = cast<MDNode>(Node->getOperand(i));
        if (MRM.count(MDOp) > 0) {
          Node->replaceOperandWith(i, MRM.lookup(MDOp));
          return;
        }

        // Note: It is actually possible for the 'old' form TBAA to have three
        // operands here, where the last MDOp is the value '1' for constant.
        // However, I don't know how to distinguish between that and
        // three-operand form noting a 1 byte offset.  It never happens in the
        // SPIR CTS, and it will never happen with ComputeCpp.
        if (MDOp->getNumOperands() == 2) {
          // Recurse first so that the memoization works
          TBAAOperandPatternMatch(MDOp, 1);

          MDNode *newOperand = MDB.createTBAAScalarTypeNode(
              cast<MDString>(MDOp->getOperand(0))->getString(),
              cast<MDNode>(MDOp->getOperand(1)), 0);
          MRM.try_emplace(MDOp, newOperand);
          Node->replaceOperandWith(i, newOperand);
        }
      };
  auto TBAANodePatternMatch = [&TBAAOperandPatternMatch](MDNode *node) {
    bool isStructPathTBAA =
        isa<MDNode>(node->getOperand(0)) && node->getNumOperands() >= 3;
    if (isStructPathTBAA) {
      TBAAOperandPatternMatch(node, 0);
      TBAAOperandPatternMatch(node, 1);
    }
  };
  auto TBAAPatternMatch = [&TBAANodePatternMatch](Instruction &I) {
    if (I.hasMetadata()) {
      if (MDNode *MD = I.getMetadata(LLVMContext::MD_tbaa)) {
        TBAANodePatternMatch(MD);
      }
    }
  };
  for (auto &F : M.functions()) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        TBAAPatternMatch(I);
      }
    }
  }

  // ProgramInfo reads the kernel metadata for the module
  compiler::ProgramInfo ProgramInfo;
  (void)moduleToProgramInfo(ProgramInfo, &M, true);

  // From version 4.0 onwards clang produces a sampler_t pointer parameter
  // for sampler arguments, but spir still mandates that this is an i32.
  // This causes issues from the run-time aspect which expects the
  // pointer. For this reason we create a new function if we have a
  // sampler parameter which takes the sampler_t pointer arguments,
  // converts them to i32 and then calls the original function.
  for (auto &KernelInfo : ProgramInfo) {
    PointerType *SamplerTypePtr = nullptr;

    // Check to see if any of the arguments are sampler
    bool HasSamplerArg = std::any_of(
        KernelInfo.argument_types.cbegin(), KernelInfo.argument_types.cend(),
        [&](compiler::ArgumentType Type) {
          return Type.kind == compiler::ArgumentKind::SAMPLER;
        });

    if (HasSamplerArg) {
      const DataLayout &DL = M.getDataLayout();
      // Get the sampler struct and create a pointer to it if not already
      // done so
      if (nullptr == SamplerTypePtr) {
        auto *samplerType =
            multi_llvm::getStructTypeByName(M, "opencl.sampler_t");

        if (nullptr == samplerType) {
          samplerType = StructType::create(M.getContext(), "opencl.sampler_t");
        }
        SamplerTypePtr = PointerType::get(samplerType, 2);
      }

      // Create a duplicate function type, but with sampler struct
      // pointers where we previously had samplers as i32
      if (Function *F = M.getFunction(KernelInfo.name)) {
        ModifiedCFG = true;
        auto *FuncTy = F->getFunctionType();
        unsigned NumParams = FuncTy->getNumParams();
        SmallVector<Type *, 8> NewParamTypes(NumParams);
        for (unsigned i = 0; i < NumParams; i++) {
          if (KernelInfo.argument_types[i].kind ==
              compiler::ArgumentKind::SAMPLER) {
            NewParamTypes[i] = SamplerTypePtr;
          } else {
            NewParamTypes[i] = FuncTy->getParamType(i);
          }
        }
        auto NewFuncTy = FunctionType::get(FuncTy->getReturnType(),
                                           NewParamTypes, /*isVarArg*/ false);

        // create our new function, using the linkage from the old one
        auto *NewF = Function::Create(NewFuncTy, F->getLinkage(), "", &M);

        // set the correct calling convention and copy attributes
        NewF->setCallingConv(F->getCallingConv());
        NewF->copyAttributesFrom(F);

        // old F has been wrapped and shouldn't be classed as a kernel
        F->setCallingConv(CallingConv::SPIR_FUNC);

        // propagate the calling conv update to any users
        for (User *U : F->users()) {
          if (CallInst *CI = dyn_cast<CallInst>(U)) {
            CI->setCallingConv(F->getCallingConv());
          }
        }

        // take the name of the old function
        NewF->takeName(F);

        auto *EntryBB = BasicBlock::Create(M.getContext(), "EntryBB", NewF);
        IRBuilder<> B(EntryBB);
        SmallVector<Value *, 8> Args;
        // set up the arguments for the original function, using the
        // current arguments but casting and truncating any sampler ones
        for (auto &Arg : NewF->args()) {
          if (Arg.getType() == SamplerTypePtr) {
            auto *PtrToInt = B.CreatePtrToInt(
                &Arg, IntegerType::get(M.getContext(),
                                       DL.getPointerSizeInBits(0)));
            auto *Trunc = B.CreateTrunc(PtrToInt, B.getInt32Ty());
            Args.push_back(Trunc);
          } else {
            Args.push_back(&Arg);
          }
        }
        auto *CI = B.CreateCall(F, Args);
        CI->setCallingConv(F->getCallingConv());
        B.CreateRetVoid();

        if (auto *NamedMetaData = M.getNamedMetadata("opencl.kernels")) {
          for (auto *MD : NamedMetaData->operands()) {
            if (MD && MD->getOperand(0) == ValueAsMetadata::get(F)) {
              MD->replaceOperandWith(0, ValueAsMetadata::get(NewF));
            }
          }
        }
      }
    } else {
      // In cases where there is no sampler we want to make sure that the
      // calling convention of the call to a spir kernel is set to
      // `SPIR_KERNEL`. Otherwise the calling conventions will mismatch which is
      // considered an undefined behaviour and will be considered an illegal
      // instruction.
      if (M.getFunction(KernelInfo.name)->getCallingConv() ==
          CallingConv::SPIR_KERNEL) {
        for (User *U : M.getFunction(KernelInfo.name)->users()) {
          if (auto CI = dyn_cast<CallInst>(U)) {
            ModifiedCFG = true;
            CI->setCallingConv(CallingConv::SPIR_KERNEL);
          }
        }
      }
    }
  }

  // According to the spir spec available_externally is supposed to represent
  // C99 inline semantics. The closest thing llvm has natively is LinkOnce.
  // It doesn't quite give us the same behaviour, but it does assume a more
  // definitive definition might exist outside the M, which is good enough
  // to not go catastrophically awry.
  for (auto &G : M.globals()) {
    if (G.getLinkage() == GlobalValue::AvailableExternallyLinkage) {
      G.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }
  }

  for (auto &F : M) {
    if (F.getLinkage() == GlobalValue::AvailableExternallyLinkage) {
      F.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }
  }

  PreservedAnalyses PA;
  if (!ModifiedCFG) {
    PA.preserveSet<CFGAnalyses>();
  }
  if (!ModifiedSCEV) {
    PA.preserve<ScalarEvolutionAnalysis>();
  }
  if (ModifiedAttrs) {
    PA.abandon<BasicAA>();
    PA.abandon<ModuleSummaryIndexAnalysis>();
    PA.abandon<MemoryDependenceAnalysis>();
  }
  return PA;
}
