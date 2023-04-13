// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_G1_PASS_MACHINERY_H_INCLUDED
#define REFSI_G1_PASS_MACHINERY_H_INCLUDED

#include <riscv/riscv_pass_machinery.h>

namespace refsi_g1_wi {

/// @brief Version of PassMachinery used in RefSi G1 architecture
/// @note This can be used to contain things that can be accessed
/// by various passes as we run through the passes.
class RefSiG1PassMachinery : public riscv::RiscvPassMachinery {
 public:
  RefSiG1PassMachinery(
      llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
      const compiler::utils::DeviceInfo &Info,
      compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
      bool verifyEach, compiler::utils::DebugLogging debugLogging,
      bool timePasses);

  void addClassToPassNames() override;
  void registerPassCallbacks() override;
  void printPassNames(llvm::raw_ostream &OS) override;
};

}  // namespace refsi_g1_wi

#endif  // REFSI_G1_PASS_MACHINERY_H_INCLUDED
