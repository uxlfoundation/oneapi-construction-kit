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

#include <base/builtin_simplification_pass.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <multi_llvm/vector_type_helper.h>

#define ABACUS_LIBRARY_STATIC
#include <abacus/abacus_math.h>

#include <cmath>
#include <map>

using namespace llvm;

namespace {
double getConstantFPAsDouble(ConstantFP *const constant) {
  // are we looking at a 32 bit floating point number?
  const bool isFP32 = constant->getType()->isFloatTy();

  // get the APFloat for the constant
  const auto &apf = constant->getValueAPF();

  // get the value of our constant as a floating point
  return isFP32 ? apf.convertToFloat() : apf.convertToDouble();
}

bool powToX(Module &module, const std::map<std::string, std::string> &map,
            double (*modifier)(double)) {
  auto getIfPossible = [&](ConstantFP *const constant) -> ConstantInt * {
    // are we looking at a 32 bit floating point number?
    const bool isFP32 = constant->getType()->isFloatTy();

    // get the value of our constant as a floating point
    const double y = modifier(getConstantFPAsDouble(constant));

    // check if it is a whole number
    const bool isWholeNumber = (std::floor(y) == y);

    // 2^24 for float or INT_MAX for double is the largest number we can
    // represent
    const double cutoff = isFP32 ? 16777216.0 : 2147483647.0;

    // check if it is representable by the corresponding integer type
    const bool isLosslesslyRepresentable = (fabs(y) <= cutoff);

    if (isWholeNumber && isLosslesslyRepresentable) {
      // get the matching integer type for our float type
      auto newType = IntegerType::get(module.getContext(), 32);

      // get the constant integer for our y value
      return ConstantInt::getSigned(newType, static_cast<int64_t>(y));
    }

    // otherwise it is not possible and we return null
    return nullptr;
  };

  SmallVector<CallInst *, 8> calls;

  for (const auto &element : map) {
    if (auto func = module.getFunction(element.first)) {
      // the function is in the module, so we go through its uses now
      for (auto user : func->users()) {
        if (auto ci = dyn_cast<CallInst>(user)) {
          calls.push_back(ci);
        }
      }
    }
  }

  bool modified = false;

  for (auto ci : calls) {
    // get the called pow function
    const auto func = ci->getCalledFunction();
    assert(func && "Could not find builtin definition");

    // get the name of the called pow function
    const auto name = func->getName();

    // get the name of the corresponding pown function
    const auto newFuncName = map.at(name.str());

    auto &use = ci->getArgOperandUse(1);

    Constant *newY = nullptr;

    if (auto vec = dyn_cast<ConstantDataVector>(use)) {
      SmallVector<Constant *, 16> constants;

      // keep track of whether our constants array is valid
      bool isValid = true;

      for (unsigned i = 0, e = vec->getNumElements(); i < e; i++) {
        auto element = vec->getElementAsConstant(i);
        if (auto constant = dyn_cast<ConstantFP>(element)) {
          auto newElement = getIfPossible(constant);

          if (nullptr == newElement) {
            isValid = false;
            break;
          }

          constants.push_back(newElement);
        }
      }

      if (isValid) {
        newY = ConstantVector::get(constants);
      }
    } else if (auto constant = dyn_cast<ConstantFP>(use)) {
      newY = getIfPossible(constant);
    }

    if (nullptr == newY) {
      continue;
    }

    // we are definitely modifying the module now!
    modified = true;

    // look up the corresponding pown version of the function
    auto newFunc = module.getFunction(newFuncName);

    // if the pown version isn't in the module (it wasn't called
    // explicitly) then we need to add it to the module
    if (nullptr == newFunc) {
      SmallVector<Type *, 2> argTypes;

      // we use the same arg type for the first argument
      argTypes.push_back(func->getFunctionType()->getParamType(0));

      // but a different arg type for the second (int or int vector)
      argTypes.push_back(newY->getType());

      // get the new function type from the args
      auto newFuncType = FunctionType::get(
          func->getFunctionType()->getReturnType(), argTypes, false);

      // and get the new function
      newFunc = Function::Create(newFuncType, func->getLinkage(), newFuncName,
                                 &module);

      // and take the calling convention of the old called function
      newFunc->setCallingConv(func->getCallingConv());
    }

    SmallVector<OperandBundleDef, 0> bundles;
    ci->getOperandBundlesAsDefs(bundles);

    SmallVector<Value *, 2> args;

    // we use the same first argument
    args.push_back(*(ci->arg_begin()));

    // and use our new constant as the second
    args.push_back(newY);

    // create a new call that calls the replacement fast function
    auto newCi = CallInst::Create(newFunc, args, bundles);
    newCi->insertBefore(ci->getIterator());

    // take the name of the old call instruction
    newCi->takeName(ci);

    // and take the calling conventions too
    newCi->setCallingConv(ci->getCallingConv());

    // replace the old call with the new one
    ci->replaceAllUsesWith(newCi);

    // and erase the old call
    ci->eraseFromParent();
  }

  return modified;
}

bool powToPown(Module &module) {
  const std::map<std::string, std::string> map = {
      {"_Z3powff", "_Z4pownfi"},
      {"_Z3powDv2_fS_", "_Z4pownDv2_fDv2_i"},
      {"_Z3powDv3_fS_", "_Z4pownDv3_fDv3_i"},
      {"_Z3powDv4_fS_", "_Z4pownDv4_fDv4_i"},
      {"_Z3powDv8_fS_", "_Z4pownDv8_fDv8_i"},
      {"_Z3powDv16_fS_", "_Z4pownDv16_fDv16_i"},
      {"_Z3powdd", "_Z4powndi"},
      {"_Z3powDv2_dS_", "_Z4pownDv2_dDv2_i"},
      {"_Z3powDv3_dS_", "_Z4pownDv3_dDv3_i"},
      {"_Z3powDv4_dS_", "_Z4pownDv4_dDv4_i"},
      {"_Z3powDv8_dS_", "_Z4pownDv8_dDv8_i"},
      {"_Z3powDv16_dS_", "_Z4pownDv16_dDv16_i"},
      {"_Z4powrff", "_Z4pownfi"},
      {"_Z4powrDv2_fS_", "_Z4pownDv2_fDv2_i"},
      {"_Z4powrDv3_fS_", "_Z4pownDv3_fDv3_i"},
      {"_Z4powrDv4_fS_", "_Z4pownDv4_fDv4_i"},
      {"_Z4powrDv8_fS_", "_Z4pownDv8_fDv8_i"},
      {"_Z4powrDv16_fS_", "_Z4pownDv16_fDv16_i"},
      {"_Z4powrdd", "_Z4powndi"},
      {"_Z4powrDv2_dS_", "_Z4pownDv2_dDv2_i"},
      {"_Z4powrDv3_dS_", "_Z4pownDv3_dDv3_i"},
      {"_Z4powrDv4_dS_", "_Z4pownDv4_dDv4_i"},
      {"_Z4powrDv8_dS_", "_Z4pownDv8_dDv8_i"},
      {"_Z4powrDv16_dS_", "_Z4pownDv16_dDv16_i"},
  };

  return powToX(module, map, [](double x) -> double { return x; });
}

bool powToRootn(Module &module) {
  const std::map<std::string, std::string> map = {
      {"_Z3powff", "_Z5rootnfi"},
      {"_Z3powDv2_fS_", "_Z5rootnDv2_fDv2_i"},
      {"_Z3powDv3_fS_", "_Z5rootnDv3_fDv3_i"},
      {"_Z3powDv4_fS_", "_Z5rootnDv4_fDv4_i"},
      {"_Z3powDv8_fS_", "_Z5rootnDv8_fDv8_i"},
      {"_Z3powDv16_fS_", "_Z5rootnDv16_fDv16_i"},
      {"_Z3powdd", "_Z5rootndi"},
      {"_Z3powDv2_dS_", "_Z5rootnDv2_dDv2_i"},
      {"_Z3powDv3_dS_", "_Z5rootnDv3_dDv3_i"},
      {"_Z3powDv4_dS_", "_Z5rootnDv4_dDv4_i"},
      {"_Z3powDv8_dS_", "_Z5rootnDv8_dDv8_i"},
      {"_Z3powDv16_dS_", "_Z5rootnDv16_dDv16_i"},
      {"_Z4powrff", "_Z5rootnfi"},
      {"_Z4powrDv2_fS_", "_Z5rootnDv2_fDv2_i"},
      {"_Z4powrDv3_fS_", "_Z5rootnDv3_fDv3_i"},
      {"_Z4powrDv4_fS_", "_Z5rootnDv4_fDv4_i"},
      {"_Z4powrDv8_fS_", "_Z5rootnDv8_fDv8_i"},
      {"_Z4powrDv16_fS_", "_Z5rootnDv16_fDv16_i"},
      {"_Z4powrdd", "_Z5rootndi"},
      {"_Z4powrDv2_dS_", "_Z5rootnDv2_dDv2_i"},
      {"_Z4powrDv3_dS_", "_Z5rootnDv3_dDv3_i"},
      {"_Z4powrDv4_dS_", "_Z5rootnDv4_dDv4_i"},
      {"_Z4powrDv8_dS_", "_Z5rootnDv8_dDv8_i"},
      {"_Z4powrDv16_dS_", "_Z5rootnDv16_dDv16_i"},
  };

  return powToX(module, map, [](double x) -> double { return 1.0 / x; });
}

bool foldOneArgBuiltin(Module &module,
                       const std::vector<std::string> &funcNames,
                       double (*ref_math_func)(double)) {
  SmallVector<CallInst *, 8> calls;

  for (const auto &funcName : funcNames) {
    if (auto func = module.getFunction(funcName)) {
      // the function is in the module, so we go through its uses now
      for (auto user : func->users()) {
        if (auto ci = dyn_cast<CallInst>(user)) {
          // get the first arg
          auto &use = ci->getArgOperandUse(0);

          // if its a constant (or constant vector)
          if (isa<ConstantDataVector>(use) || isa<ConstantVector>(use) ||
              isa<ConstantFP>(use)) {
            calls.push_back(ci);
          }
        }
      }
    }
  }

  // we are modifying the module if there are any calls to replace
  const bool modified = 0 < calls.size();

  for (auto ci : calls) {
    // get the one argument to our function
    auto &use = ci->getArgOperandUse(0);

    // the replacement constant value
    Constant *replacementConstant = nullptr;

    if (auto vec = dyn_cast<ConstantDataVector>(use)) {
      if (multi_llvm::getVectorElementType(vec->getType())->isFloatTy()) {
        SmallVector<float, 16> constants;

        for (unsigned i = 0, e = vec->getNumElements(); i < e; i++) {
          auto element = vec->getElementAsConstant(i);

          if (auto constant = dyn_cast<ConstantFP>(element)) {
            constants.push_back(ref_math_func(getConstantFPAsDouble(constant)));
          }
        }

        replacementConstant =
            ConstantDataVector::get(module.getContext(), constants);
      } else {
        SmallVector<double, 16> constants;

        for (unsigned i = 0, e = vec->getNumElements(); i < e; i++) {
          auto element = vec->getElementAsConstant(i);

          if (auto constant = dyn_cast<ConstantFP>(element)) {
            constants.push_back(ref_math_func(getConstantFPAsDouble(constant)));
          }
        }

        replacementConstant =
            ConstantDataVector::get(module.getContext(), constants);
      }
    } else if (auto constant = dyn_cast<ConstantFP>(use)) {
      // we have a constant, so we can fold it!
      replacementConstant = ConstantFP::get(
          constant->getType(), ref_math_func(getConstantFPAsDouble(constant)));
    } else {
      llvm_unreachable(
          "Unknown constant type (expected ConstantDataVector or ConstantFP!)");
    }

    // replace the old call with the constant
    ci->replaceAllUsesWith(replacementConstant);

    // and erase the old call
    ci->eraseFromParent();
  }

  return modified;
}

bool foldCos(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z3cosf",
                                    "_Z3cosDv2_f",
                                    "_Z3cosDv3_f",
                                    "_Z3cosDv4_f",
                                    "_Z3cosDv8_f",
                                    "_Z3cosDv16_f",
                                    "_Z3cosd",
                                    "_Z3cosDv2_d",
                                    "_Z3cosDv3_d",
                                    "_Z3cosDv4_d",
                                    "_Z3cosDv8_d",
                                    "_Z3cosDv16_d",
                                    "_Z8half_cosf",
                                    "_Z8half_cosDv2_f",
                                    "_Z8half_cosDv3_f",
                                    "_Z8half_cosDv4_f",
                                    "_Z8half_cosDv8_f",
                                    "_Z8half_cosDv16_f",
                                    "_Z10native_cosf",
                                    "_Z10native_cosDv2_f",
                                    "_Z10native_cosDv3_f",
                                    "_Z10native_cosDv4_f",
                                    "_Z10native_cosDv8_f",
                                    "_Z10native_cosDv16_f",
                                },
                                abacus::cos);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5cospif",
                                    "_Z5cospiDv2_f",
                                    "_Z5cospiDv3_f",
                                    "_Z5cospiDv4_f",
                                    "_Z5cospiDv8_f",
                                    "_Z5cospiDv16_f",
                                    "_Z5cospid",
                                    "_Z5cospiDv2_d",
                                    "_Z5cospiDv3_d",
                                    "_Z5cospiDv4_d",
                                    "_Z5cospiDv8_d",
                                    "_Z5cospiDv16_d",
                                },
                                abacus::cospi);

  return modified;
}

