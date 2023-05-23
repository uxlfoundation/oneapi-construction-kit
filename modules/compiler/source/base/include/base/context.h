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

#ifndef COMPILER_BASE_CONTEXT_H_INCLUDED
#define COMPILER_BASE_CONTEXT_H_INCLUDED

#include <cargo/array_view.h>
#include <compiler/context.h>
#include <compiler/utils/pass_machinery.h>
#include <mux/mux.h>

#include <mutex>

namespace compiler {
/// @addtogroup cl_compiler
/// @{

/// @brief Compiler context implementation.
class BaseContext : public Context {
 public:
  BaseContext();
  ~BaseContext();

  /// @brief Deleted move constructor.
  ///
  /// Also deletes the copy constructor and the assignment operators.
  BaseContext(BaseContext &&) = delete;

  /// @brief Checks if a binary stream is valid SPIR.
  ///
  /// @param binary View of the SPIR binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  bool isValidSPIR(cargo::array_view<const std::uint8_t> binary) override;

  /// @brief Checks if a binary stream is valid SPIR-V.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns `true` if the stream is valid, `false` otherwise.
  bool isValidSPIRV(cargo::array_view<const uint32_t> code) override;

  /// @brief Get a description of all of a SPIR-V modules specializable
  /// constants.
  ///
  /// @param code View of the SPIR-V binary stream.
  ///
  /// @return Returns a map of the modules specializable constants on success,
  /// otherwise returns an error string.
  cargo::expected<spirv::SpecializableConstantsMap, std::string>
  getSpecializableConstants(cargo::array_view<const uint32_t> code) override;

  /// @brief Locks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::lock`.
  void lock() override;

  /// @brief Attempts to acquire the lock on the underlying mutex, used to
  /// control access to the underlying LLVM context.
  ///
  /// Forwards to `std::mutex::try_lock`.
  ///
  /// @return Returns true if the lock was acquired, false otherwise.
  bool try_lock() override;

  /// @brief Unlocks the underlying mutex, used to control access to the
  /// underlying LLVM context.
  ///
  /// Forwards to `std::mutex::unlock`.
  void unlock() override;

  bool isLLVMVerifyEachEnabled() const { return llvm_verify_each; }

  bool isLLVMTimePassesEnabled() const { return llvm_time_passes; }

  compiler::utils::DebugLogging getLLVMDebugLoggingLevel() const {
    return llvm_debug_passes;
  }

 private:
  /// @brief Mutex for accessing the BaseContext.
  std::mutex base_context_mutex;

  /// @brief True if compiler passes should be individually verified.
  ///
  /// If false, the default is to verify before/after each pass pipeline.
  bool llvm_verify_each = false;

  /// @brief True if compiler passes should be individually timed, with a
  /// summary reported for each pipeline.
  bool llvm_time_passes = false;

  /// @brief Debug logging level used with compiler passes.
  compiler::utils::DebugLogging llvm_debug_passes =
      compiler::utils::DebugLogging::None;

};  // class ContextImpl
/// @}
}  // namespace compiler

#endif  // COMPILER_BASE_CONTEXT_H_INCLUDED
