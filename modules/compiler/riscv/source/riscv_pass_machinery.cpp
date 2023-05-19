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

#include <base/pass_pipelines.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Passes/PassBuilder.h>
#include <multi_llvm/optional_helper.h>
#include <riscv/ir_to_builtins_pass.h>
#include <riscv/riscv_pass_machinery.h>
#include <vecz/pass.h>

riscv::RiscvPassMachinery::RiscvPassMachinery(
    llvm::LLVMContext &Ctx, llvm::TargetMachine *TM,
    const compiler::utils::DeviceInfo &Info,
    compiler::utils::BuiltinInfoAnalysis::CallbackFn BICallback,
    bool verifyEach, compiler::utils::DebugLogging debugLogLevel,
    bool timePasses)
    : compiler::BaseModulePassMachinery(Ctx, TM, Info, BICallback, verifyEach,
                                        debugLogLevel, timePasses) {}

// Process vecz flags based off build options and environment variables
// return true if we want to vectorize
llvm::SmallVector<vecz::VeczPassOptions> processVeczFlags(
    llvm::Optional<compiler::VectorizationMode> vecz_mode) {
  llvm::SmallVector<vecz::VeczPassOptions> vecz_options_vec;
  vecz::VeczPassOptions vecz_opts;
  // The minimum number of elements to vectorize for. For a fixed-length VF,
  // this is the exact number of elements to vectorize by. For scalable VFs,
  // the actual number of elements is a multiple (vscale) of these, unknown at
  // compile time. Default taken from config. May be overriden later.
  vecz_opts.factor = compiler::utils::VectorizationFactor::getScalar();

  vecz_opts.choices.enable(vecz::VectorizationChoices::eDivisionExceptions);

  vecz_opts.vecz_auto = vecz_mode == compiler::VectorizationMode::AUTO;
  vecz_opts.vec_dim_idx = 0;

  // This is of the form of a comma separated set of fields
  // S     - use scalable vectorization
  // V     - vectorize only, otherwise produce both scalar and vector kernels
  // A     - let vecz automatically choose the vectorization factor
  // 1-64  - vectorization factor multiplier: the fixed amount itself, or the
  //         value that multiplies the scalable amount
  // VP    - produce a vector-predicated kernel
  // VVP   - produce both a vectorized and a vector-predicated kernel
  bool add_vvp = false;
  if (const auto *vecz_vf_flags_env = std::getenv("CA_RISCV_VF")) {
    // Set scalable to off and let users add it explicitly with 'S'.
    vecz_opts.factor.setIsScalable(false);
    llvm::SmallVector<llvm::StringRef, 4> flags;
    llvm::StringRef vf_flags_ref(vecz_vf_flags_env);
    vf_flags_ref.split(flags, ',');
    for (auto r : flags) {
      if (r == "A" || r == "a") {
        vecz_opts.vecz_auto = true;
      } else if (r == "V" || r == "v") {
        // 'V is no longer a vectorization choice but is legally found as part
        // of this variable. See hasForceNoTail, below.
      } else if (r == "S" || r == "s") {
        vecz_opts.factor.setIsScalable(true);
      } else if (isdigit(r[0])) {
        vecz_opts.factor.setKnownMin(std::stoi(r.str()));
      } else if (r == "VP" || r == "vp") {
        vecz_opts.choices.enable(
            vecz::VectorizationChoices::eVectorPredication);
      } else if (r == "VVP" || r == "vvp") {
        // Add the vectorized pass option now (controlled by other iterations
        // of this loop), and flag that we have to add a vector-predicated form
        // later.
        add_vvp = true;
      } else {
        return {};
      }
    }
  }

  // Choices override the cost model
  const char *ptr = std::getenv("CODEPLAY_VECZ_CHOICES");
  if (ptr) {
    bool success = vecz_opts.choices.parseChoicesString(ptr);
    if (!success) {
      llvm::errs() << "failed to parse the CODEPLAY_VECZ_CHOICES variable\n";
    }
  }

  vecz_options_vec.push_back(vecz_opts);
  if (add_vvp) {
    vecz_opts.choices.enable(vecz::VectorizationChoices::eVectorPredication);
    vecz_options_vec.push_back(vecz_opts);
  }

  return vecz_options_vec;
}

bool riscvVeczPassOpts(llvm::Function &F, llvm::ModuleAnalysisManager &,
                       llvm::SmallVectorImpl<vecz::VeczPassOptions> &PassOpts) {
  auto vecz_mode = compiler::getVectorizationMode(F);
  if (!compiler::utils::isKernelEntryPt(F) ||
      F.hasFnAttribute(llvm::Attribute::OptimizeNone) ||
      vecz_mode == compiler::VectorizationMode::NEVER) {
    return false;
  }
  auto vecz_options_vec = processVeczFlags(vecz_mode);
  if (vecz_options_vec.empty()) {
    return false;
  }
  PassOpts.assign(vecz_options_vec);
  return true;
}

void riscv::RiscvPassMachinery::addClassToPassNames() {
  BaseModulePassMachinery::addClassToPassNames();
// Register compiler passes
#define MODULE_PASS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  PIC.addClassToPassName(decltype(CREATE_PASS)::name(), NAME);

#include "riscv_pass_registry.def"
}

void riscv::RiscvPassMachinery::registerPasses() {
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "riscv_pass_registry.def"
  compiler::BaseModulePassMachinery::registerPasses();
}

void riscv::RiscvPassMachinery::registerPassCallbacks() {
  BaseModulePassMachinery::registerPassCallbacks();
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::ModulePassManager &PM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) \
  if (Name == NAME) {                  \
    PM.addPass(CREATE_PASS);           \
    return true;                       \
  }
#include "riscv_pass_registry.def"
        return false;
      });
}

void riscv::RiscvPassMachinery::printPassNames(llvm::raw_ostream &OS) {
  BaseModulePassMachinery::printPassNames(OS);

  OS << "\nRisc-v specific Target passes:\n\n";
  OS << "Module passes:\n";
#define MODULE_PASS(NAME, CREATE_PASS) compiler::utils::printPassName(NAME, OS);
#include "riscv_pass_registry.def"

  OS << "Module analyses:\n";
#define MODULE_ANALYSIS(NAME, CREATE_PASS) \
  compiler::utils::printPassName(NAME, OS);
#include "riscv_pass_registry.def"
}
