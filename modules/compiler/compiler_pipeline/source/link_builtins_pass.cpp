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

// This pass will manually link in any functions required from a given
// 'builtins' module, into the current module. It exists for a few reasons:
// * LLVM's LinkModules is destructive to the source module - it will happily
//   destroy the source module as it links it into the destination. This is
//   fine for most cases, but not ours. In our case, we want to load the
//   builtins module once (in our finalizer) and then re-use that loaded module
//   multiple times (saves significant memory & processing requirements on our
//   hot path).
// * We can strip out unnecessary symbols as we perform our link step - meaning
//   we can do what amounts to a simple global DCE pass for free.
// * We can run our link step as an LLVM pass. Previously, we would link our
//   kernel module into the lazily loaded builtins module (the recommended way
//   to link between a small and a very large LLVM module), which we would not
//   be able to do in a pass (as the Module the pass refers to effectively dies
//   as the linking would occur).

#include <compiler/utils/StructTypeRemapper.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/link_builtins_pass.h>
#include <compiler/utils/mangling.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

namespace {

class GlobalVarMaterializer final : public ValueMaterializer {
 public:
  GlobalVarMaterializer(Module &M) : M(M) {}

  /// @brief List of variables created during materialization.
  const ArrayRef<GlobalVariable *> getGlobalVariables() const {
    return GlobalVars;
  }

  /// @brief Materialize the given value.
  ///
  /// @param[in] value - Value to materialize.
  ///
  /// @return A value that lives in the destination module, or nullptr if the
  /// given value could not be materialized (e.g. it is not a global variable).
  virtual Value *materialize(Value *value) override final {
    auto GV = dyn_cast<GlobalVariable>(value);
    if (!GV) {
      return nullptr;
    }

    auto *NewGV = M.getGlobalVariable(GV->getName());

    if (!NewGV) {
      NewGV = new GlobalVariable(M, GV->getValueType(), GV->isConstant(),
                                 GV->getLinkage(), nullptr, GV->getName(),
                                 nullptr, GV->getThreadLocalMode(),
                                 GV->getType()->getAddressSpace());
      NewGV->copyAttributesFrom(GV);
      GlobalVars.push_back(GV);
    }

    return NewGV;
  }

 private:
  Module &M;
  SmallVector<GlobalVariable *, 8> GlobalVars;
};
}  // namespace

/// @brief If we have the same structs in the main module and builtins with
/// different names, copy the body
/// @param [in] M LLVM Module
/// @param [out] Map A structure mapping from builtin structure types to the
/// corresponding module types
void compiler::utils::LinkBuiltinsPass::cloneStructs(
    Module &M, Module &BuiltinsModule, compiler::utils::StructMap &Map) {
  for (auto *StructTy : M.getIdentifiedStructTypes()) {
    auto StructName = StructTy->getName();

    for (auto *BuiltinStructTy : BuiltinsModule.getIdentifiedStructTypes()) {
      auto BuiltinStructName = BuiltinStructTy->getName();

      const char *Suffix = ".0123456789";

      // check if the names match (minus the suffix LLVM sometimes adds to
      // struct types to differentiate between them)
      if (StructName.rtrim(Suffix) == BuiltinStructName.rtrim(Suffix)) {
        if (StructTy->isOpaque() && !BuiltinStructTy->isOpaque()) {
          StructTy->setBody(BuiltinStructTy->elements(),
                            BuiltinStructTy->isPacked());
        }

        Map[BuiltinStructTy] = StructTy;
      }
    }
  }
}

