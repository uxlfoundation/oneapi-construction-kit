// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef MULTI_LLVM_VECTOR_TYPE_HELPER_H_INCLUDED
#define MULTI_LLVM_VECTOR_TYPE_HELPER_H_INCLUDED

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/TypeSize.h>
#include <multi_llvm/llvm_version.h>

namespace multi_llvm {

// Vector Reduce Intrinsics IDs differentiate between LLVM versions. LLVM 12
// now promotes them as first-class residents and drops the experimental bit.
namespace Intrinsic {
static constexpr const auto vector_reduce_and =
    llvm::Intrinsic::vector_reduce_and;
static constexpr const auto vector_reduce_or =
    llvm::Intrinsic::vector_reduce_or;
static constexpr const auto vector_reduce_xor =
    llvm::Intrinsic::vector_reduce_xor;
static constexpr const auto vector_reduce_add =
    llvm::Intrinsic::vector_reduce_add;
static constexpr const auto vector_reduce_mul =
    llvm::Intrinsic::vector_reduce_mul;
static constexpr const auto vector_reduce_fadd =
    llvm::Intrinsic::vector_reduce_fadd;
static constexpr const auto vector_reduce_fmul =
    llvm::Intrinsic::vector_reduce_fmul;
static constexpr const auto vector_reduce_fmax =
    llvm::Intrinsic::vector_reduce_fmax;
static constexpr const auto vector_reduce_smax =
    llvm::Intrinsic::vector_reduce_smax;
static constexpr const auto vector_reduce_umax =
    llvm::Intrinsic::vector_reduce_umax;
static constexpr const auto vector_reduce_fmin =
    llvm::Intrinsic::vector_reduce_fmin;
static constexpr const auto vector_reduce_smin =
    llvm::Intrinsic::vector_reduce_smin;
static constexpr const auto vector_reduce_umin =
    llvm::Intrinsic::vector_reduce_umin;
}  // namespace Intrinsic

// LLVM 11 removes CompositeType and SequentialType classes, so this
// is just a helper method to check for CA supported sequential types
static inline bool isSequentialType(llvm::Type::TypeID TyID) {
  return TyID == llvm::Type::ArrayTyID || TyID == llvm::Type::FixedVectorTyID ||
         TyID == llvm::Type::ScalableVectorTyID;
}

// The functions defined below are common functions to allow us to generically
// get VectorType information from a base Type class, due to either deprecation
// or removal of these in LLVM 11 (result of scalable/fixed vectors separation)

inline llvm::Type *getVectorElementType(llvm::Type *ty) {
  assert(llvm::isa<llvm::VectorType>(ty) && "Not a vector type");
  return llvm::cast<llvm::VectorType>(ty)->getElementType();
}
inline llvm::Type *getVectorElementType(const llvm::Type *ty) {
  assert(llvm::isa<llvm::VectorType>(ty) && "Not a vector type");
  return llvm::cast<llvm::VectorType>(ty)->getElementType();
}

inline unsigned getVectorNumElements(llvm::Type *ty) {
  assert(ty->getTypeID() == llvm::Type::FixedVectorTyID &&
         "Not a fixed vector type");
  return llvm::cast<llvm::FixedVectorType>(ty)
      ->getElementCount()
      .getFixedValue();
}
inline unsigned getVectorNumElements(const llvm::Type *ty) {
  assert(ty->getTypeID() == llvm::Type::FixedVectorTyID &&
         "Not a fixed vector type");
  return llvm::cast<llvm::FixedVectorType>(ty)
      ->getElementCount()
      .getFixedValue();
}

inline llvm::ElementCount getVectorElementCount(llvm::Type *ty) {
  return llvm::cast<llvm::VectorType>(ty)->getElementCount();
}
inline llvm::ElementCount getVectorElementCount(const llvm::Type *ty) {
  return llvm::cast<llvm::VectorType>(ty)->getElementCount();
}

inline unsigned getVectorKnownMinNumElements(llvm::Type *ty) {
  return getVectorElementCount(ty).getKnownMinValue();
}

inline unsigned getVectorKnownMinNumElements(const llvm::Type *ty) {
  return getVectorElementCount(ty).getKnownMinValue();
}

inline bool isScalableVectorTy(llvm::Type *ty) {
  return llvm::isa<llvm::ScalableVectorType>(ty);
}
inline bool isScalableVectorTy(const llvm::Type *ty) {
  return llvm::isa<llvm::ScalableVectorType>(ty);
}
}  // namespace multi_llvm

#endif  // MULTI_LLVM_VECTOR_TYPE_HELPER_H_INCLUDED
