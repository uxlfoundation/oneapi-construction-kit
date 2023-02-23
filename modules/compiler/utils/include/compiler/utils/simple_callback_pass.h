// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// SimpleCallbackPass pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED
#define COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

namespace compiler {
namespace utils {

using CallbackFnTy = void(llvm::Module &);

/// @brief Invokes a callback with the module when run.
/// Important: all analyses must be preserved by the callback function.
class SimpleCallbackPass : public llvm::PassInfoMixin<SimpleCallbackPass> {
 public:
  /// @param C Callback function to invoke when the pass is run.
  template <typename CallableT>
  SimpleCallbackPass(CallableT C) : Callback(std::move(C)) {}

  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &) {
    Callback(module);
    return llvm::PreservedAnalyses::all();
  }

 private:
  llvm::unique_function<CallbackFnTy> Callback;
};

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_SIMPLE_CALLBACK_PASS_H_INCLUDED