bool foldExp(Module &module) {
  const std::vector<std::string> map = {
      "_Z3expf",
      "_Z3expDv2_f",
      "_Z3expDv3_f",
      "_Z3expDv4_f",
      "_Z3expDv8_f",
      "_Z3expDv16_f",
      "_Z3expd",
      "_Z3expDv2_d",
      "_Z3expDv3_d",
      "_Z3expDv4_d",
      "_Z3expDv8_d",
      "_Z3expDv16_d",
      "_Z8half_expf",
      "_Z8half_expDv2_f",
      "_Z8half_expDv3_f",
      "_Z8half_expDv4_f",
      "_Z8half_expDv8_f",
      "_Z8half_expDv16_f",
      "_Z10native_expf",
      "_Z10native_expDv2_f",
      "_Z10native_expDv3_f",
      "_Z10native_expDv4_f",
      "_Z10native_expDv8_f",
      "_Z10native_expDv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::exp);
}

bool foldExp2(Module &module) {
  const std::vector<std::string> map = {
      "_Z4exp2f",
      "_Z4exp2Dv2_f",
      "_Z4exp2Dv3_f",
      "_Z4exp2Dv4_f",
      "_Z4exp2Dv8_f",
      "_Z4exp2Dv16_f",
      "_Z4exp2d",
      "_Z4exp2Dv2_d",
      "_Z4exp2Dv3_d",
      "_Z4exp2Dv4_d",
      "_Z4exp2Dv8_d",
      "_Z4exp2Dv16_d",
      "_Z9half_exp2f",
      "_Z9half_exp2Dv2_f",
      "_Z9half_exp2Dv3_f",
      "_Z9half_exp2Dv4_f",
      "_Z9half_exp2Dv8_f",
      "_Z9half_exp2Dv16_f",
      "_Z11native_exp2f",
      "_Z11native_exp2Dv2_f",
      "_Z11native_exp2Dv3_f",
      "_Z11native_exp2Dv4_f",
      "_Z11native_exp2Dv8_f",
      "_Z11native_exp2Dv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::exp2);
}

bool foldExp10(Module &module) {
  const std::vector<std::string> map = {
      "_Z5exp10f",
      "_Z5exp10Dv2_f",
      "_Z5exp10Dv3_f",
      "_Z5exp10Dv4_f",
      "_Z5exp10Dv8_f",
      "_Z5exp10Dv16_f",
      "_Z5exp10d",
      "_Z5exp10Dv2_d",
      "_Z5exp10Dv3_d",
      "_Z5exp10Dv4_d",
      "_Z5exp10Dv8_d",
      "_Z5exp10Dv16_d",
      "_Z10half_exp10f",
      "_Z10half_exp10Dv2_f",
      "_Z10half_exp10Dv3_f",
      "_Z10half_exp10Dv4_f",
      "_Z10half_exp10Dv8_f",
      "_Z10half_exp10Dv16_f",
      "_Z12native_exp10f",
      "_Z12native_exp10Dv2_f",
      "_Z12native_exp10Dv3_f",
      "_Z12native_exp10Dv4_f",
      "_Z12native_exp10Dv8_f",
      "_Z12native_exp10Dv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::exp10);
}

bool foldExpm1(Module &module) {
  const std::vector<std::string> map = {
      "_Z5expm1f",     "_Z5expm1Dv2_f",  "_Z5expm1Dv3_f", "_Z5expm1Dv4_f",
      "_Z5expm1Dv8_f", "_Z5expm1Dv16_f", "_Z5expm1d",     "_Z5expm1Dv2_d",
      "_Z5expm1Dv3_d", "_Z5expm1Dv4_d",  "_Z5expm1Dv8_d", "_Z5expm1Dv16_d",
  };

  return foldOneArgBuiltin(module, map, abacus::expm1);
}

bool foldLog(Module &module) {
  const std::vector<std::string> map = {
      "_Z3logf",
      "_Z3logDv2_f",
      "_Z3logDv3_f",
      "_Z3logDv4_f",
      "_Z3logDv8_f",
      "_Z3logDv16_f",
      "_Z3logd",
      "_Z3logDv2_d",
      "_Z3logDv3_d",
      "_Z3logDv4_d",
      "_Z3logDv8_d",
      "_Z3logDv16_d",
      "_Z8half_logf",
      "_Z8half_logDv2_f",
      "_Z8half_logDv3_f",
      "_Z8half_logDv4_f",
      "_Z8half_logDv8_f",
      "_Z8half_logDv16_f",
      "_Z10native_logf",
      "_Z10native_logDv2_f",
      "_Z10native_logDv3_f",
      "_Z10native_logDv4_f",
      "_Z10native_logDv8_f",
      "_Z10native_logDv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::log);
}

bool foldLog2(Module &module) {
  const std::vector<std::string> map = {
      "_Z4log2f",
      "_Z4log2Dv2_f",
      "_Z4log2Dv3_f",
      "_Z4log2Dv4_f",
      "_Z4log2Dv8_f",
      "_Z4log2Dv16_f",
      "_Z4log2d",
      "_Z4log2Dv2_d",
      "_Z4log2Dv3_d",
      "_Z4log2Dv4_d",
      "_Z4log2Dv8_d",
      "_Z4log2Dv16_d",
      "_Z9half_log2f",
      "_Z9half_log2Dv2_f",
      "_Z9half_log2Dv3_f",
      "_Z9half_log2Dv4_f",
      "_Z9half_log2Dv8_f",
      "_Z9half_log2Dv16_f",
      "_Z11native_log2f",
      "_Z11native_log2Dv2_f",
      "_Z11native_log2Dv3_f",
      "_Z11native_log2Dv4_f",
      "_Z11native_log2Dv8_f",
      "_Z11native_log2Dv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::log2);
}

bool foldLog10(Module &module) {
  const std::vector<std::string> map = {
      "_Z5log10f",
      "_Z5log10Dv2_f",
      "_Z5log10Dv3_f",
      "_Z5log10Dv4_f",
      "_Z5log10Dv8_f",
      "_Z5log10Dv16_f",
      "_Z5log10d",
      "_Z5log10Dv2_d",
      "_Z5log10Dv3_d",
      "_Z5log10Dv4_d",
      "_Z5log10Dv8_d",
      "_Z5log10Dv16_d",
      "_Z10half_log10f",
      "_Z10half_log10Dv2_f",
      "_Z10half_log10Dv3_f",
      "_Z10half_log10Dv4_f",
      "_Z10half_log10Dv8_f",
      "_Z10half_log10Dv16_f",
      "_Z12native_log10f",
      "_Z12native_log10Dv2_f",
      "_Z12native_log10Dv3_f",
      "_Z12native_log10Dv4_f",
      "_Z12native_log10Dv8_f",
      "_Z12native_log10Dv16_f",
  };

  return foldOneArgBuiltin(module, map, abacus::log10);
}

bool foldLog1p(Module &module) {
  const std::vector<std::string> map = {
      "_Z5log1pf",     "_Z5log1pDv2_f",  "_Z5log1pDv3_f", "_Z5log1pDv4_f",
      "_Z5log1pDv8_f", "_Z5log1pDv16_f", "_Z5log1pd",     "_Z5log1pDv2_d",
      "_Z5log1pDv3_d", "_Z5log1pDv4_d",  "_Z5log1pDv8_d", "_Z5log1pDv16_d",
  };

  return foldOneArgBuiltin(module, map, abacus::log1p);
}

bool foldSin(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z3sinf",
                                    "_Z3sinDv2_f",
                                    "_Z3sinDv3_f",
                                    "_Z3sinDv4_f",
                                    "_Z3sinDv8_f",
                                    "_Z3sinDv16_f",
                                    "_Z3sind",
                                    "_Z3sinDv2_d",
                                    "_Z3sinDv3_d",
                                    "_Z3sinDv4_d",
                                    "_Z3sinDv8_d",
                                    "_Z3sinDv16_d",
                                    "_Z8half_sinf",
                                    "_Z8half_sinDv2_f",
                                    "_Z8half_sinDv3_f",
                                    "_Z8half_sinDv4_f",
                                    "_Z8half_sinDv8_f",
                                    "_Z8half_sinDv16_f",
                                    "_Z10native_sinf",
                                    "_Z10native_sinDv2_f",
                                    "_Z10native_sinDv3_f",
                                    "_Z10native_sinDv4_f",
                                    "_Z10native_sinDv8_f",
                                    "_Z10native_sinDv16_f",
                                },
                                abacus::sin);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5sinpif",
                                    "_Z5sinpiDv2_f",
                                    "_Z5sinpiDv3_f",
                                    "_Z5sinpiDv4_f",
                                    "_Z5sinpiDv8_f",
                                    "_Z5sinpiDv16_f",
                                    "_Z5sinpid",
                                    "_Z5sinpiDv2_d",
                                    "_Z5sinpiDv3_d",
                                    "_Z5sinpiDv4_d",
                                    "_Z5sinpiDv8_d",
                                    "_Z5sinpiDv16_d",
                                },
                                abacus::sinpi);

  return modified;
}

bool foldTan(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z3tanf",
                                    "_Z3tanDv2_f",
                                    "_Z3tanDv3_f",
                                    "_Z3tanDv4_f",
                                    "_Z3tanDv8_f",
                                    "_Z3tanDv16_f",
                                    "_Z3tand",
                                    "_Z3tanDv2_d",
                                    "_Z3tanDv3_d",
                                    "_Z3tanDv4_d",
                                    "_Z3tanDv8_d",
                                    "_Z3tanDv16_d",
                                    "_Z8half_tanf",
                                    "_Z8half_tanDv2_f",
                                    "_Z8half_tanDv3_f",
                                    "_Z8half_tanDv4_f",
                                    "_Z8half_tanDv8_f",
                                    "_Z8half_tanDv16_f",
                                    "_Z10native_tanf",
                                    "_Z10native_tanDv2_f",
                                    "_Z10native_tanDv3_f",
                                    "_Z10native_tanDv4_f",
                                    "_Z10native_tanDv8_f",
                                    "_Z10native_tanDv16_f",
                                },
                                abacus::tan);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5tanpif",
                                    "_Z5tanpiDv2_f",
                                    "_Z5tanpiDv3_f",
                                    "_Z5tanpiDv4_f",
                                    "_Z5tanpiDv8_f",
                                    "_Z5tanpiDv16_f",
                                    "_Z5tanpid",
                                    "_Z5tanpiDv2_d",
                                    "_Z5tanpiDv3_d",
                                    "_Z5tanpiDv4_d",
                                    "_Z5tanpiDv8_d",
                                    "_Z5tanpiDv16_d",
                                },
                                abacus::tanpi);

  return modified;
}

