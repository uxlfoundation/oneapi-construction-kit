// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief DXIL's BuiltinInfo implementation.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_DXIL_BUILTIN_INFO_H_INCLUDED
#define COMPILER_UTILS_DXIL_BUILTIN_INFO_H_INCLUDED

#include "builtin_info.h"

namespace compiler {
namespace utils {
/// @addtogroup utils
/// @{

/// @brief Used by the vectorizer to manipulate and query information about
/// DXIL builtin functions.
class DXILBuiltinInfo final : public BILangInfoConcept {
 public:
  /// @see BuiltinInfo::isBuiltinUniform
  BuiltinUniformity isBuiltinUniform(Builtin const &B, const llvm::CallInst *CI,
                                     unsigned SimdDimIdx) const override;
  /// @see BuiltinInfo::analyzeBuiltin
  Builtin analyzeBuiltin(llvm::Function const &) const override;
  /// @see BuiltinInfo::getVectorEquivalent
  llvm::Function *getVectorEquivalent(Builtin const &B, unsigned Width,
                                      llvm::Module *M = nullptr) override;
  /// @see BuiltinInfo::getScalarEquivalent
  llvm::Function *getScalarEquivalent(Builtin const &B,
                                      llvm::Module *M) override;
  /// @see BuiltinInfo::emitBuiltinInline
  llvm::Value *emitBuiltinInline(llvm::Function *Builtin, llvm::IRBuilder<> &B,
                                 llvm::ArrayRef<llvm::Value *> Args) override;
  /// @see BuiltinInfo::getPrintfBuiltin
  BuiltinID getPrintfBuiltin() const override;

 private:
  BuiltinID identifyBuiltin(llvm::Function const &) const;

  llvm::Value *emitBuiltinInline(BuiltinID ID, llvm::IRBuilder<> &B,
                                 llvm::ArrayRef<llvm::Value *> Args);
};

/// @}
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_DXIL_BUILTIN_INFO_H_INCLUDED
