// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// RefSi M1 LLVM passes.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_M1_REPLACE_MUX_DMA_PASS_H_INCLUDED
#define REFSI_M1_REPLACE_MUX_DMA_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace refsi_m1 {

/// This provides a RefSi-specific solution for replacing mux DMA intrinsics.
/// These DMA intrinsics are `__mux_dma_read_1D`, `__mux_dma_write_1D`,
/// `__mux_dma_read_2D`, `__mux_dma_write_2D`, `__mux_dma_read_3D`,
/// `__mux_dma_write_3D` and `__mux_dma_wait`. After running the pass, these
/// functions have a body that start DMA transfers and wait for DMA transfers to
/// finish through the RefSi DMA interface. Since this interface is based on
/// memory-mapped registers, using this pass when targeting RISC-V
/// implementations other than RefSi M1 would likely result in traps when
/// executing kernels.
class RefSiM1ReplaceMuxDmaPass final
    : public llvm::PassInfoMixin<RefSiM1ReplaceMuxDmaPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

};  // namespace refsi_m1

#endif  // REFSI_M1_REPLACE_MUX_DMA_PASS_H_INCLUDED
