// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Add Metadata Pass.
///
/// NOTE: This pass should be run after
/// compiler::utils::ComputeLocalMemoryUsagePass() so that the correct value for
/// local_memory is encoded into the serialized metadata. If the pass is not run
/// or run after this pass has completed, the value of local_memory_usage will
/// be encoded as 0.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED
#define COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED

#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/metadata_hooks.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {

template <class AnalysisTy, class HandlerTy>
class AddMetadataPass
    : public llvm::PassInfoMixin<AddMetadataPass<AnalysisTy, HandlerTy>> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    md_hooks hooks = getElfMetadataWriteHooks();
    HandlerTy handler;
    if (!handler.init(&hooks, &M)) {
      return llvm::PreservedAnalyses::none();
    }

    llvm::FunctionAnalysisManager &FAM =
        MAM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();

    for (auto &Fn : M.functions()) {
      if (!isKernelEntryPt(Fn)) {
        continue;
      }
      auto KernelInfo = FAM.getResult<AnalysisTy>(Fn);
      if (!handler.write(KernelInfo)) {
        return llvm::PreservedAnalyses::none();
      }
    }

    if (!handler.finalize()) {
      return llvm::PreservedAnalyses::none();
    }

    return llvm::PreservedAnalyses::all();
  }
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_ADD_METADATA_PASS_H_INCLUDED
