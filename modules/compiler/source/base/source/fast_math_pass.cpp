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

#include <base/fast_math_pass.h>
#include <compiler/utils/metadata.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/InlineAdvisor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

using namespace llvm;

const StringMap<const char *> slowToFast = {
    // geometric builtins
    {"_Z6lengthf", "_Z11fast_lengthf"},
    {"_Z6lengthDv2_f", "_Z11fast_lengthDv2_f"},
    {"_Z6lengthDv3_f", "_Z11fast_lengthDv3_f"},
    {"_Z6lengthDv4_f", "_Z11fast_lengthDv4_f"},
    {"_Z6lengthd", "_Z11fast_lengthd"},
    {"_Z6lengthDv2_d", "_Z11fast_lengthDv2_d"},
    {"_Z6lengthDv3_d", "_Z11fast_lengthDv3_d"},
    {"_Z6lengthDv4_d", "_Z11fast_lengthDv4_d"},
    {"_Z9normalizef", "_Z14fast_normalizef"},
    {"_Z9normalizeDv2_f", "_Z14fast_normalizeDv2_f"},
    {"_Z9normalizeDv3_f", "_Z14fast_normalizeDv3_f"},
    {"_Z9normalizeDv4_f", "_Z14fast_normalizeDv4_f"},
    {"_Z9normalized", "_Z14fast_normalized"},
    {"_Z9normalizeDv2_d", "_Z14fast_normalizeDv2_d"},
    {"_Z9normalizeDv3_d", "_Z14fast_normalizeDv3_d"},
    {"_Z9normalizeDv4_d", "_Z14fast_normalizeDv4_d"},
    {"_Z8distanceff", "_Z13fast_distanceff"},
    {"_Z8distanceDv2_fS_", "_Z13fast_distanceDv2_fS_"},
    {"_Z8distanceDv3_fS_", "_Z13fast_distanceDv3_fS_"},
    {"_Z8distanceDv4_fS_", "_Z13fast_distanceDv4_fS_"},
    {"_Z8distancedd", "_Z13fast_distancedd"},
    {"_Z8distanceDv2_dS_", "_Z13fast_distanceDv2_dS_"},
    {"_Z8distanceDv3_dS_", "_Z13fast_distanceDv3_dS_"},
    {"_Z8distanceDv4_dS_", "_Z13fast_distanceDv4_dS_"},

    // standard math builtins
    {"_Z3cosf", "_Z10native_cosf"},
    {"_Z3cosDv2_f", "_Z10native_cosDv2_f"},
    {"_Z3cosDv3_f", "_Z10native_cosDv3_f"},
    {"_Z3cosDv4_f", "_Z10native_cosDv4_f"},
    {"_Z3cosDv8_f", "_Z10native_cosDv8_f"},
    {"_Z3cosDv16_f", "_Z10native_cosDv16_f"},
    {"_Z3expf", "_Z10native_expf"},
    {"_Z3expDv2_f", "_Z10native_expDv2_f"},
    {"_Z3expDv3_f", "_Z10native_expDv3_f"},
    {"_Z3expDv4_f", "_Z10native_expDv4_f"},
    {"_Z3expDv8_f", "_Z10native_expDv8_f"},
    {"_Z3expDv16_f", "_Z10native_expDv16_f"},
    {"_Z4exp2f", "_Z11native_exp2f"},
    {"_Z4exp2Dv2_f", "_Z11native_exp2Dv2_f"},
    {"_Z4exp2Dv3_f", "_Z11native_exp2Dv3_f"},
    {"_Z4exp2Dv4_f", "_Z11native_exp2Dv4_f"},
    {"_Z4exp2Dv8_f", "_Z11native_exp2Dv8_f"},
    {"_Z4exp2Dv16_f", "_Z11native_exp2Dv16_f"},
    {"_Z5exp10f", "_Z12native_exp10f"},
    {"_Z5exp10Dv2_f", "_Z12native_exp10Dv2_f"},
    {"_Z5exp10Dv3_f", "_Z12native_exp10Dv3_f"},
    {"_Z5exp10Dv4_f", "_Z12native_exp10Dv4_f"},
    {"_Z5exp10Dv8_f", "_Z12native_exp10Dv8_f"},
    {"_Z5exp10Dv16_f", "_Z12native_exp10Dv16_f"},
    {"_Z3logf", "_Z10native_logf"},
    {"_Z3logDv2_f", "_Z10native_logDv2_f"},
    {"_Z3logDv3_f", "_Z10native_logDv3_f"},
    {"_Z3logDv4_f", "_Z10native_logDv4_f"},
    {"_Z3logDv8_f", "_Z10native_logDv8_f"},
    {"_Z3logDv16_f", "_Z10native_logDv16_f"},
    {"_Z4log2f", "_Z11native_log2f"},
    {"_Z4log2Dv2_f", "_Z11native_log2Dv2_f"},
    {"_Z4log2Dv3_f", "_Z11native_log2Dv3_f"},
    {"_Z4log2Dv4_f", "_Z11native_log2Dv4_f"},
    {"_Z4log2Dv8_f", "_Z11native_log2Dv8_f"},
    {"_Z4log2Dv16_f", "_Z11native_log2Dv16_f"},
    {"_Z5log10f", "_Z12native_log10f"},
    {"_Z5log10Dv2_f", "_Z12native_log10Dv2_f"},
    {"_Z5log10Dv3_f", "_Z12native_log10Dv3_f"},
    {"_Z5log10Dv4_f", "_Z12native_log10Dv4_f"},
    {"_Z5log10Dv8_f", "_Z12native_log10Dv8_f"},
    {"_Z5log10Dv16_f", "_Z12native_log10Dv16_f"},
    {"_Z4powrff", "_Z11native_powrff"},
    {"_Z4powrDv2_fS_", "_Z11native_powrDv2_fS_"},
    {"_Z4powrDv3_fS_", "_Z11native_powrDv3_fS_"},
    {"_Z4powrDv4_fS_", "_Z11native_powrDv4_fS_"},
    {"_Z4powrDv8_fS_", "_Z11native_powrDv8_fS_"},
    {"_Z4powrDv16_fS_", "_Z11native_powrDv16_fS_"},
    {"_Z5rsqrtf", "_Z12native_rsqrtf"},
    {"_Z5rsqrtDv2_f", "_Z12native_rsqrtDv2_f"},
    {"_Z5rsqrtDv3_f", "_Z12native_rsqrtDv3_f"},
    {"_Z5rsqrtDv4_f", "_Z12native_rsqrtDv4_f"},
    {"_Z5rsqrtDv8_f", "_Z12native_rsqrtDv8_f"},
    {"_Z5rsqrtDv16_f", "_Z12native_rsqrtDv16_f"},
    {"_Z3sinf", "_Z10native_sinf"},
    {"_Z3sinDv2_f", "_Z10native_sinDv2_f"},
    {"_Z3sinDv3_f", "_Z10native_sinDv3_f"},
    {"_Z3sinDv4_f", "_Z10native_sinDv4_f"},
    {"_Z3sinDv8_f", "_Z10native_sinDv8_f"},
    {"_Z3sinDv16_f", "_Z10native_sinDv16_f"},
    {"_Z4sqrtf", "_Z11native_sqrtf"},
    {"_Z4sqrtDv2_f", "_Z11native_sqrtDv2_f"},
    {"_Z4sqrtDv3_f", "_Z11native_sqrtDv3_f"},
    {"_Z4sqrtDv4_f", "_Z11native_sqrtDv4_f"},
    {"_Z4sqrtDv8_f", "_Z11native_sqrtDv8_f"},
    {"_Z4sqrtDv16_f", "_Z11native_sqrtDv16_f"},
    {"_Z3tanf", "_Z10native_tanf"},
    {"_Z3tanDv2_f", "_Z10native_tanDv2_f"},
    {"_Z3tanDv3_f", "_Z10native_tanDv3_f"},
    {"_Z3tanDv4_f", "_Z10native_tanDv4_f"},
    {"_Z3tanDv8_f", "_Z10native_tanDv8_f"},
    {"_Z3tanDv16_f", "_Z10native_tanDv16_f"},
};

