// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// RISC-V IR-to-Builtin replacement pass.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED
#define RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>

namespace riscv {
/// @brief Pass mapping IR instructions to builtins.
///
/// Maps IR instructions to OpenCL builtins
/// Currently only supports frem and does not consider constant expressions
/// This avoids link errors for fmodf and fmod. Preferred solution is to
/// create a library we can link with.
struct IRToBuiltinReplacementPass final
    : public llvm::PassInfoMixin<IRToBuiltinReplacementPass> {
  bool replaceInstruction(llvm::Module &module, unsigned opcode,
                          llvm::StringRef name);

  llvm::PreservedAnalyses run(llvm::Module &module,
                              llvm::ModuleAnalysisManager &AM);

  IRToBuiltinReplacementPass() {}
};

}  // namespace riscv

#endif  // RISCV_IR_TO_BUILTINS_PASS_H_INCLUDED
