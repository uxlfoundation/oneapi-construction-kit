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

#ifndef RISCV_TARGET_H_INCLUDED
#define RISCV_TARGET_H_INCLUDED

#include <base/target.h>
#include <compiler/module.h>
#include <hal_riscv.h>

#include <memory>

namespace riscv {

constexpr const char RISCV_SNAPSHOT_INPUT[] = "input";
constexpr const char RISCV_SNAPSHOT_VECTORIZED[] = "vectorized";
constexpr const char RISCV_SNAPSHOT_BARRIER[] = "barrier";
constexpr const char RISCV_SNAPSHOT_SCHEDULED[] = "scheduled";
constexpr const char RISCV_SNAPSHOT_BACKEND[] = "backend";

/// @brief Compiler target class.
class RiscvTarget : public compiler::BaseAOTTarget {
 public:
  RiscvTarget(const compiler::Info *compiler_info,
              const hal_device_info_riscv_t *hal_device_info,
              compiler::Context *context, compiler::NotifyCallbackFn callback);
  ~RiscvTarget();

  /// @see BaseTarget::initWithBuiltins
  compiler::Result initWithBuiltins(
      std::unique_ptr<llvm::Module> builtins_module) override;

  /// @see BaseTarget::createModule
  std::unique_ptr<compiler::Module> createModule(uint32_t &num_errors,
                                                 std::string &log) override;

  /// @brief debug prefix for environment variables e.g. CA_RISCV
  std::string env_debug_prefix;
  /// @brief llvm target triple e.g. riscv64-unknown-elf
  std::string llvm_triple;
  /// @brief llvm target cpu e.g. generic-rv64
  std::string llvm_cpu;
  /// @brief llvm target ABI e.g. lp64d
  std::string llvm_abi;
  /// @brief comma separated feature list e.g. +f,+d,+c
  std::string llvm_features;
  /// @brief pointer to runtime lib or nullptr if none
  const uint8_t *rt_lib = nullptr;
  /// @brief size in bytes of runtime lib or zero if none
  size_t rt_lib_size = 0;
  /// @brief the HAL device info for riscv target
  const hal_device_info_riscv_t *riscv_hal_device_info;
  /// @brief Target-configurable snapshot stage names
  std::vector<std::string> available_snapshots;
};
}  // namespace riscv

#endif  // RISCV_TARGET_H_INCLUDED
