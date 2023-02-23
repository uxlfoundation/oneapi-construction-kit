// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class BitShiftFixupPass interface.
///
/// Shift instructions need to be updated: SPIR defines them to have defined
/// results with oversized shift amounts but LLVM IR does not. 'modulo N'
/// needs to be performed to the shift amount prior to the shift, where N is
/// the bit width of the value to shift.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED
#define BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Update shift instructions to add a modulo when required.
struct BitShiftFixupPass final : public llvm::PassInfoMixin<BitShiftFixupPass> {
  BitShiftFixupPass() = default;

  /// @brief The entry point to the pass.
  ///
  /// @param[in,out] F The function provided to the pass.
  /// @param[in,out] FAM The analysis manager provided to the pass.
  ///
  /// @return Return analyses preserved by the pass
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);

  // This pass is not an optimization
  static bool isRequired() { return true; }
};

/// @}
}  // namespace compiler

#endif  // BASE_BIT_SHIFT_FIXUP_PASS_H_INCLUDED
