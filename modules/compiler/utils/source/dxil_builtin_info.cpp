// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/dxil_builtin_info.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/vector_type_helper.h>

#include <cmath>
#include <set>

namespace {

/// @brief Identifiers for recognized DXIL builtins.
enum DXILBuiltinID : compiler::utils::BuiltinID {
  eDXILCreateHandle = compiler::utils::eFirstTargetBuiltin,
  eDXILThreadId,
  eDXILGroupId,
  eDXILThreadIdInGroup,
  eDXILFlattenedThreadIdInGroup,
  eDXILBufferLoad,
  eDXILBufferStore,
  eDXILUnary,
  eDXILIsSpecialFloat,
  eDXILBarrier,
  eDXILBinary,
  eDXILDot,
  eDXILCBufferLoad,
  eDXILCBufferLoadLegacy,
  eDXILRawBufferLoad,
  eDXILRawBufferStore,
  eDXILBufferUpdateCounter,
  eDXILAtomicBinOp,
  eDXILAtomicCompareExchange,
  eDXILBitcastI16toF16,
  eDXILBitcastF16toI16,
  eDXILBitcastI32toF32,
  eDXILBitcastF32toI32,
  eDXILBitcastI64toF64,
  eDXILBitcastF64toI64,
  eDXILLegacyF32toF16,
  eDXILLegacyF16toF32,
  eDXILTertiary,
  eDXILQuaternary,
  eDXILCheckAccessFullyMapped,
  eDXILGetDimensions,
  eDXILSplitDouble,
  eDXILMakeDouble
};

enum DXILTags : uint32_t {
  eDXILTagFAbs = 6,
  eDXILTagSaturate = 7,
  eDXILTagIsNaN = 8,
  eDXILTagIsInf = 9,
  eDXILTagIsFinite = 10,
  eDXILTagIsNormal = 11,
  eDXILTagCos = 12,
  eDXILTagSin = 13,
  eDXILTagTan = 14,
  eDXILTagExp = 21,
  eDXILTagLog = 23,
  eDXILTagSqrt = 24,
  eDXILTagRsqrt = 25,
  eDXILTagRoundNe = 26,
  eDXILTagRoundNi = 27,
  eDXILTagRoundPi = 28,
  eDXILTagRoundZ = 29,
  eDXILTagBfrev = 30,
  eDXILTagCountbits = 31,
  eDXILTagFirstbitLo = 32,
  eDXILTagFirstbitHi = 33,
  eDXILTagFirstbitSHi = 34,
  eDXILTagFMax = 35,
  eDXILTagFMin = 36,
  eDXILTagIMax = 37,
  eDXILTagIMin = 38,
  eDXILTagUMax = 39,
  eDXILTagUMin = 40,
  eDXILTagFMad = 46,
  eDXILTagIMad = 48,
  eDXILTagMsad = 50,
  eDXILTagBfi = 53,
  eDXILTagDot2 = 54,
  eDXILTagDot3 = 55,
  eDXILTagDot4 = 56,
  eDXILTagCBufferLoad = 58,
  eDXILTagCBufferLoadLegacy = 59,
  eDXILTagBufferUpdateCounter = 70,
  eDXILTagCheckAccessFullyMapped = 71,
  eDXILTagGetDimensions = 72,
  eDXILTagAtomicBinOp = 78,
  eDXILTagAtomicCompareExchange = 79,
  eDXILTagThreadId = 93,
  eDXILTagGroupId = 94,
  eDXILTagThreadIdInGroup = 95,
  eDXILTagFlattenedThreadIdInGroup = 96,
  eDXILTagMakeDouble = 101,
  eDXILTagSplitDouble = 102,
  eDXILTagBitcastI16toF16 = 124,
  eDXILTagBitcastF16toI16 = 125,
  eDXILTagBitcastI32toF32 = 126,
  eDXILTagBitcastF32toI32 = 127,
  eDXILTagBitcastI64toF64 = 128,
  eDXILTagBitcastF64toI64 = 129,
  eDXILTagLegacyF32toF16 = 130,
  eDXILTagLegacyF16toF32 = 131,
  eDXILTagRawBufferLoad = 139,
  eDXILTagRawBufferStore = 140
};
}  // namespace

