// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file
///
/// @brief Compiler program module API.

#ifndef HOST_MODULE_H_INCLUDED
#define HOST_MODULE_H_INCLUDED

#include <base/module.h>
#include <builtins/printf.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <compiler/module.h>
#include <compiler/utils/pass_machinery.h>
#include <llvm/IR/Module.h>
#include <loader/elf.h>
#include <loader/mapper.h>

#include <mutex>

namespace host {

class HostKernel;
class HostTarget;

/// @brief Stores the metadata for a kernel.
struct KernelMetadata {
  std::string name;
  uint32_t local_memory_used;
  uint32_t work_width;
};

/// @brief A free function implementing
/// HostModule::initializePassMachineryForFinalize
void initializePassMachineryForFinalize(
    compiler::utils::PassMachinery &passMach, const HostTarget &target);

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class HostModule : public compiler::BaseModule {
public:
  HostModule(compiler::BaseTarget &target, compiler::BaseContext &context,
             uint32_t &num_errors, std::string &log);

  HostModule(const HostModule &) = delete;
  HostModule &operator=(const HostModule &) = delete;

  /// @see Module::createBinary
  compiler::Result
  createBinary(cargo::array_view<std::uint8_t> &buffer) override;

  /// @see Module::buildTargetPipeline
  llvm::ModulePassManager
  getLateTargetPasses(compiler::utils::PassMachinery &) override;

  /// @see BaseModule::createKernel
  compiler::Kernel *createKernel(const std::string &name) override;

  /// @see BaseModule::createPassMachinery
  std::unique_ptr<compiler::utils::PassMachinery>
  createPassMachinery(llvm::LLVMContext &) override;

  /// @see BaseModule::initializePassMachineryForFinalize
  void initializePassMachineryForFinalize(
      compiler::utils::PassMachinery &passMach) const override;

private:
  /// @brief Compiled object code compiler from the LLVM module.
  cargo::dynamic_array<uint8_t> object_code;

  const HostTarget &getHostTarget() const;

  /// @brief Compiles the input LLVM module into an ELF binary.
  ///
  /// @param target Target to compile the module for.
  /// @param build_options Build options that will affect optimizations
  /// performed.
  /// @param module Module to compile, needs to have been finalized (i.e.
  /// `BaseModule::finalize` has been called).
  ///
  /// @return Cargo dynamic array containing the ELF binary.
  cargo::expected<cargo::dynamic_array<uint8_t>, compiler::Result>
  hostCompileObject(HostTarget &target, const compiler::Options &build_options,
                    llvm::Module *module);
}; // class Module
} // namespace host

#endif // HOST_MODULE_H_INCLUDED
