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
#ifndef COMPILER_TOOLS_MUXC_H_INCLUDED
#define COMPILER_TOOLS_MUXC_H_INCLUDED

#include <base/base_module_pass_machinery.h>
#include <base/context.h>
#include <base/target.h>
#include <mux/mux.hpp>

namespace muxc {

/// @brief Function return status
enum result : int { success = 0, failure = 1 };

/// @brief Parses pipelines to run LLVM passes provided from the target's Mux
/// Compiler
class driver {
 public:
  /// @brief Default constructor.
  driver() {}

  /// @brief Loads any arguments from command-line.
  void parseArguments(int argc, char **argv);

  /// @brief Initializes all of the compiler components and LLVMContext
  llvm::Error setupContext();

  /// @brief Converts the input file to an IR module.
  llvm::Expected<std::unique_ptr<llvm::Module>> convertInputToIR();

  /// @brief Create the pass machinery
  llvm::Expected<std::unique_ptr<compiler::utils::PassMachinery>>
  createPassMachinery();

  /// @brief Run the pass pipeline provided
  llvm::Error runPipeline(llvm::Module &, compiler::utils::PassMachinery &);

 private:
  uint32_t ModuleNumErrors = 0;
  std::string ModuleLog;
  /// @brief Selected compiler.
  const compiler::Info *CompilerInfo = nullptr;
  /// @brief Compiler context to drive compilation.
  std::unique_ptr<compiler::Context> CompilerContext;
  /// @brief Compiler target to drive compilation.
  std::unique_ptr<compiler::BaseTarget> CompilerTarget;
  /// @brief LLVM context. Used unless CompilerTarget is set, in which case we
  /// use its LLVMContext.
  std::unique_ptr<llvm::LLVMContext> LLVMCtx;
  /// @brief Compiler module being compiled.
  std::unique_ptr<compiler::Module> CompilerModule;

  /// @brief Find the desired `compiler::Info` from `device_name_substring`.
  ///
  /// @return Returns the compiler::Info matching the device, or an Error on
  /// failure.
  llvm::Expected<const compiler::Info *> findDevice();
};

}  // namespace muxc

#endif  // COMPILER_TOOLS_MUXC_H_INCLUDED
