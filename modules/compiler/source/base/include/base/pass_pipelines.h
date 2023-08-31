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

/// @file
///
/// @brief Functions to help build compiler pipelines

#ifndef BASE_PASS_PIPELINES_H_INCLUDED
#define BASE_PASS_PIPELINES_H_INCLUDED

#include <compiler/module.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/PassManager.h>

#include <optional>

namespace llvm {
class Module;
class PassBuilder;
class TargetMachine;
}  // namespace llvm

namespace compiler {

using addPassFunc = std::function<void(llvm::ModulePassManager &PM)>;

/// @brief A utility class to help orchestrate and tweak the creation of
/// compiler pass pipelines in a target-specific way. It holds any state that
/// is commonly used to either conditionally schedule or configure ComputeMux
/// compiler passes.
///
/// Users may re-use this class to define their own pass pipeline components
/// that fit in with the pre-defined components.
struct BasePassPipelineTuner {
  explicit BasePassPipelineTuner(const compiler::Options &opts)
      : options(opts) {}

  /// @brief The build options being compiled for.
  compiler::Options options;

  /// @brief Whether or not to generate code for degenerate sub groups.
  bool degenerate_sub_groups = false;

  /// @brief Whether or not the WorkItemLoopsPass is going to be run.
  bool handling_work_item_loops = true;

  /// @brief The desired target calling convention, used to configure the
  /// FixupCallingConvention pass.
  llvm::CallingConv::ID calling_convention = llvm::CallingConv::C;
};

/// @brief Adds passes which are both required and recommended for use before
/// scheduling the vecz RunVeczPass pass.
///
/// None require that the vecz pass is actually scheduled; some of these may be
/// beneficial without it.
void addPreVeczPasses(llvm::ModulePassManager &PM,
                      const BasePassPipelineTuner &tuner);

/// @brief Adds passes to link in a builtins Module, followed by
/// additional passes to materialize any missing ones, optimize them with
/// inline IR replacements, provide definitions of certain mux builtins, etc.
///
/// The passes help to prepare the module for the final round of mux work-group
/// scheduling passes and optimizations.
void addLateBuiltinsPasses(llvm::ModulePassManager &PM,
                           const BasePassPipelineTuner &tuner);

/// @brief Adds the standard set of passes which prepare kernels to be
/// scheduled across work-groups.
///
/// This pipeline adds mux scheduling structures as parameters to functions in
/// the module and materializes the mux work-group and work-item builtins to
/// read/write shared state via those structures.
void addPrepareWorkGroupSchedulingPasses(llvm::ModulePassManager &PM);

/// @brief Adds stock LLVM per-module optimization passes. Roughly equivalent
/// to the O0 and O3 default IR optimization pipelines.
void addLLVMDefaultPerModulePipeline(llvm::ModulePassManager &PM,
                                     llvm::PassBuilder &PB,
                                     const compiler::Options &options);

/// @brief Invokes the LLVM backend to produce an object binary
///
/// @param M Module to compile
/// @param TM TargetMachine to compile for
/// @param ostream Stream to write the object binary to
/// @param create_assembly true to return a textual assembly file, false to
/// create a binary object
/// @return Result of the backend compilation
Result emitCodeGenFile(llvm::Module &M, llvm::TargetMachine *TM,
                       llvm::raw_pwrite_stream &ostream,
                       bool create_assembly = false);

void encodeVectorizationMode(llvm::Function &, VectorizationMode);
std::optional<VectorizationMode> getVectorizationMode(const llvm::Function &);

}  // namespace compiler

#endif  // BASE_PASS_PIPELINES_H_INCLUDED
