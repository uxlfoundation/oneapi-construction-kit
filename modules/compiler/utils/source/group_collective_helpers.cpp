// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/group_collective_helpers.h>
#include <compiler/utils/mangling.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "multi_llvm/multi_llvm.h"

using namespace llvm;
static llvm::Constant *getNeutralIdentityHelper(multi_llvm::RecurKind Kind,
                                                Type *Ty, bool UseNaN,
                                                bool UseFZero) {
  switch (Kind) {
    default:
      return nullptr;
    case multi_llvm::RecurKind::And:
      return ConstantInt::getAllOnesValue(Ty);
    case multi_llvm::RecurKind::Or:
    case multi_llvm::RecurKind::Add:
    case multi_llvm::RecurKind::Xor:
      return ConstantInt::getNullValue(Ty);
    case multi_llvm::RecurKind::SMin:
      return ConstantInt::get(
          Ty, APInt::getSignedMaxValue(Ty->getScalarSizeInBits()));
    case multi_llvm::RecurKind::SMax:
      return ConstantInt::get(
          Ty, APInt::getSignedMinValue(Ty->getScalarSizeInBits()));
    case multi_llvm::RecurKind::UMin:
      return ConstantInt::get(Ty,
                              APInt::getMaxValue(Ty->getScalarSizeInBits()));
    case multi_llvm::RecurKind::UMax:
      return ConstantInt::get(Ty,
                              APInt::getMinValue(Ty->getScalarSizeInBits()));
    case multi_llvm::RecurKind::FAdd:
      // -0.0 + 0.0 = 0.0 meaning -0.0 (not 0.0) is the neutral value for floats
      // under addition.
      return UseFZero ? ConstantFP::get(Ty, 0.0) : ConstantFP::get(Ty, -0.0);
    case multi_llvm::RecurKind::FMin:
      return UseNaN ? ConstantFP::getQNaN(Ty, /*Negative*/ false)
                    : ConstantFP::getInfinity(Ty, /*Negative*/ false);
    case multi_llvm::RecurKind::FMax:
      return UseNaN ? ConstantFP::getQNaN(Ty, /*Negative*/ true)
                    : ConstantFP::getInfinity(Ty, /*Negative*/ true);
    case multi_llvm::RecurKind::Mul:
      return ConstantInt::get(Ty, 1);
    case multi_llvm::RecurKind::FMul:
      return ConstantFP::get(Ty, 1.0);
  }
}

llvm::Constant *compiler::utils::getNeutralVal(multi_llvm::RecurKind Kind,
                                               Type *Ty) {
  return getNeutralIdentityHelper(Kind, Ty, /*UseNaN*/ true,
                                  /*UseFZero*/ false);
}

llvm::Constant *compiler::utils::getIdentityVal(multi_llvm::RecurKind Kind,
                                                Type *Ty) {
  return getNeutralIdentityHelper(Kind, Ty, /*UseNaN*/ false, /*UseFZero*/
                                  true);
}

llvm::Optional<compiler::utils::GroupCollective>
compiler::utils::isGroupCollective(llvm::Function *f) {
  Lexer L(f->getName());

  // Consume the _Z[0-9]+
  L.Consume("_Z");
  unsigned int _;
  if (!L.ConsumeInteger(_)) {
    return llvm::None;
  }
  GroupCollective collective{};

  // Parse the scope.
  if (L.Consume("work_group_")) {
    collective.scope = GroupCollective::Scope::WorkGroup;
  } else if (L.Consume("sub_group_")) {
    collective.scope = GroupCollective::Scope::SubGroup;
  } else {
    return llvm::None;
  }

  // Then the operation type.
  if (L.Consume("reduce_")) {
    collective.op = GroupCollective::Op::Reduction;
  } else if (L.Consume("all")) {
    collective.op = GroupCollective::Op::All;
  } else if (L.Consume("any")) {
    collective.op = GroupCollective::Op::Any;
  } else if (L.Consume("scan_exclusive_")) {
    collective.op = GroupCollective::Op::ScanExclusive;
  } else if (L.Consume("scan_inclusive_")) {
    collective.op = GroupCollective::Op::ScanInclusive;
  } else if (L.Consume("broadcast")) {
    collective.op = GroupCollective::Op::Broadcast;
  } else {
    return llvm::None;
  }

  // Then the recurrence kind.
  if (collective.op == GroupCollective::Op::All) {
    collective.recurKind = multi_llvm::RecurKind::And;
  } else if (collective.op == GroupCollective::Op::Any) {
    collective.recurKind = multi_llvm::RecurKind::Or;
  } else if (collective.op == GroupCollective::Op::Reduction ||
             collective.op == GroupCollective::Op::ScanExclusive ||
             collective.op == GroupCollective::Op::ScanInclusive) {
    StringRef OpKind;
    if (L.Consume("logical_")) {
      collective.isLogical = true;
    }
    if (L.ConsumeAlpha(OpKind)) {
      auto isSignedInt = [](char c) {
        return c == 'c' || c == 's' || c == 'i' || c == 'l';
      };
      auto isUnsignedSignedInt = [](char c) {
        return c == 'h' || c == 't' || c == 'j' || c == 'm';
      };
      auto isInt = [&](char c) {
        return isUnsignedSignedInt(c) || isSignedInt(c);
      };
      const auto manglingChar = OpKind.back();
      collective.recurKind =
          StringSwitch<multi_llvm::RecurKind>(OpKind)
              .StartsWith("add", isInt(manglingChar)
                                     ? multi_llvm::RecurKind::Add
                                     : multi_llvm::RecurKind::FAdd)
              .StartsWith("min", isInt(manglingChar)
                                     ? (isUnsignedSignedInt(manglingChar)
                                            ? multi_llvm::RecurKind::UMin
                                            : multi_llvm::RecurKind::SMin)
                                     : multi_llvm::RecurKind::FMin)
              .StartsWith("max", isInt(manglingChar)
                                     ? (isUnsignedSignedInt(manglingChar)
                                            ? multi_llvm::RecurKind::UMax
                                            : multi_llvm::RecurKind::SMax)
                                     : multi_llvm::RecurKind::FMax)
              .StartsWith("mul", isInt(manglingChar)
                                     ? multi_llvm::RecurKind::Mul
                                     : multi_llvm::RecurKind::FMul)
              .StartsWith("and", multi_llvm::RecurKind::And)
              .StartsWith("or", multi_llvm::RecurKind::Or)
              .StartsWith("xor", multi_llvm::RecurKind::Xor)
              .Default(multi_llvm::RecurKind::None);
    } else {
      collective.recurKind = multi_llvm::RecurKind::None;
    }
  }
  collective.func = f;
  collective.type = f->getArg(0)->getType();
  return collective;
}
