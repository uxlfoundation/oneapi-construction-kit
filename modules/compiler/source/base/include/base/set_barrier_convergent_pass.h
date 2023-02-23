// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class SetBarrierConvergentPass interface.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_SET_BARRIER_CONVERGENT_PASS_H_INCLUDED
#define BASE_SET_BARRIER_CONVERGENT_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Set the convergent attribute on the OpenCL barrier function.
///
/// Note that this is only required for SPIR kernels, OpenCL-C kernels get this
/// set by the implicit builtins header.
struct SetBarrierConvergentPass final
    : public llvm::PassInfoMixin<SetBarrierConvergentPass> {
  SetBarrierConvergentPass() = default;

  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] module The module provided to the pass.
  ///
  /// @return Return whether or not the pass changed anything.
  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &);
};

/// @}
}  // namespace compiler

#endif  // BASE_SET_BARRIER_CONVERGENT_PASS_H_INCLUDED
