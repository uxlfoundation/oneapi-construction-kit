// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_VECTORIZATION_HELPERS_H_INCLUDED
#define VECZ_VECTORIZATION_HELPERS_H_INCLUDED

#include <llvm/Support/TypeSize.h>

#include <string>

namespace llvm {
class Function;
class StringRef;
}  // namespace llvm

namespace vecz {
class VectorizationUnit;
class VectorizationChoices;

/// @brief Generate a name for the vectorized function, which depends on the
/// original function name and SIMD width.
///
/// @param[in] ScalarName Name of the original function.
/// @param[in] VF vectorization factor of the vectorized function.
/// @param[in] Choices choices used for vectorization
///
/// @return Name for the vectorized function.
std::string getVectorizedFunctionName(llvm::StringRef ScalarName,
                                      llvm::ElementCount VF,
                                      VectorizationChoices Choices);

/// @brief Clone the scalar function's body into the function to vectorize,
/// vectorizing function argument types where required.
///
/// @param[in] VU the Vectorization Unit of the scalar function to clone.
///
/// @return The cloned function.
llvm::Function *cloneFunctionToVector(VectorizationUnit const &VU);

/// @brief Create a copy of the scalar functions debug info metatadata
//         nodes and set the scope of the copied DI to the vectorized
//         function.
void cloneDebugInfo(VectorizationUnit const &VU);

/// @brief Clone OpenCL related metadata from the scalar kernel to the
/// vectorized one.
///
/// This function will copy any 'opencl.kernels' or
/// 'opencl.kernel_wg_size_info' metadata from the scalar kernel to the
/// vectorized one. Obviously, the kernel itself has to be cloned before
/// calling this function.
void cloneOpenCLMetadata(VectorizationUnit const &VU);
}  // namespace vecz

#endif  // VECZ_VECTORIZATION_HELPERS_H_INCLUDED
