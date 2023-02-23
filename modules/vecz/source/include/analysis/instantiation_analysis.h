// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_ANALYSIS_INSTANTIATION_ANALYSIS_H_INCLUDED
#define VECZ_ANALYSIS_INSTANTIATION_ANALYSIS_H_INCLUDED

namespace llvm {
class Instruction;
}  // namespace llvm

namespace vecz {
class VectorizationContext;

/// @brief Determine whether the given instruction needs to be instantiated.
///
/// @param[in] CTx the vectorization context
/// @param[in] I Instruction to analyze.
///
/// @return true iff the instruction requires instantiation.
bool needsInstantiation(VectorizationContext const &Ctx, llvm::Instruction &I);
};  // namespace vecz

#endif  // VECZ_ANALYSIS_INSTANTIATION_ANALYSIS_H_INCLUDED
