// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Class CheckForExtFuncsPass
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BASE_CHECK_FOR_EXT_FUNCS_PASS_H_INCLUDED
#define BASE_CHECK_FOR_EXT_FUNCS_PASS_H_INCLUDED

#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/PassManager.h>

#include <string>

namespace llvm {
class DiagnosticPrinter;
}

namespace compiler {

struct DiagnosticInfoExternalFunc : public llvm::DiagnosticInfo {
  static int DK_ExternalFunc;

  DiagnosticInfoExternalFunc(llvm::StringRef FName)
      : llvm::DiagnosticInfo(DK_ExternalFunc, llvm::DS_Error), FName(FName) {}

  std::string formatMessage() const;

  void print(llvm::DiagnosticPrinter &) const override;

  static bool classof(const llvm::DiagnosticInfo *DI) {
    return DI->getKind() == DK_ExternalFunc;
  }

 private:
  std::string FName;
};

/// @addtogroup cl_compiler
/// @{

/// @brief Pass to check for unavailable external functions.
///
/// All unavailable external functions are raised as DiagnosticInfoExternalFunc
/// diagnostics with error-level severity. The currently installed diagnostic
/// handler is responsible for handling them. It may abort, it may log an error
/// and continue, or it may ignore them completely; there are no requirements
/// imposed by ComputeMux.
///
/// Note that the compilation pipeline will continue after this pass unless the
/// diagnostic stops it.
struct CheckForExtFuncsPass final
    : public llvm::PassInfoMixin<CheckForExtFuncsPass> {
  /// @brief The entry point to the pass.
  ///
  /// @param M The Module provided to the pass.
  /// @param AM Manager providing analyses.
  ///
  /// @return Return whether or not the pass changed anything (always false for
  /// this pass).
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

/// @}
}  // namespace compiler

#endif  // BASE_CHECK_FOR_EXT_FUNCS_PASS_H_INCLUDED
