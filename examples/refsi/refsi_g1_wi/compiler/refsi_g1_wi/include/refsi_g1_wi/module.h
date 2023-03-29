// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_G1_MODULE_H_INCLUDED
#define REFSI_G1_MODULE_H_INCLUDED

#include <base/context.h>
#include <mux/mux.hpp>
#include <riscv/module.h>

namespace refsi_g1_wi {

class RefSiG1Target;

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class RefSiG1Module final : public riscv::RiscvModule {
 public:
  RefSiG1Module(RefSiG1Target &target, compiler::BaseContext &context,
                uint32_t &num_errors, std::string &log);

  /// @see Module::createPassMachinery
  std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery()
      override;

  /// @see Module::getLateTargetPasses
  llvm::ModulePassManager getLateTargetPasses(
      compiler::utils::PassMachinery &pass_mach) override;

};  // class RefSiG1Module
}  // namespace refsi_g1_wi

#endif  // REFSI_G1_MODULE_H_INCLUDED
