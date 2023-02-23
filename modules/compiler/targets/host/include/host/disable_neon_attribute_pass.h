// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Disable 'neon' attribute pass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED
#define HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>

namespace host {

/// @brief A pass that will disables NEON support from all functions in the
/// module if the possibility of hitting a 64-bit ARM issue is encountered in
/// the IR.
/// The specific issue is that NEON vector conversions from i64 -> float do the
/// conversion in two stages: i64 -> double then double -> float. This loses
/// precision because of incorrect rounding in the intermediate value.
class DisableNeonAttributePass final
    : public llvm::PassInfoMixin<DisableNeonAttributePass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};
}  // namespace host

#endif  // HOST_DISABLE_NEON_ATTRIBUTE_PASS_H_INCLUDED
