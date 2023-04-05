// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef MULTI_LLVM_MULTI_LLVM_H_INCLUDED
#define MULTI_LLVM_MULTI_LLVM_H_INCLUDED

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/IVDescriptors.h>
#include <llvm/Analysis/IteratedDominanceFrontier.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/GlobalObject.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/InstructionCost.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/llvm_version.h>

#include <memory>

namespace llvm {
class TargetTransformInfo;
}

namespace multi_llvm {

using std::make_unique;

using IDFCalculator = llvm::IDFCalculator<false>;

using llvm::isa_and_nonnull;

template <typename T>
llvm::ArrayRef<T> ArrayRef(T *data, size_t size) {
#if LLVM_VERSION_MAJOR >= 16
  return llvm::ArrayRef<T>(data, size);
#else
  return llvm::makeArrayRef<T>(data, size);
#endif
}

template <typename T>
llvm::ArrayRef<T> ArrayRef(llvm::SmallVectorImpl<T> &data) {
#if LLVM_VERSION_MAJOR >= 16
  return llvm::ArrayRef<T>(data.data(), data.size());
#else
  return llvm::makeArrayRef<T>(data.data(), data.size());
#endif
}

struct InlineResult {
  llvm::InlineResult result;
  InlineResult(const llvm::InlineResult &r) : result(r) {}
  inline bool isSuccess() { return result.isSuccess(); }
};

// LLVM 11 changes the InlineFunction API so it takes the CallBase argument as
// a reference now. Therefore, we need a generic helper that will also work for
// prior LLVM versions.
inline InlineResult InlineFunction(llvm::CallInst *CI,
                                   llvm::InlineFunctionInfo &IFI,
                                   llvm::AAResults *CalleeAAR = nullptr,
                                   bool InsertLifetime = true) {
#if LLVM_VERSION_MAJOR >= 16
  return llvm::InlineFunction(*CI, IFI, /* MergeAttributes */ false, CalleeAAR,
                              InsertLifetime,
                              /* *ForwardVarArgsTo */ nullptr);
#else
  return llvm::InlineFunction(*CI, IFI, CalleeAAR, InsertLifetime);
#endif
}

inline llvm::StructType *getStructTypeByName(llvm::Module &module,
                                             llvm::StringRef name) {
  return llvm::StructType::getTypeByName(module.getContext(), name);
}

inline llvm::DILocation *getDILocation(unsigned Line, unsigned Column,
                                       llvm::MDNode *Scope,
                                       llvm::MDNode *InlinedAt = nullptr) {
  // If no scope is available, this is an unknown location.
  if (!Scope) return llvm::DebugLoc();
  return llvm::DILocation::get(Scope->getContext(), Line, Column, Scope,
                               InlinedAt, /*ImplicitCode*/ false);
}

inline void insertAtEnd(llvm::BasicBlock *bb, llvm::Instruction *newInst) {
#if LLVM_VERSION_MAJOR >= 16
  newInst->insertInto(bb, bb->end());
#else
  bb->getInstList().push_back(newInst);
#endif
}

template <typename T>
inline typename std::remove_reference_t<T>::ScalarTy getFixedValue(T &&V) {
#if LLVM_VERSION_MAJOR >= 16
  return V.getFixedValue();
#else
  return V.getFixedSize();
#endif
}

template <typename T>
inline typename std::remove_reference_t<T>::ScalarTy getKnownMinValue(T &&M) {
#if LLVM_VERSION_MAJOR >= 16
  return M.getKnownMinValue();
#else
  return M.getKnownMinSize();
#endif
}

using RecurKind = llvm::RecurKind;

/// @brief Create a binary operation corresponding to the given
/// `multi_llvm::RecurKind` with the two provided arguments. It may not
/// necessarily return one of LLVM's in-built `BinaryOperator`s, or even one
/// operation: integer min/max operations may defer to multiple instructions or
/// intrinsics depending on the LLVM version.
///
/// @param[in] B the IRBuilder to build new instructions
/// @param[in] lhs the left-hand value for the operation
/// @param[in] rhs the right-hand value for the operation
/// @param[in] kind the kind of operation to create
/// @param[out] The binary operation.
inline llvm::Value *createBinOpForRecurKind(llvm::IRBuilder<> &B,
                                            llvm::Value *lhs, llvm::Value *rhs,
                                            multi_llvm::RecurKind kind) {
  switch (kind) {
    default:
      break;
    case multi_llvm::RecurKind::None:
      return nullptr;
    case multi_llvm::RecurKind::Add:
      return B.CreateAdd(lhs, rhs);
    case multi_llvm::RecurKind::Mul:
      return B.CreateMul(lhs, rhs);
    case multi_llvm::RecurKind::Or:
      return B.CreateOr(lhs, rhs);
    case multi_llvm::RecurKind::And:
      return B.CreateAnd(lhs, rhs);
    case multi_llvm::RecurKind::Xor:
      return B.CreateXor(lhs, rhs);
    case multi_llvm::RecurKind::FAdd:
      return B.CreateFAdd(lhs, rhs);
    case multi_llvm::RecurKind::FMul:
      return B.CreateFMul(lhs, rhs);
  }
  assert((kind == multi_llvm::RecurKind::FMin ||
          kind == multi_llvm::RecurKind::FMax ||
          kind == multi_llvm::RecurKind::SMin ||
          kind == multi_llvm::RecurKind::SMax ||
          kind == multi_llvm::RecurKind::UMin ||
          kind == multi_llvm::RecurKind::UMax) &&
         "Unexpected min/max kind");
  if (kind == multi_llvm::RecurKind::FMin ||
      kind == multi_llvm::RecurKind::FMax) {
    return B.CreateBinaryIntrinsic(kind == multi_llvm::RecurKind::FMin
                                       ? llvm::Intrinsic::minnum
                                       : llvm::Intrinsic::maxnum,
                                   lhs, rhs);
  }
  bool isMin = kind == multi_llvm::RecurKind::SMin ||
               kind == multi_llvm::RecurKind::UMin;
  bool isSigned = kind == multi_llvm::RecurKind::SMin ||
                  kind == multi_llvm::RecurKind::SMax;
  llvm::Intrinsic::ID intrOpc =
      isMin ? (isSigned ? llvm::Intrinsic::smin : llvm::Intrinsic::umin)
            : (isSigned ? llvm::Intrinsic::smax : llvm::Intrinsic::umax);
  return B.CreateBinaryIntrinsic(intrOpc, lhs, rhs);
}

inline llvm::Value *createSimpleTargetReduction(
    llvm::IRBuilder<> &B, const llvm::TargetTransformInfo *TTI,
    llvm::Value *Src, RecurKind RdxKind) {
  return llvm::createSimpleTargetReduction(B, TTI, Src, RdxKind);
}

// LLVM 12 replaces the return types of high-level cost functions such as in
// the TargetTransformInfo interfaces with InstructionCost instead of integer.
using InstructionCost = llvm::InstructionCost;
using InstructionCostValueType = InstructionCost::CostType;
inline InstructionCostValueType getInstructionCostValue(
    const InstructionCost &Cost) {
  assert((Cost.isValid() && Cost.getValue()) && "Invalid cost");
  return *(Cost.getValue());
}

using CloneFunctionChangeType = llvm::CloneFunctionChangeType;

inline void CloneFunctionInto(
    llvm::Function *NewFunc, const llvm::Function *OldFunc,
    llvm::ValueToValueMapTy &VMap, CloneFunctionChangeType Changes,
    llvm::SmallVectorImpl<llvm::ReturnInst *> &Returns,
    const char *NameSuffix = "", llvm::ClonedCodeInfo *CodeInfo = nullptr,
    llvm::ValueMapTypeRemapper *TypeMapper = nullptr,
    llvm::ValueMaterializer *Materializer = nullptr) {
  llvm::CloneFunctionInto(NewFunc, OldFunc, VMap, Changes, Returns, NameSuffix,
                          CodeInfo, TypeMapper, Materializer);
  // FIXME This works around a bug introduced in llvm@22a52dfdd
  // Remove this once https://reviews.llvm.org/D99334 is merged and generally
  // available
  if (auto *M = NewFunc->getParent()) {
    if (auto *NMD = M->getNamedMetadata("llvm.dbg.cu")) {
      if (!NMD->getNumOperands() &&
          !OldFunc->getParent()->getNamedMetadata("llvm.dbg.cu")) {
        NMD->eraseFromParent();
      }
    }
  }
}

inline llvm::AtomicCmpXchgInst *CreateAtomicCmpXchg(
    llvm::IRBuilder<> &IRBuilder, llvm::Value *Ptr, llvm::Value *Cmp,
    llvm::Value *New, llvm::AtomicOrdering SuccessOrdering,
    llvm::AtomicOrdering FailureOrdering,
    llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
  return IRBuilder.CreateAtomicCmpXchg(Ptr, Cmp, New, llvm::MaybeAlign(),
                                       SuccessOrdering, FailureOrdering, SSID);
}

inline llvm::AtomicRMWInst *CreateAtomicRMW(
    llvm::IRBuilder<> &IRBuilder, llvm::AtomicRMWInst::BinOp Op,
    llvm::Value *Ptr, llvm::Value *Val, llvm::AtomicOrdering Ordering,
    llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
  return IRBuilder.CreateAtomicRMW(Op, Ptr, Val, llvm::MaybeAlign(), Ordering,
                                   SSID);
}

/** `llvm::RegisterPass<>` doesn't support non-default-constructible passes, so
 * we have to use a little SFINAE hackery here to support both types of pass
 * with a single incantation. The non-default-constructible passes don't get
 * allocated by the llvm pass registry, but they get made available to other
 * pass registry machinery such as `-print-after-all`
 */
template <typename P>
struct RegisterPass : public llvm::PassInfo {
  template <typename T = P,
            typename std::enable_if<!std::is_default_constructible<T>::value,
                                    bool>::type = true>
  RegisterPass(llvm::StringRef PassArg, llvm::StringRef Name,
               bool CFGOnly = false, bool is_analysis = false, T * = nullptr)
      : llvm::PassInfo(Name, PassArg, &P::ID, nullptr, CFGOnly, is_analysis) {
    llvm::PassRegistry::getPassRegistry()->registerPass(*this);
  }
  template <typename T = P,
            typename std::enable_if<std::is_default_constructible<T>::value,
                                    bool>::type = true>
  RegisterPass(llvm::StringRef PassArg, llvm::StringRef Name,
               bool CFGOnly = false, bool is_analysis = false, T * = nullptr)
      : llvm::PassInfo(Name, PassArg, &P::ID,
                       llvm::PassInfo::NormalCtor_t(llvm::callDefaultCtor<P>),
                       CFGOnly, is_analysis) {
    llvm::PassRegistry::getPassRegistry()->registerPass(*this);
  }
};

inline void replaceUsesWithIf(
    llvm::Value *old, llvm::Value *New,
    llvm::function_ref<bool(llvm::Use &U)> ShouldReplace) {
  old->replaceUsesWithIf(New, ShouldReplace);
}

/// @brief Returns a poison with the given type if poison is available, else
/// returns an undef value with that type. Useful when poison is the canonical
/// or recommended value in newer LLVM versions, but undef suits as a fallback.
///
/// @param[in] ty the type of poison/undef
/// @param[out] The poison/undef value
inline llvm::Value *getPoisonOrUndef(llvm::Type *ty) {
  return llvm::PoisonValue::get(ty);
}

}  // namespace multi_llvm

#endif  // MULTI_LLVM_MULTI_LLVM_H_INCLUDED
