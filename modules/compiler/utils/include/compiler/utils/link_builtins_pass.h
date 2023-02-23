// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// Link builtins pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED
#define COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED

#include <compiler/utils/StructTypeRemapper.h>
#include <llvm/IR/PassManager.h>

namespace compiler {
namespace utils {
/// @brief A pass for linking builtins to the current module
///
/// This pass will manually link in any functions required from a given
/// `builtins` module, into the current module.
class LinkBuiltinsPass final : public llvm::PassInfoMixin<LinkBuiltinsPass> {
 public:
  ///
  /// @param EarlyLinking Flag to indicate this is run before the vectorizer
  /// (vecz) so should allow relevant builtins through, e.g., get_global_id.
  LinkBuiltinsPass(bool EarlyLinking) : EarlyLinking(EarlyLinking) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);

 private:
  void cloneStructs(llvm::Module &M, llvm::Module &BuiltinsModule,
                    compiler::utils::StructMap &Map);
  void cloneBuiltins(llvm::Module &M, llvm::Module &BuiltinsModule,
                     llvm::SmallVectorImpl<llvm::Function *> &BuiltinFnDecls,
                     compiler::utils::StructTypeRemapper *StructMap);

  bool EarlyLinking = false;
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_LINK_BUILTINS_PASS_H_INCLUDED
