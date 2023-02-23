// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef MULTI_LLVM_OPAQUE_POINTERS_H_INCLUDED
#define MULTI_LLVM_OPAQUE_POINTERS_H_INCLUDED

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

namespace multi_llvm {
inline bool isOpaquePointerTy(llvm::Type *Ty) {
  if (auto *PTy = llvm::dyn_cast<llvm::PointerType>(Ty)) {
    return PTy->isOpaque();
  }
  return false;
}

inline bool isOpaqueOrPointeeTypeMatches(llvm::PointerType *PTy,
                                         llvm::Type *EltTy) {
#if LLVM_VERSION_MAJOR >= 15
  (void)EltTy;
  (void)PTy;
  assert(PTy->isOpaque() && "No support for typed pointers in LLVM 15+");
  return true;
#else
  return PTy->isOpaque() || PTy->getPointerElementType() == EltTy;
#endif
}

inline llvm::Type *getPtrElementType(llvm::PointerType *PTy) {
  if (PTy->isOpaque()) {
    return nullptr;
  }
#if LLVM_VERSION_MAJOR >= 15
  assert(false && "No support for typed pointers in LLVM 15+");
  return nullptr;
#else
  return PTy->getPointerElementType();
#endif
}

};  // namespace multi_llvm

#endif  // MULTI_LLVM_OPAQUE_POINTERS_H_INCLUDED
