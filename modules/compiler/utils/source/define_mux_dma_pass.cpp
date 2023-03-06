// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