bool foldArcFuncs(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4acosf",
                                    "_Z4acosDv2_f",
                                    "_Z4acosDv3_f",
                                    "_Z4acosDv4_f",
                                    "_Z4acosDv8_f",
                                    "_Z4acosDv16_f",
                                    "_Z4acosd",
                                    "_Z4acosDv2_d",
                                    "_Z4acosDv3_d",
                                    "_Z4acosDv4_d",
                                    "_Z4acosDv8_d",
                                    "_Z4acosDv16_d",
                                },
                                abacus::acos);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4asinf",
                                    "_Z4asinDv2_f",
                                    "_Z4asinDv3_f",
                                    "_Z4asinDv4_f",
                                    "_Z4asinDv8_f",
                                    "_Z4asinDv16_f",
                                    "_Z4asind",
                                    "_Z4asinDv2_d",
                                    "_Z4asinDv3_d",
                                    "_Z4asinDv4_d",
                                    "_Z4asinDv8_d",
                                    "_Z4asinDv16_d",
                                },
                                abacus::asin);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4atanf",
                                    "_Z4atanDv2_f",
                                    "_Z4atanDv3_f",
                                    "_Z4atanDv4_f",
                                    "_Z4atanDv8_f",
                                    "_Z4atanDv16_f",
                                    "_Z4atand",
                                    "_Z4atanDv2_d",
                                    "_Z4atanDv3_d",
                                    "_Z4atanDv4_d",
                                    "_Z4atanDv8_d",
                                    "_Z4atanDv16_d",
                                },
                                abacus::atan);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z6acospif",
                                    "_Z6acospiDv2_f",
                                    "_Z6acospiDv3_f",
                                    "_Z6acospiDv4_f",
                                    "_Z6acospiDv8_f",
                                    "_Z6acospiDv16_f",
                                    "_Z6acospid",
                                    "_Z6acospiDv2_d",
                                    "_Z6acospiDv3_d",
                                    "_Z6acospiDv4_d",
                                    "_Z6acospiDv8_d",
                                    "_Z6acospiDv16_d",
                                },
                                abacus::acospi);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z6asinpif",
                                    "_Z6asinpiDv2_f",
                                    "_Z6asinpiDv3_f",
                                    "_Z6asinpiDv4_f",
                                    "_Z6asinpiDv8_f",
                                    "_Z6asinpiDv16_f",
                                    "_Z6asinpid",
                                    "_Z6asinpiDv2_d",
                                    "_Z6asinpiDv3_d",
                                    "_Z6asinpiDv4_d",
                                    "_Z6asinpiDv8_d",
                                    "_Z6asinpiDv16_d",
                                },
                                abacus::asinpi);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z6atanpif",
                                    "_Z6atanpiDv2_f",
                                    "_Z6atanpiDv3_f",
                                    "_Z6atanpiDv4_f",
                                    "_Z6atanpiDv8_f",
                                    "_Z6atanpiDv16_f",
                                    "_Z6atanpid",
                                    "_Z6atanpiDv2_d",
                                    "_Z6atanpiDv3_d",
                                    "_Z6atanpiDv4_d",
                                    "_Z6atanpiDv8_d",
                                    "_Z6atanpiDv16_d",
                                },
                                abacus::atanpi);

  return modified;
}

