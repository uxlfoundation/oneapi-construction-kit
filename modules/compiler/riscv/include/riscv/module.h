// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef RISCV_MODULE_H_INCLUDED
#define RISCV_MODULE_H_INCLUDED

#include <base/context.h>
#include <base/module.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <mux/mux.hpp>

namespace compiler {
struct BasePassPipelineTuner;
}

namespace riscv {

class RiscvTarget;

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class RiscvModule : public compiler::BaseModule {
 public:
  RiscvModule(riscv::RiscvTarget &target, compiler::BaseContext &context,
              uint32_t &num_errors, std::string &log);

  /// @see Module::clear
  void clear() override;

  /// @see Module::createBinary
  compiler::Result createBinary(
      cargo::array_view<std::uint8_t> &buffer) override;

  /// @see Module::createPassMachinery
  std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery()
      override;

  /// @see BaseModule::initializePassMachineryForFrontend
  void initializePassMachineryForFrontend(
      compiler::utils::PassMachinery &,
      const clang::CodeGenOptions &) const override;

  /// @see BaseModule::initializePassMachineryForFinalize
  void initializePassMachineryForFinalize(
      compiler::utils::PassMachinery &) const override;

  /// @brief Stores the metadata for a kernel.
  struct KernelMetadata {
    std::string name;
    uint32_t local_memory_used;
    uint32_t subgroup_size;
  };

  /// @brief Gets or creates the TargetMachine to be used in the compilation of
  /// this module.
  llvm::TargetMachine *getTargetMachine();

 protected:
  /// @see BaseModule::getLateTargetPasses
  llvm::ModulePassManager getLateTargetPasses(
      compiler::utils::PassMachinery &) override;

  /// @see Module::createKernel
  compiler::Kernel *createKernel(const std::string &name) override;

  const riscv::RiscvTarget &getTarget() const;

  /// @brief Try to parse a string that specifies a list of stages to dump IR
  /// for.
  /// @param stages Vector to add parsed stage IDs to.
  /// @param dump_ir_string String containing the stage names to parse.
  /// @return Returns true if any stages were successfully parsed from the
  /// string.
  bool getStagesFromDumpIRString(std::vector<std::string> &stages,
                                 const char *dump_ir_string);

  /// @brief Add internal snapshots for all valid snapshot stages found in the
  /// stages string.
  /// @param stages Comma-separated string containing a list of snapshot stages.
  /// @return true if the stages string was successfully parsed or false
  /// otherwise.
  bool addIRSnapshotStages(const char *stages);

  /// @brief Add an 'internal' snapshot for the given stage, which is triggered
  /// through an environment variable rather than a Mux API function.
  ///
  /// @param stage Compilation stage to take the snapshot at.
  void addInternalSnapshot(const char *stage);

  /// @brief Take a 'backend' snapshot of the module at the current point. This
  /// compiles a clone of the module to assembly or an object file, depending
  /// on the snapshot.
  ///
  /// @param M Module to snapshot.
  /// @param TM TargetMachine to compile for
  /// @param snapshot Information needed to take the snapshot.
  void takeBackendSnapshot(
      llvm::Module *M, llvm::TargetMachine *TM,
      const compiler::BaseModule::SnapshotDetails &snapshot);

 protected:
  /// @brief Legacy helper function based off env variables to decode whether to
  /// force "no tail". Checks for comma separated 'V' in <env_debug_prefix>_VF
  /// @param env_debug_prefix
  /// @return true if found
  bool hasForceNoTail(const std::string &env_debug_prefix);

  /// @brief Legacy helper function based off env variables to decode whether to
  /// early linking is enabled. Checks for comma separated 'S' in
  /// <env_debug_prefix>_VF or <env_debug_prefix>_EARLY_LINK_BUILTINS being set
  /// to non-zero.
  /// @param env_debug_prefix
  /// @return true if found
  bool isEarlyBuiltinLinkingEnabled(const std::string &env_debug_prefix);

 private:
  cargo::dynamic_array<uint8_t> object_code;

  /// @brief Target machine to use to compile IR to assembly.
  std::unique_ptr<llvm::TargetMachine> target_machine;
};  // class RiscvModule
}  // namespace riscv

#endif  // RISCV_MODULE_H_INCLUDED