void compiler::utils::LinkBuiltinsPass::cloneBuiltins(
    Module &M, Module &BuiltinsModule,
    SmallVectorImpl<std::pair<Function *, bool>> &BuiltinFnDecls,
    compiler::utils::StructTypeRemapper *StructMap) {
  // Clone the builtin and its callees.
  DenseMap<Function *, bool> Callees;

  while (!BuiltinFnDecls.empty()) {
    auto [BuiltinFn, IsImplicit] = BuiltinFnDecls.pop_back_val();

    // if we are already tracking the callee, we can skip the function
    if (!Callees.insert({BuiltinFn, IsImplicit}).second) {
      continue;
    }

    auto Error = BuiltinsModule.materialize(BuiltinFn);

    assert(!Error && "Bitcode materialization failed!");

    // Find any callees in the function and add them to the list.
    for (auto &BB : *BuiltinFn) {
      for (auto &I : BB) {
        // if we have a call instruction
        if (auto *const CI = dyn_cast<CallInst>(&I)) {
          // and the called function is known
          if (Function *Callee = CI->getCalledFunction()) {
            // Assume that we have no calls in builtins to LLVM intrinsics that
            // require libcalls.
            BuiltinFnDecls.push_back({Callee, false});
          }
        }
      }
    }
  }

  // Copy builtin and callees to the target module
  ValueToValueMapTy ValueMap;
  SmallVector<ReturnInst *, 4> Returns;
  // Avoid linking errors.
  const auto DefaultLinkage = GlobalValue::LinkOnceAnyLinkage;

  // Declare the callees in the module if they don't already exist.
  for (auto [Callee, IsImplicit] : Callees) {
    const auto Linkage = Callee->isIntrinsic() || Callee->isDeclaration()
                             ? Callee->getLinkage()
                             : DefaultLinkage;

    auto *NewCallee = M.getFunction(Callee->getName());
    if (!NewCallee) {
      auto *FnTy = Callee->getFunctionType();
      if (StructMap) {
        // We need to remap any image types for function arguments
        auto *RetTy = StructMap->remapType(FnTy->getReturnType());
        SmallVector<Type *, 4> ParamTys;
        for (auto *ParamTy : FnTy->params()) {
          ParamTys.push_back(StructMap->remapType(ParamTy));
        }
        FnTy = FunctionType::get(RetTy, ParamTys, FnTy->isVarArg());
      }

      NewCallee = Function::Create(FnTy, Linkage, Callee->getName(), &M);
      NewCallee->setCallingConv(Callee->getCallingConv());
    } else {
      NewCallee->setLinkage(Linkage);
    }

    auto NewArgIterator = NewCallee->arg_begin();
    for (Argument &Arg : Callee->args()) {
      NewArgIterator->setName(Arg.getName());
      ValueMap[&Arg] = &*(NewArgIterator++);
    }

    NewCallee->copyAttributesFrom(Callee);
    ValueMap[Callee] = NewCallee;

    if (IsImplicit) {
      llvm::appendToCompilerUsed(M, {NewCallee});
    }
  }

  // Clone the callees' bodies into the module.
  GlobalVarMaterializer GVMaterializer(M);

  for (auto [Callee, IsImplicit] : Callees) {
    // if the function isn't an intrinsic or a declaration
    if (!Callee->isIntrinsic() && !Callee->isDeclaration()) {
      auto NewCallee = cast<Function>(ValueMap[Callee]);
      auto Changes = NewCallee->getParent() != Callee->getParent()
                         ? CloneFunctionChangeType::DifferentModule
                         : CloneFunctionChangeType::LocalChangesOnly;
      CloneFunctionInto(NewCallee, Callee, ValueMap, Changes, Returns, "",
                        nullptr, StructMap, &GVMaterializer);
      Returns.clear();
    }
  }

  // Clone global variable initializers.
  for (auto *Var : GVMaterializer.getGlobalVariables()) {
    auto *NewVar = cast<GlobalVariable>(ValueMap[Var]);
    Constant *OldInit = Var->getInitializer();
    Constant *NewInit = MapValue(OldInit, ValueMap);
    NewVar->setInitializer(NewInit);
  }
}

PreservedAnalyses compiler::utils::LinkBuiltinsPass::run(
    Module &M, ModuleAnalysisManager &MAM) {
  auto &BI = MAM.getResult<BuiltinInfoAnalysis>(M);

  auto *BuiltinsModule = BI.getBuiltinsModule();
  // if we don't actually have a builtins module
  if (nullptr == BuiltinsModule) {
    return PreservedAnalyses::all();
  }

  SmallVector<std::pair<Function *, bool>> BuiltinFnDecls;

  // Intrinsics that may be lowered to a libcall must ensure that the
  // corresponding library function is pulled in. For RISC-V, we do that here.
  // For other targets, the host version will be provided later.
  const bool IncludeIntrinsicLibcalls =
      llvm::Triple(M.getTargetTriple()).isRISCV();
  if (IncludeIntrinsicLibcalls) {
    const llvm::StringRef IntrinsicFunctions[] = {"memcpy", "memmove",
                                                  "memset"};
    for (auto IntrinsicFunction : IntrinsicFunctions) {
      BuiltinFnDecls.push_back(
          {BuiltinsModule->getFunction(IntrinsicFunction), true});
    }
  }

  for (auto &F : M.functions()) {
    if (F.isIntrinsic()) {
      continue;
    }
    auto BuiltinF = BuiltinsModule->getFunction(F.getName());
    if (F.isDeclaration() && nullptr != BuiltinF) {
      BuiltinFnDecls.push_back({BuiltinF, F.isIntrinsic()});
    }
  }

  if (BuiltinFnDecls.empty()) {
    return PreservedAnalyses::all();
  }

  StructMap Map;
  cloneStructs(M, *BuiltinsModule, Map);
  StructTypeRemapper structMap(Map);
  cloneBuiltins(M, *BuiltinsModule, BuiltinFnDecls,
                Map.empty() ? nullptr : &structMap);

  return PreservedAnalyses::none();
}