bool foldHyperbolicFuncs(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4coshf",
                                    "_Z4coshDv2_f",
                                    "_Z4coshDv3_f",
                                    "_Z4coshDv4_f",
                                    "_Z4coshDv8_f",
                                    "_Z4coshDv16_f",
                                    "_Z4coshd",
                                    "_Z4coshDv2_d",
                                    "_Z4coshDv3_d",
                                    "_Z4coshDv4_d",
                                    "_Z4coshDv8_d",
                                    "_Z4coshDv16_d",
                                },
                                abacus::cosh);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4sinhf",
                                    "_Z4sinhDv2_f",
                                    "_Z4sinhDv3_f",
                                    "_Z4sinhDv4_f",
                                    "_Z4sinhDv8_f",
                                    "_Z4sinhDv16_f",
                                    "_Z4sinhd",
                                    "_Z4sinhDv2_d",
                                    "_Z4sinhDv3_d",
                                    "_Z4sinhDv4_d",
                                    "_Z4sinhDv8_d",
                                    "_Z4sinhDv16_d",
                                },
                                abacus::sinh);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4tanhf",
                                    "_Z4tanhDv2_f",
                                    "_Z4tanhDv3_f",
                                    "_Z4tanhDv4_f",
                                    "_Z4tanhDv8_f",
                                    "_Z4tanhDv16_f",
                                    "_Z4tanhd",
                                    "_Z4tanhDv2_d",
                                    "_Z4tanhDv3_d",
                                    "_Z4tanhDv4_d",
                                    "_Z4tanhDv8_d",
                                    "_Z4tanhDv16_d",
                                },
                                abacus::tanh);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5acoshf",
                                    "_Z5acoshDv2_f",
                                    "_Z5acoshDv3_f",
                                    "_Z5acoshDv4_f",
                                    "_Z5acoshDv8_f",
                                    "_Z5acoshDv16_f",
                                    "_Z5acoshd",
                                    "_Z5acoshDv2_d",
                                    "_Z5acoshDv3_d",
                                    "_Z5acoshDv4_d",
                                    "_Z5acoshDv8_d",
                                    "_Z5acoshDv16_d",
                                },
                                abacus::acosh);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5asinhf",
                                    "_Z5asinhDv2_f",
                                    "_Z5asinhDv3_f",
                                    "_Z5asinhDv4_f",
                                    "_Z5asinhDv8_f",
                                    "_Z5asinhDv16_f",
                                    "_Z5asinhd",
                                    "_Z5asinhDv2_d",
                                    "_Z5asinhDv3_d",
                                    "_Z5asinhDv4_d",
                                    "_Z5asinhDv8_d",
                                    "_Z5asinhDv16_d",
                                },
                                abacus::asinh);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5atanhf",
                                    "_Z5atanhDv2_f",
                                    "_Z5atanhDv3_f",
                                    "_Z5atanhDv4_f",
                                    "_Z5atanhDv8_f",
                                    "_Z5atanhDv16_f",
                                    "_Z5atanhd",
                                    "_Z5atanhDv2_d",
                                    "_Z5atanhDv3_d",
                                    "_Z5atanhDv4_d",
                                    "_Z5atanhDv8_d",
                                    "_Z5atanhDv16_d",
                                },
                                abacus::atanh);

  return modified;
}