static bool markFPOperatorsFast(Module &M) {
  bool modified = false;

  for (auto &function : M.functions()) {
    for (auto &basicBlock : function) {
      for (auto &instruction : basicBlock) {
        if (isa<FPMathOperator>(instruction)) {
          instruction.setFast(true);
          modified = true;
        }
      }
    }
  }
  return modified;
}

static bool replaceFastMathCalls(Module &M) {
  // A list of call instructions and the name with which we'll be replacing them
  SmallVector<std::pair<CallInst *, const char *>, 8> replacements;

  for (auto &function : M.functions()) {
    for (auto &basicBlock : function) {
      for (auto &instruction : basicBlock) {
        // get the call instruction (or null)
        if (auto ci = dyn_cast<CallInst>(&instruction)) {
          const auto func = ci->getCalledFunction();
          assert(func && "builtin calls cannot be indirect");
          const auto name = func->getName();

          // look for the called function in our map and store it
          auto entry = slowToFast.find(name);
          if (entry == slowToFast.end()) {
            continue;
          }
          replacements.emplace_back(ci, entry->second);
        }
      }
    }
  }

  for (auto pair : replacements) {
    auto ci = pair.first;
    const auto newFuncName = pair.second;
    const auto ciFunction = ci->getCalledFunction();
    assert(ciFunction && "Could not find function");
    // look up the corresponding fast version of the function
    // and if the fast version isn't in the module (it wasn't called
    // explicitly) then we need to add it to the module
    auto newFunc = M.getFunction(newFuncName);

    if (!newFunc) {
      newFunc = Function::Create(ciFunction->getFunctionType(),
                                 ciFunction->getLinkage(), newFuncName, &M);

      newFunc->setCallingConv(ciFunction->getCallingConv());
    }
    SmallVector<OperandBundleDef, 0> bundles;
    ci->getOperandBundlesAsDefs(bundles);

    SmallVector<Value *, 8> args;
    for (auto &arg : ci->args()) {
      args.push_back(arg);
    }

    auto newCi = CallInst::Create(newFunc, args, bundles);
    newCi->insertBefore(ci->getIterator());
    newCi->takeName(ci);
    newCi->setCallingConv(ci->getCallingConv());
    ci->replaceAllUsesWith(newCi);
    ci->eraseFromParent();
  }
  return replacements.size();
}

namespace compiler {
PreservedAnalyses FastMathPass::run(Module &M, ModuleAnalysisManager &) {
  auto preserved = PreservedAnalyses::all();
  auto version = compiler::utils::getOpenCLVersion(M);
  // OpenCL 3.0 introduced stricter ULP requirements for relaxed math.
  // This pass inserts calls to fast_* and native_* functions. Depending on the
  // device, these may not have ULP guarantees at all, so the pass is only
  // valid under 1.2.
  if (version >= compiler::utils::OpenCLC30) {
    return preserved;
  }
  if (markFPOperatorsFast(M)) {
    preserved.abandon<InlineAdvisorAnalysis>();
  }
  if (replaceFastMathCalls(M)) {
    preserved.abandon<InlineAdvisorAnalysis>();
    preserved.abandon<CallGraphAnalysis>();
  }
  return preserved;
}
}  // namespace compiler
