// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_VECTORIZATION_HEURISTICS_H_INCLUDED
#define VECZ_VECTORIZATION_HEURISTICS_H_INCLUDED

#include <llvm/Support/TypeSize.h>

namespace llvm {
class Function;
}  // namespace llvm

namespace vecz {
class VectorizationContext;

/// @brief Decide whether a function is worth vectorizing for a given
/// vectorization factor.
///
/// @param[in] F the function to analyze
/// @param[in] Ctx the vectorization context
/// @param[in] VF the vectorization factor
/// @param[in] SimdDimIdx the vectorization dimension
///
/// @return Whether we should vectorize the function or not.
bool shouldVectorize(llvm::Function &F, VectorizationContext &Ctx,
                     llvm::ElementCount VF, unsigned SimdDimIdx);

}  // namespace vecz

#endif  // VECZ_VECTORIZATION_HEURISTICS_H_INCLUDED
