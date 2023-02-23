// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_M1_MODULE_H_INCLUDED
#define REFSI_M1_MODULE_H_INCLUDED

#include <base/context.h>
#include <mux/mux.hpp>
#include <riscv/module.h>

namespace refsi_m1 {

class RefSiM1Target;

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class RefSiM1Module final : public riscv::RiscvModule {
 public:
  RefSiM1Module(RefSiM1Target &target, compiler::BaseContext &context,
                uint32_t &num_errors, std::string &log);

  /// @see Module::createPassMachinery
  std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery()
      override;

  /// @see Module::getLateTargetPasses
  llvm::ModulePassManager getLateTargetPasses(
      compiler::utils::PassMachinery &pass_mach) override;

 private:
  /// @brief Add any final kernel passes
  /// @note This is run just before the AddMetadataPass
  void addFinalKernelPasses(llvm::ModulePassManager &PM);

  /// @brief Modify the tuner used for the compiler pipeline
  void modifyTuner(compiler::BasePassPipelineTuner &tuner);
};  // class RefSiM1Module
}  // namespace refsi_m1

#endif  // REFSI_M1_MODULE_H_INCLUDED