bool foldRootFuncs(Module &module) {
  bool modified = false;

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4cbrtf",
                                    "_Z4cbrtDv2_f",
                                    "_Z4cbrtDv3_f",
                                    "_Z4cbrtDv4_f",
                                    "_Z4cbrtDv8_f",
                                    "_Z4cbrtDv16_f",
                                    "_Z4cbrtd",
                                    "_Z4cbrtDv2_d",
                                    "_Z4cbrtDv3_d",
                                    "_Z4cbrtDv4_d",
                                    "_Z4cbrtDv8_d",
                                    "_Z4cbrtDv16_d",
                                },
                                abacus::cbrt);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z4sqrtf",
                                    "_Z4sqrtDv2_f",
                                    "_Z4sqrtDv3_f",
                                    "_Z4sqrtDv4_f",
                                    "_Z4sqrtDv8_f",
                                    "_Z4sqrtDv16_f",
                                    "_Z4sqrtd",
                                    "_Z4sqrtDv2_d",
                                    "_Z4sqrtDv3_d",
                                    "_Z4sqrtDv4_d",
                                    "_Z4sqrtDv8_d",
                                    "_Z4sqrtDv16_d",
                                },
                                abacus::sqrt);

  modified |= foldOneArgBuiltin(module,
                                {
                                    "_Z5rsqrtf",
                                    "_Z5rsqrtDv2_f",
                                    "_Z5rsqrtDv3_f",
                                    "_Z5rsqrtDv4_f",
                                    "_Z5rsqrtDv8_f",
                                    "_Z5rsqrtDv16_f",
                                    "_Z5rsqrtd",
                                    "_Z5rsqrtDv2_d",
                                    "_Z5rsqrtDv3_d",
                                    "_Z5rsqrtDv4_d",
                                    "_Z5rsqrtDv8_d",
                                    "_Z5rsqrtDv16_d",
                                },
                                abacus::rsqrt);

  return modified;
}
}  // namespace

PreservedAnalyses compiler::BuiltinSimplificationPass::run(
    Module &module, ModuleAnalysisManager &) {
  bool Changed = false;

  bool LocalChanged;

  do {
    // Reset local result.
    LocalChanged = false;

    LocalChanged |= powToPown(module);
    LocalChanged |= powToRootn(module);
    LocalChanged |= foldCos(module);
    LocalChanged |= foldExp(module);
    LocalChanged |= foldExp2(module);
    LocalChanged |= foldExp10(module);
    LocalChanged |= foldExpm1(module);
    LocalChanged |= foldLog(module);
    LocalChanged |= foldLog2(module);
    LocalChanged |= foldLog10(module);
    LocalChanged |= foldLog1p(module);
    LocalChanged |= foldSin(module);
    LocalChanged |= foldTan(module);
    LocalChanged |= foldArcFuncs(module);
    LocalChanged |= foldHyperbolicFuncs(module);
    LocalChanged |= foldRootFuncs(module);

    // Update the global result if we modified anything.
    Changed |= LocalChanged;
  } while (LocalChanged);

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
