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

#ifndef HOST_TARGET_H
#define HOST_TARGET_H

#include <base/context.h>
#include <base/target.h>
#include <compiler/module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <mux/mux.h>

#include <map>

namespace llvm {
class Module;
}  // namespace llvm

namespace host {
struct HostInfo;

constexpr const char HOST_SNAPSHOT_VECTORIZED[] = "vectorized";
constexpr const char HOST_SNAPSHOT_BARRIER[] = "barrier";
constexpr const char HOST_SNAPSHOT_SCHEDULED[] = "scheduled";

/// @brief Compiler target class.
class HostTarget : public compiler::BaseTarget {
 public:
  HostTarget(const HostInfo *compiler_info, compiler::Context *context,
             compiler::NotifyCallbackFn callback);

  /// @see BaseTarget::initWithBuiltins
  compiler::Result initWithBuiltins(
      std::unique_ptr<llvm::Module> builtins_module) override;

  /// @see Target::createModule
  std::unique_ptr<compiler::Module> createModule(uint32_t &num_errors,
                                                 std::string &log) override;

  /// @brief An execution engine that is used to generate kernels at runtime.
  std::shared_ptr<llvm::ExecutionEngine> engine;

  /// @brief An atomic uint64_t to ensure unique identifiers are used.
  ///
  /// This field is used to ensure that each kernel that is JIT'ed by the
  /// execution engine has a unique name. This identifier will be suffixed onto
  /// the kernel names, and incremented under the context's mutex lock such that
  /// no conflict should occur.
  uint64_t unique_identifier;

#ifdef CA_ENABLE_HOST_BUILTINS
  std::unique_ptr<llvm::Module> builtins_host;
#endif
};
}  // namespace host

#endif  // HOST_TARGET_H
