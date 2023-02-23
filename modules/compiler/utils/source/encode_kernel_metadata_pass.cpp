// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/attributes.h>
#include <compiler/utils/encode_kernel_metadata_pass.h>
#include <compiler/utils/metadata.h>

using namespace llvm;

PreservedAnalyses compiler::utils::TransferKernelMetadataPass::run(
    Module &M, ModuleAnalysisManager &) {
  SmallVector<KernelInfo, 4> Kernels;
  populateKernelList(M, Kernels);

  for (const auto &Kernel : Kernels) {
    if (auto *F = M.getFunction(Kernel.Name)) {
      setOrigFnName(*F);
      setIsKernelEntryPt(*F);
      if (Kernel.ReqdWGSize) {
        encodeLocalSizeMetadata(*F, *Kernel.ReqdWGSize);
      }
    }
  }

  return PreservedAnalyses::all();
}

PreservedAnalyses compiler::utils::EncodeKernelMetadataPass::run(
    Module &M, ModuleAnalysisManager &) {
  if (auto *F = M.getFunction(KernelName)) {
    setOrigFnName(*F);
    setIsKernelEntryPt(*F);
    if (LocalSizes) {
      encodeLocalSizeMetadata(*F, *LocalSizes);
    }
  }
  return PreservedAnalyses::all();
}
