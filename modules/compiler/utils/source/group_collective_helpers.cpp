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

#include <compiler/utils/group_collective_helpers.h>
#include <compiler/utils/mangling.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <multi_llvm/optional_helper.h>

#include "multi_llvm/multi_llvm.h"

using namespace llvm;
static llvm::Constant *getNeutralIdentityHelper(RecurKind Kind, Type *Ty,
                                                bool UseNaN, bool UseFZero) {
  switch (Kind) {
    default:
      return nullptr;
    case RecurKind::And:
      return ConstantInt::getAllOnesValue(Ty);
    case RecurKind::Or:
    case RecurKind::Add:
    case RecurKind::Xor:
      return ConstantInt::getNullValue(Ty);
    case RecurKind::SMin:
      return ConstantInt::get(
          Ty, APInt::getSignedMaxValue(Ty->getScalarSizeInBits()));
    case RecurKind::SMax:
      return ConstantInt::get(
          Ty, APInt::getSignedMinValue(Ty->getScalarSizeInBits()));
    case RecurKind::UMin:
      return ConstantInt::get(Ty,
                              APInt::getMaxValue(Ty->getScalarSizeInBits()));
    case RecurKind::UMax:
      return ConstantInt::get(Ty,
                              APInt::getMinValue(Ty->getScalarSizeInBits()));
    case RecurKind::FAdd:
      // -0.0 + 0.0 = 0.0 meaning -0.0 (not 0.0) is the neutral value for floats
      // under addition.
      return UseFZero ? ConstantFP::get(Ty, 0.0) : ConstantFP::get(Ty, -0.0);
    case RecurKind::FMin:
      return UseNaN ? ConstantFP::getQNaN(Ty, /*Negative*/ false)
                    : ConstantFP::getInfinity(Ty, /*Negative*/ false);
    case RecurKind::FMax:
      return UseNaN ? ConstantFP::getQNaN(Ty, /*Negative*/ true)
                    : ConstantFP::getInfinity(Ty, /*Negative*/ true);
    case RecurKind::Mul:
      return ConstantInt::get(Ty, 1);
    case RecurKind::FMul:
      return ConstantFP::get(Ty, 1.0);
  }
}

llvm::Constant *compiler::utils::getNeutralVal(RecurKind Kind, Type *Ty) {
  return getNeutralIdentityHelper(Kind, Ty, /*UseNaN*/ true,
                                  /*UseFZero*/ false);
}

llvm::Constant *compiler::utils::getIdentityVal(RecurKind Kind, Type *Ty) {
  return getNeutralIdentityHelper(Kind, Ty, /*UseNaN*/ false, /*UseFZero*/
                                  true);
}

multi_llvm::Optional<compiler::utils::GroupCollective>
compiler::utils::isGroupCollective(llvm::Function *f) {
  compiler::utils::NameMangler Mangler(&f->getContext());
  SmallVector<Type *, 4> ArgumentTypes;
  SmallVector<compiler::utils::TypeQualifiers, 4> Qualifiers;

  const auto DemangledName = std::string(
      Mangler.demangleName(f->getName(), ArgumentTypes, Qualifiers));

  Lexer L(DemangledName);

  GroupCollective collective{};

  // Parse the scope.
  if (L.Consume("work_group_")) {
    collective.Scope = GroupCollective::ScopeKind::WorkGroup;
  } else if (L.Consume("sub_group_")) {
    collective.Scope = GroupCollective::ScopeKind::SubGroup;
  } else if (L.Consume("vec_group_")) {
    collective.Scope = GroupCollective::ScopeKind::VectorGroup;
  } else {
    return multi_llvm::None;
  }

  // Then the operation type.
  if (L.Consume("reduce_")) {
    collective.Op = GroupCollective::OpKind::Reduction;
  } else if (L.Consume("all")) {
    collective.Op = GroupCollective::OpKind::All;
  } else if (L.Consume("any")) {
    collective.Op = GroupCollective::OpKind::Any;
  } else if (L.Consume("scan_exclusive_")) {
    collective.Op = GroupCollective::OpKind::ScanExclusive;
  } else if (L.Consume("scan_inclusive_")) {
    collective.Op = GroupCollective::OpKind::ScanInclusive;
  } else if (L.Consume("broadcast")) {
    collective.Op = GroupCollective::OpKind::Broadcast;
  } else {
    return multi_llvm::None;
  }

  // Then the recurrence kind.
  if (collective.Op == GroupCollective::OpKind::All) {
    collective.Recurrence = RecurKind::And;
  } else if (collective.Op == GroupCollective::OpKind::Any) {
    collective.Recurrence = RecurKind::Or;
  } else if (collective.Op == GroupCollective::OpKind::Reduction ||
             collective.Op == GroupCollective::OpKind::ScanExclusive ||
             collective.Op == GroupCollective::OpKind::ScanInclusive) {
    if (L.Consume("logical_")) {
      collective.IsLogical = true;
    }

    assert(Qualifiers.size() == 1 && ArgumentTypes.size() == 1 &&
           "Unknown collective builtin");
    auto &Qual = Qualifiers[0];

    bool isSignedInt = false;
    while (!isSignedInt && Qual.getCount()) {
      isSignedInt |= Qual.pop_front() == compiler::utils::eTypeQualSignedInt;
    }

    bool isInt = ArgumentTypes[0]->isIntegerTy();
    bool isFP = ArgumentTypes[0]->isFloatingPointTy();
    // It's not impossible that someone tries to smuggle us a group collective
    // with an unexpected type, so bail out here.
    if (!isInt && !isFP) {
      return multi_llvm::None;
    }

    StringRef OpKind;
    if (!L.ConsumeAlpha(OpKind)) {
      return multi_llvm::None;
    }

    collective.Recurrence =
        StringSwitch<RecurKind>(OpKind)
            .Case("add", isInt ? RecurKind::Add : RecurKind::FAdd)

            .Case("min", isInt
                             ? (isSignedInt ? RecurKind::SMin : RecurKind::UMin)
                             : RecurKind::FMin)
            .Case("max", isInt
                             ? (isSignedInt ? RecurKind::SMax : RecurKind::UMax)
                             : RecurKind::FMax)
            .Case("mul", isInt ? RecurKind::Mul : RecurKind::FMul)
            .Case("and", RecurKind::And)
            .Case("or", RecurKind::Or)
            .Case("xor", RecurKind::Xor)
            .Default(RecurKind::None);

    if (collective.Recurrence == RecurKind::None) {
      return multi_llvm::None;
    }
  }

  // If we've trailing characters left, we're not a recognized collective
  // function.
  if (L.Left() != 0) {
    return multi_llvm::None;
  }

  collective.Func = f;
  collective.Ty = f->getArg(0)->getType();

  return collective;
}