namespace compiler {
namespace utils {
BuiltinID DXILBuiltinInfo::identifyBuiltin(llvm::Function const &F) const {
  auto const name = F.getName();
  return llvm::StringSwitch<BuiltinID>(name)
      .Case("dx.op.createHandle", eDXILCreateHandle)
      .Case("dx.op.threadId.i32", eDXILThreadId)
      .Case("dx.op.threadIdInGroup.i32", eDXILThreadIdInGroup)
      .Case("dx.op.groupId.i32", eDXILGroupId)
      .Case("dx.op.flattenedThreadIdInGroup.i32", eDXILFlattenedThreadIdInGroup)
      .Case("dx.op.barrier", eDXILBarrier)
      .StartsWith("dx.op.bufferLoad", eDXILBufferLoad)
      .StartsWith("dx.op.bufferStore", eDXILBufferStore)
      .StartsWith("dx.op.cbufferLoad", eDXILCBufferLoad)
      .StartsWith("dx.op.cbufferLoadLegacy", eDXILCBufferLoadLegacy)
      .StartsWith("dx.op.rawBufferLoad", eDXILRawBufferLoad)
      .StartsWith("dx.op.rawBufferStore", eDXILRawBufferStore)
      .StartsWith("dx.op.bufferUpdateCounter", eDXILBufferUpdateCounter)
      .StartsWith("dx.op.unary", eDXILUnary)
      .StartsWith("dx.op.binary", eDXILBinary)
      .StartsWith("dx.op.tertiary", eDXILTertiary)
      .StartsWith("dx.op.quaternary", eDXILQuaternary)
      .StartsWith("dx.op.dot", eDXILDot)
      .StartsWith("dx.op.isSpecialFloat", eDXILIsSpecialFloat)
      .StartsWith("dx.op.atomicBinOp", eDXILAtomicBinOp)
      .StartsWith("dx.op.atomicCompareExchange", eDXILAtomicCompareExchange)
      .StartsWith("dx.op.bitcastI16toF16", eDXILBitcastI16toF16)
      .StartsWith("dx.op.bitcastF16toI16", eDXILBitcastF16toI16)
      .StartsWith("dx.op.bitcastI32toF32", eDXILBitcastI32toF32)
      .StartsWith("dx.op.bitcastF32toI32", eDXILBitcastF32toI32)
      .StartsWith("dx.op.bitcastI64toF64", eDXILBitcastI64toF64)
      .StartsWith("dx.op.bitcastF64toI64", eDXILBitcastF64toI64)
      .StartsWith("dx.op.legacyF32ToF16", eDXILLegacyF32toF16)
      .StartsWith("dx.op.legacyF16ToF32", eDXILLegacyF16toF32)
      .StartsWith("dx.op.checkAccessFullyMapped", eDXILCheckAccessFullyMapped)
      .StartsWith("dx.op.getDimensions", eDXILGetDimensions)
      .StartsWith("dx.op.splitDouble", eDXILSplitDouble)
      .StartsWith("dx.op.makeDouble", eDXILMakeDouble)
      .Default(eBuiltinInvalid);
}

BuiltinUniformity DXILBuiltinInfo::isBuiltinUniform(Builtin const &B,
                                                    const llvm::CallInst *CI,
                                                    unsigned SimdDimIdx) const {
  switch (B.ID) {
    default:
      return eBuiltinUniformityUnknown;
    case eDXILBarrier:
    case eDXILCreateHandle:
    case eDXILGroupId:
      return eBuiltinUniformityAlways;
    case eDXILThreadId:
    case eDXILThreadIdInGroup:
      // If we've got a call instruction and its got the correct number of
      // arguments
      if (CI && CI->arg_size() == 2) {
        auto Rank = llvm::dyn_cast<llvm::ConstantInt>(CI->getArgOperand(1));

        if (Rank) {
          // Only vectorize on the selected dimension
          if (Rank->getZExtValue() == SimdDimIdx) {
            return eBuiltinUniformityInstanceID;
          } else {
            return eBuiltinUniformityAlways;
          }
        }
      }

      return eBuiltinUniformityNever;
    case eDXILFlattenedThreadIdInGroup:
      return eBuiltinUniformityInstanceID;
    case eDXILBufferLoad:
    case eDXILBufferStore:
    case eDXILCBufferLoad:
    case eDXILCBufferLoadLegacy:
    case eDXILRawBufferLoad:
    case eDXILRawBufferStore:
    case eDXILUnary:
    case eDXILBinary:
    case eDXILTertiary:
    case eDXILQuaternary:
    case eDXILDot:
    case eDXILBitcastI16toF16:
    case eDXILBitcastF16toI16:
    case eDXILBitcastI32toF32:
    case eDXILBitcastF32toI32:
    case eDXILBitcastI64toF64:
    case eDXILBitcastF64toI64:
    case eDXILLegacyF32toF16:
    case eDXILLegacyF16toF32:
    case eDXILCheckAccessFullyMapped:
    case eDXILGetDimensions:
    case eDXILSplitDouble:
    case eDXILMakeDouble:
      return eBuiltinUniformityLikeInputs;
    case eDXILBufferUpdateCounter:
    case eDXILAtomicBinOp:
    case eDXILAtomicCompareExchange:
      return eBuiltinUniformityNever;
  }
}

Builtin DXILBuiltinInfo::analyzeBuiltin(llvm::Function const &Callee) const {
  auto Properties =
      llvm::StringSwitch<BuiltinProperties>(Callee.getName())
          .Case("dx.op.createHandle", eBuiltinPropertySideEffects)
          .Case("dx.op.threadId.i32",
                static_cast<BuiltinProperties>(
                    eBuiltinPropertyWorkItem |
                    eBuiltinPropertyInlinePostVectorization))
          .Case("dx.op.threadIdInGroup.i32",
                static_cast<BuiltinProperties>(eBuiltinPropertyWorkItem |
                                               eBuiltinPropertyLocalID))
          .Case("dx.op.groupId.i32", eBuiltinPropertyWorkItem)
          .Case("dx.op.flattenedThreadIdInGroup.i32", eBuiltinPropertyWorkItem)
          .Case("dx.op.barrier", eBuiltinPropertyExecutionFlow)
          .Case("dx.op.atomicBinOp.i32", eBuiltinPropertySupportsInstantiation)
          .Case("dx.op.atomicCompareExchange.i32",
                eBuiltinPropertySupportsInstantiation)
          .StartsWith("dx.op.bufferLoad",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.bufferStore",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.cbufferLoad",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.cbufferLoadLegacy",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.rawBufferLoad",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.rawBufferStore",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.bufferUpdateCounter",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent |
                          eBuiltinPropertySideEffects))
          .StartsWith("dx.op.unary", static_cast<BuiltinProperties>(
                                         eBuiltinPropertySupportsInstantiation |
                                         eBuiltinPropertyCanEmitInline |
                                         eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.binary",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.tertiary",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline))
          .StartsWith("dx.op.quaternary", eBuiltinPropertySupportsInstantiation)
          .StartsWith("dx.op.dot", static_cast<BuiltinProperties>(
                                       eBuiltinPropertySupportsInstantiation |
                                       eBuiltinPropertyCanEmitInline))
          .StartsWith("dx.op.isSpecialFloat",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline |
                          eBuiltinPropertyNoVectorEquivalent))
          .StartsWith("dx.op.bitcast",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline))
          .StartsWith("dx.op.legacyF16ToF32",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline))
          .StartsWith("dx.op.legacyF32ToF16",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline))
          .StartsWith("dx.op.checkAccessFullyMapped",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.getDimensions",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.splitDouble",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline |
                          eBuiltinPropertyVectorEquivalent))
          .StartsWith("dx.op.makeDouble",
                      static_cast<BuiltinProperties>(
                          eBuiltinPropertySupportsInstantiation |
                          eBuiltinPropertyCanEmitInline |
                          eBuiltinPropertyVectorEquivalent))
          .Default(eBuiltinPropertyNone);
  auto ID = identifyBuiltin(Callee);
  return Builtin{Callee, ID, Properties};
}

