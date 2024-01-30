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

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/device_info.h>
#include <compiler/utils/replace_mux_math_decls_pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/triple.h>

using namespace llvm;

PreservedAnalyses compiler::utils::ReplaceMuxMathDeclsPass::run(
    Module &M, ModuleAnalysisManager &MAM) {
  bool Changed = false;

  const Triple TT(M.getTargetTriple());
  auto &DI = MAM.getResult<DeviceInfoAnalysis>(M);

  // Abacus needs to know if denormal floats are supported, so it can avoid
  // Flush To Zero behaviour in intermediate operations of maths builtins.
  //
  // We make the assumption in abacus that double precision always supports
  // denormals, but if either single and half precision do not we set the FTZ
  // flag.
  bool flush_denorms_to_zero = false;
  bool is_embedded_profile = false;
  if (DI.float_capabilities) {
    flush_denorms_to_zero |= (0 == (DI.float_capabilities &
                                    device_floating_point_capabilities_denorm));

    is_embedded_profile |= (0 == (DI.float_capabilities &
                                  device_floating_point_capabilities_full));
  }

  if (DI.half_capabilities) {
    flush_denorms_to_zero |= (0 == (DI.half_capabilities &
                                    device_floating_point_capabilities_denorm));

    is_embedded_profile |=
        (0 == (DI.half_capabilities & device_floating_point_capabilities_full));
  }

  const std::pair<StringRef, const bool> MuxMathDecls[] = {
      {MuxBuiltins::isftz, flush_denorms_to_zero},
      {MuxBuiltins::usefast, UseFast},
      {MuxBuiltins::isembeddedprofile, is_embedded_profile},
  };

  for (const auto &MathDecl : MuxMathDecls) {
    if (Function *const Fn = M.getFunction(MathDecl.first)) {
      if (Fn->size()) {
        // If the function already has a definition, do not attempt to redefine
        // it. This pass may run multiple times.
        continue;
      }

      Changed = true;

      if (Fn->hasFnAttribute(Attribute::NoInline)) {
        report_fatal_error(
            "internal error: Mux internal function declared with noinline "
            "attribute");
      }

      Fn->addFnAttr(Attribute::AlwaysInline);

      // create an IR builder with a single basic block in our function
      IRBuilder<> IR(BasicBlock::Create(Fn->getContext(), "", Fn));

      IR.CreateRet(IR.getInt1(MathDecl.second));
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
