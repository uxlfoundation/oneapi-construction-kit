// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef RISCV_PASS_MACHINERY_H_INCLUDED
#define RISCV_PASS_MACHINERY_H_INCLUDED

#include <base/base_pass_machinery.h>

namespace riscv {

/// @brief Version of PassMachinery used in risc-v
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class RiscvPassMachinery : public compiler::BaseModulePassMachinery {
 public:
  RiscvPassMachinery(
      llvm::TargetMachine *TM, const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses);

  void addClassToPassNames() override;

  void registerPasses() override;

  void registerPassCallbacks() override;

  void printPassNames(llvm::raw_ostream &OS) override;
};

}  // namespace riscv

#endif  // RISCV_PASS_MACHINERY_H_INCLUDED