llvm::Function *DXILBuiltinInfo::getVectorEquivalent(Builtin const &B,
                                                     unsigned Width,
                                                     llvm::Module *) {
  if (B.function.getName().equals("dx.op.threadId.i32")) {
    // We need to store the Width as a metadata item so we can offset our
    // original thread id by it later!
    auto &Context = B.function.getContext();
    auto WidthConstant =
        llvm::ConstantInt::get(llvm::IntegerType::get(Context, 32), Width);
    auto WidthMetadata = llvm::ConstantAsMetadata::get(WidthConstant);
    auto WidthMDNode = llvm::MDNode::get(Context, WidthMetadata);

    // TODON'T we shouldn't be doing this
    auto &func = const_cast<llvm::Function &>(B.function);
    func.addMetadata("Width", *WidthMDNode);
  }

  return nullptr;
}

llvm::Function *DXILBuiltinInfo::getScalarEquivalent(Builtin const &,
                                                     llvm::Module *) {
  return nullptr;
}

llvm::Value *DXILBuiltinInfo::emitBuiltinInline(
    llvm::Function *Builtin, llvm::IRBuilder<> &B,
    llvm::ArrayRef<llvm::Value *> Args) {
  auto Name = Builtin->getName();
  if (Name.equals("dx.op.threadId.i32")) {
    auto MDNode = Builtin->getMetadata("Width");
    if (nullptr == MDNode) {
      return nullptr;
    }

    auto WidthMetadata =
        llvm::dyn_cast<llvm::ConstantAsMetadata>(MDNode->getOperand(0).get());

    if (nullptr == WidthMetadata) {
      return nullptr;
    }

    auto WidthConstant =
        llvm::dyn_cast<llvm::ConstantInt>(WidthMetadata->getValue());

    if (nullptr == WidthConstant) {
      return nullptr;
    }

    auto NewCI = B.CreateCall(Builtin, Args);

    // Clear the metadata we hid in the function
    Builtin->setMetadata("Width", nullptr);

    return NewCI;
  } else if (Name.startswith("dx.op.unary") ||
             Name.startswith("dx.op.binary") ||
             Name.startswith("dx.op.tertiary") ||
             Name.startswith("dx.op.quaternary") ||
             Name.startswith("dx.op.dot") ||
             Name.startswith("dx.op.isSpecialFloat") ||
             Name.startswith("dx.op.bitcast") ||
             Name.startswith("dx.op.legacyF16ToF32") ||
             Name.startswith("dx.op.legacyF32ToF16") ||
             Name.startswith("dx.op.splitDouble") ||
             Name.startswith("dx.op.makeDouble")) {
    auto TagConstant = llvm::cast<llvm::ConstantInt>(Args[0]);
    auto Tag = TagConstant->getZExtValue();

    llvm::Module *M = B.GetInsertBlock()->getParent()->getParent();

    if (eDXILTagFAbs == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::fabs, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagSaturate == Tag) {
      llvm::Function *Min = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::minnum, Args[1]->getType());
      llvm::Function *Max = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::maxnum, Args[1]->getType());

      llvm::Type *Ty = Args[1]->getType();

      llvm::Value *Part =
          B.CreateCall(Min, {Args[1], llvm::ConstantFP::get(Ty, 1.0)});

      return B.CreateCall(Max, {Part, llvm::ConstantFP::get(Ty, 0.0)});
    } else if (eDXILTagIsNaN == Tag) {
      return B.CreateFCmpUNO(Args[1], Args[1]);
    } else if (eDXILTagIsInf == Tag) {
      llvm::Type *Ty = Args[1]->getType();

      if (!Ty->isFloatTy()) {
        return nullptr;
      }

      const uint32_t Mask = 0x7fffffffu;
      const uint32_t Inf = 0x7f800000u;
      const unsigned Bitwidth = 32;

      llvm::Type *IntTy = B.getIntNTy(Bitwidth);

      auto Cast = B.CreateBitCast(Args[1], IntTy);
      auto And = B.CreateAnd(Cast, B.getIntN(Bitwidth, Mask));
      return B.CreateICmpEQ(And, B.getIntN(Bitwidth, Inf));
    } else if (eDXILTagIsFinite == Tag) {
      llvm::Type *Ty = Args[1]->getType();

      if (!Ty->isFloatTy()) {
        return nullptr;
      }

      const uint32_t Mask = 0x7f800000u;
      const unsigned Bitwidth = 32;

      auto Cast = B.CreateBitCast(Args[1], B.getIntNTy(Bitwidth));
      auto And = B.CreateAnd(Cast, B.getIntN(Bitwidth, Mask));
      return B.CreateICmpNE(And, B.getIntN(Bitwidth, Mask));
    } else if (eDXILTagIsNormal == Tag) {
      llvm::Type *Ty = Args[1]->getType();

      if (!Ty->isFloatTy()) {
        return nullptr;
      }

      const uint32_t Mask = 0x7fffffffu;
      const uint32_t Inf = 0x7f800000u;
      const uint32_t Denorm = 0x007fffffu;
      const unsigned Bitwidth = 32;

      auto Cast = B.CreateBitCast(Args[1], B.getIntNTy(Bitwidth));
      auto And = B.CreateAnd(Cast, B.getIntN(Bitwidth, Mask));
      auto Lt = B.CreateICmpULT(And, B.getIntN(Bitwidth, Inf));
      auto Gt = B.CreateICmpUGT(And, B.getIntN(Bitwidth, Denorm));
      return B.CreateAnd(Lt, Gt);
    } else if (eDXILTagCos == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::cos, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagSin == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::sin, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagTan == Tag) {
      llvm::Function *Sin = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::sin, Args[1]->getType());
      llvm::Function *Cos = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::cos, Args[1]->getType());

      auto SinCall = B.CreateCall(Sin, Args[1]);
      auto CosCall = B.CreateCall(Cos, Args[1]);
      return B.CreateFDiv(SinCall, CosCall);
    } else if (eDXILTagExp == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::exp2, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagLog == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::log2, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagSqrt == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::sqrt, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagRsqrt == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::sqrt, Args[1]->getType());

      auto One = llvm::ConstantFP::get(Args[1]->getType(), 1.0);
      auto Call = B.CreateCall(Intrinsic, Args[1]);

      return B.CreateFDiv(One, Call);
    } else if (eDXILTagRoundNe == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::round, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagRoundNi == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::floor, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagRoundPi == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::ceil, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagRoundZ == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::trunc, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagBfrev == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::bitreverse, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagCountbits == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::ctpop, Args[1]->getType());

      return B.CreateCall(Intrinsic, Args[1]);
    } else if (eDXILTagFirstbitLo == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::cttz, Args[1]->getType());

      return B.CreateCall(Intrinsic, {Args[1], B.getInt1(true)});
    } else if (eDXILTagFirstbitHi == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::ctlz, Args[1]->getType());

      return B.CreateCall(Intrinsic, {Args[1], B.getInt1(true)});
    } else if (eDXILTagFirstbitSHi == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::ctlz, Args[1]->getType());

      auto Cond = B.CreateICmpSLT(Args[1], B.getInt32(0));
      auto Not = B.CreateNot(Args[1]);
      auto Select = B.CreateSelect(Cond, Not, Args[1]);

      return B.CreateCall(Intrinsic, {Select, B.getInt1(true)});
    } else if (eDXILTagFMax == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::maxnum, Args[1]->getType());

      return B.CreateCall(Intrinsic, {Args[1], Args[2]});
    } else if (eDXILTagFMin == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::minnum, Args[1]->getType());

      return B.CreateCall(Intrinsic, {Args[1], Args[2]});
    } else if (eDXILTagIMax == Tag) {
      auto Cond = B.CreateICmpSGT(Args[1], Args[2]);

      return B.CreateSelect(Cond, Args[1], Args[2]);
    } else if (eDXILTagIMin == Tag) {
      auto Cond = B.CreateICmpSLT(Args[1], Args[2]);

      return B.CreateSelect(Cond, Args[1], Args[2]);
    } else if (eDXILTagUMax == Tag) {
      auto Cond = B.CreateICmpUGT(Args[1], Args[2]);

      return B.CreateSelect(Cond, Args[1], Args[2]);
    } else if (eDXILTagUMin == Tag) {
      auto Cond = B.CreateICmpULT(Args[1], Args[2]);

      return B.CreateSelect(Cond, Args[1], Args[2]);
    } else if (eDXILTagDot2 == Tag) {
      auto Mul1 = B.CreateFMul(Args[1], Args[3]);
      auto Mul2 = B.CreateFMul(Args[2], Args[4]);

      return B.CreateFAdd(Mul1, Mul2);
    } else if (eDXILTagDot3 == Tag) {
      auto Mul1 = B.CreateFMul(Args[1], Args[4]);
      auto Mul2 = B.CreateFMul(Args[2], Args[5]);
      auto Mul3 = B.CreateFMul(Args[3], Args[6]);

      auto Add = B.CreateFAdd(Mul1, Mul2);

      return B.CreateFAdd(Add, Mul3);
    } else if (eDXILTagDot4 == Tag) {
      auto Mul1 = B.CreateFMul(Args[1], Args[5]);
      auto Mul2 = B.CreateFMul(Args[2], Args[6]);
      auto Mul3 = B.CreateFMul(Args[3], Args[7]);
      auto Mul4 = B.CreateFMul(Args[4], Args[8]);

      auto Add1 = B.CreateFAdd(Mul1, Mul2);
      auto Add2 = B.CreateFAdd(Mul3, Mul4);

      return B.CreateFAdd(Add1, Add2);
    } else if (eDXILTagBitcastI16toF16 == Tag) {
      return B.CreateBitCast(Args[1], B.getHalfTy());
    } else if (eDXILTagBitcastF16toI16 == Tag) {
      return B.CreateBitCast(Args[1], B.getInt16Ty());
    } else if (eDXILTagBitcastI32toF32 == Tag) {
      return B.CreateBitCast(Args[1], B.getFloatTy());
    } else if (eDXILTagBitcastF32toI32 == Tag) {
      return B.CreateBitCast(Args[1], B.getInt32Ty());
    } else if (eDXILTagBitcastI64toF64 == Tag) {
      return B.CreateBitCast(Args[1], B.getDoubleTy());
    } else if (eDXILTagBitcastF64toI64 == Tag) {
      return B.CreateBitCast(Args[1], B.getInt64Ty());
    } else if (eDXILTagLegacyF32toF16 == Tag) {
      // Generated by the f32tof16 intrinsic, which stores the f16 in the lower
      // half of a uint.
      auto Trunc = B.CreateFPTrunc(Args[1], B.getHalfTy());
      auto Cast = B.CreateBitCast(Trunc, B.getInt16Ty());

      return B.CreateZExt(Cast, B.getInt32Ty());
    } else if (eDXILTagLegacyF16toF32 == Tag) {
      // Generated by the f16tof32 intrinsic, which loads the f16 from the
      // lower half of a uint.
      auto Trunc = B.CreateTrunc(Args[1], B.getInt16Ty());
      auto Cast = B.CreateBitCast(Trunc, B.getHalfTy());

      return B.CreateFPExt(Cast, B.getFloatTy());
    } else if (eDXILTagFMad == Tag) {
      llvm::Function *Intrinsic = llvm::Intrinsic::getDeclaration(
          M, llvm::Intrinsic::fmuladd, Args[1]->getType());

      return B.CreateCall(Intrinsic, {Args[1], Args[2], Args[3]});
    } else if (eDXILTagIMad == Tag) {
      auto Mul = B.CreateMul(Args[1], Args[2]);

      return B.CreateAdd(Mul, Args[3]);
    } else if (eDXILTagSplitDouble == Tag) {
      auto Bitcast = B.CreateBitCast(Args[1], B.getInt64Ty());
      auto Lo = B.CreateTrunc(Bitcast, B.getInt32Ty());
      auto Shift = B.CreateLShr(Bitcast, 32);
      auto Hi = B.CreateTrunc(Shift, B.getInt32Ty());

      auto RetStructUndef = llvm::UndefValue::get(Builtin->getReturnType());
      auto RetStruct = B.CreateInsertValue(RetStructUndef, Lo, 0);
      RetStruct = B.CreateInsertValue(RetStruct, Hi, 1);

      return RetStruct;
    } else if (eDXILTagMakeDouble == Tag) {
      auto Lo = B.CreateZExt(Args[1], B.getInt64Ty());
      auto ZExt = B.CreateZExt(Args[2], B.getInt64Ty());
      auto Hi = B.CreateShl(ZExt, 32);
      auto Or = B.CreateOr(Lo, Hi);

      return B.CreateBitCast(Or, B.getDoubleTy());
    }
  }

  return nullptr;
}

llvm::Value *DXILBuiltinInfo::emitBuiltinInline(
    BuiltinID BuiltinID, llvm::IRBuilder<> &B,
    llvm::ArrayRef<llvm::Value *> Args) {
  if (eDXILThreadId == BuiltinID || eDXILThreadIdInGroup == BuiltinID ||
      eDXILFlattenedThreadIdInGroup == BuiltinID) {
    llvm::Value *ActualArgs[2];

    if (1 == Args.size()) {
      // VECZ assumes the ID builtins takes one arg, on DXIL it takes 2, so we
      // have to fix it up!

      // The first argument to DXIL's opcodes is the tag
      switch (BuiltinID) {
        case eDXILThreadId:
          ActualArgs[0] = B.getInt32(eDXILTagThreadId);
          break;
        case eDXILThreadIdInGroup:
          ActualArgs[0] = B.getInt32(eDXILTagThreadIdInGroup);
          break;
        case eDXILFlattenedThreadIdInGroup:
          ActualArgs[0] = B.getInt32(eDXILTagFlattenedThreadIdInGroup);
          break;
      }

      ActualArgs[1] = Args[0];
    } else if (2 == Args.size()) {
      ActualArgs[0] = Args[0];
      ActualArgs[1] = Args[1];
    } else {
      // Wrong number of arguments!
      return nullptr;
    }

    llvm::Type *Tys[2] = {B.getInt32Ty(), B.getInt32Ty()};

    llvm::FunctionType *FuncTy =
        llvm::FunctionType::get(B.getInt32Ty(), Tys, false);

    const char *Name;
    switch (BuiltinID) {
      default:
      case eDXILThreadId:
        Name = "dx.op.threadId.i32";
        break;
      case eDXILThreadIdInGroup:
        Name = "dx.op.threadIdInGroup.i32";
        break;
      case eDXILFlattenedThreadIdInGroup:
        Name = "dx.op.flattenedThreadIdInGroup.i32";
        break;
    }

    llvm::BasicBlock *BB = B.GetInsertBlock();
    llvm::Module *M = BB->getParent()->getParent();

    // Using 'auto' type deduction because LLVM 9 and below will return Value*,
    // otherwise FunctionCallee which effectively wraps {FunctionType+Callee}.
    auto Func = M->getOrInsertFunction(Name, FuncTy);
    return B.CreateCall(Func, ActualArgs);
  }

  return nullptr;
}

BuiltinID DXILBuiltinInfo::getPrintfBuiltin() const {
  // We don't have a printf builtin on DXIL!
  return eBuiltinInvalid;
}

}  // namespace utils
}  // namespace compiler
