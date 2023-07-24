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
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <llvm/ADT/StringSwitch.h>
#include <multi_llvm/optional_helper.h>

using namespace llvm;

namespace compiler {
namespace utils {

AnalysisKey BuiltinInfoAnalysis::Key;

BuiltinInfoAnalysis::BuiltinInfoAnalysis()
    : BICallback([](const Module &) -> BuiltinInfo {
        return BuiltinInfo(std::make_unique<CLBuiltinInfo>(nullptr));
      }) {}

Module *BuiltinInfo::getBuiltinsModule() {
  if (LangImpl) {
    return LangImpl->getBuiltinsModule();
  }
  // Mux builtins don't need a module.
  return nullptr;
}

BuiltinID BuiltinInfo::identifyMuxBuiltin(Function const &F) const {
  StringRef const Name = F.getName();
  auto ID =
      StringSwitch<BuiltinID>(Name)
          .Case(MuxBuiltins::isftz, eMuxBuiltinIsFTZ)
          .Case(MuxBuiltins::usefast, eMuxBuiltinUseFast)
          .Case(MuxBuiltins::isembeddedprofile, eMuxBuiltinIsEmbeddedProfile)
          .Case(MuxBuiltins::get_global_size, eMuxBuiltinGetGlobalSize)
          .Case(MuxBuiltins::get_global_id, eMuxBuiltinGetGlobalId)
          .Case(MuxBuiltins::get_global_offset, eMuxBuiltinGetGlobalOffset)
          .Case(MuxBuiltins::get_local_size, eMuxBuiltinGetLocalSize)
          .Case(MuxBuiltins::get_local_id, eMuxBuiltinGetLocalId)
          .Case(MuxBuiltins::set_local_id, eMuxBuiltinSetLocalId)
          .Case(MuxBuiltins::get_sub_group_id, eMuxBuiltinGetSubGroupId)
          .Case(MuxBuiltins::set_sub_group_id, eMuxBuiltinSetSubGroupId)
          .Case(MuxBuiltins::get_num_groups, eMuxBuiltinGetNumGroups)
          .Case(MuxBuiltins::get_num_sub_groups, eMuxBuiltinGetNumSubGroups)
          .Case(MuxBuiltins::set_num_sub_groups, eMuxBuiltinSetNumSubGroups)
          .Case(MuxBuiltins::get_max_sub_group_size,
                eMuxBuiltinGetMaxSubGroupSize)
          .Case(MuxBuiltins::set_max_sub_group_size,
                eMuxBuiltinSetMaxSubGroupSize)
          .Case(MuxBuiltins::get_group_id, eMuxBuiltinGetGroupId)
          .Case(MuxBuiltins::get_work_dim, eMuxBuiltinGetWorkDim)
          .Case(MuxBuiltins::dma_read_1d, eMuxBuiltinDMARead1D)
          .Case(MuxBuiltins::dma_read_2d, eMuxBuiltinDMARead2D)
          .Case(MuxBuiltins::dma_read_3d, eMuxBuiltinDMARead3D)
          .Case(MuxBuiltins::dma_write_1d, eMuxBuiltinDMAWrite1D)
          .Case(MuxBuiltins::dma_write_2d, eMuxBuiltinDMAWrite2D)
          .Case(MuxBuiltins::dma_write_3d, eMuxBuiltinDMAWrite3D)
          .Case(MuxBuiltins::dma_wait, eMuxBuiltinDMAWait)
          .Case(MuxBuiltins::get_global_linear_id, eMuxBuiltinGetGlobalLinearId)
          .Case(MuxBuiltins::get_local_linear_id, eMuxBuiltinGetLocalLinearId)
          .Case(MuxBuiltins::get_enqueued_local_size,
                eMuxBuiltinGetEnqueuedLocalSize)
          .Case(MuxBuiltins::work_group_barrier, eMuxBuiltinWorkGroupBarrier)
          .Case(MuxBuiltins::sub_group_barrier, eMuxBuiltinSubGroupBarrier)
          .Case(MuxBuiltins::mem_barrier, eMuxBuiltinMemBarrier)
          .Default(eBuiltinInvalid);
  return ID;
}

BuiltinUniformity BuiltinInfo::isBuiltinUniform(Builtin const &B,
                                                const CallInst *CI,
                                                unsigned SimdDimIdx) const {
  switch (B.ID) {
    default:
      break;
    case eMuxBuiltinGetGlobalId:
    case eMuxBuiltinGetLocalId: {
      // We need to know the dimension requested from these builtins at compile
      // time to infer their uniformity.
      if (!CI || CI->arg_empty()) {
        return eBuiltinUniformityNever;
      }
      auto *Rank = dyn_cast<ConstantInt>(CI->getArgOperand(0));
      if (!Rank) {
        // The Rank is some function, which "might" evaluate to zero
        // sometimes, so we let the packetizer sort it out with some
        // conditional magic.
        // TODO Make sure this can never go haywire in weird edge cases.
        // Where we have one get_global_id() dependent on another, this is
        // not packetized correctly. Doing so is very hard!  We should
        // probably just fail to packetize in this case.  We might also be
        // able to return eBuiltinUniformityNever here, in cases where we can
        // prove that the value can never be zero.
        return eBuiltinUniformityMaybeInstanceID;
      }
      // Only vectorize on selected dimension. The value of get_global_id with
      // other ranks is uniform.
      if (Rank->getZExtValue() == SimdDimIdx) {
        return eBuiltinUniformityInstanceID;
      }

      return eBuiltinUniformityAlways;
    }
    case eMuxBuiltinGetLocalLinearId:
    case eMuxBuiltinGetGlobalLinearId:
      // TODO: This is fine for vectorizing in the x-axis, but currently we do
      // not support vectorizing along y or z (see CA-2843).
      return SimdDimIdx ? eBuiltinUniformityNever
                        : eBuiltinUniformityInstanceID;
  }
  if (LangImpl) {
    return LangImpl->isBuiltinUniform(B, CI, SimdDimIdx);
  }
  return eBuiltinUniformityUnknown;
}

Builtin BuiltinInfo::analyzeBuiltin(Function const &F) const {
  // Handle LLVM intrinsics.
  if (F.isIntrinsic()) {
    int32_t Properties = eBuiltinPropertyNone;

    Intrinsic::ID IntrID = (Intrinsic::ID)F.getIntrinsicID();
    AttributeList AS = Intrinsic::getAttributes(F.getContext(), IntrID);
    bool NoSideEffect = F.onlyReadsMemory();
    bool SafeIntrinsic = false;
    switch (IntrID) {
      default:
        SafeIntrinsic = false;
        break;
      case Intrinsic::smin:
      case Intrinsic::smax:
      case Intrinsic::umin:
      case Intrinsic::umax:
      case Intrinsic::abs:
      case Intrinsic::ctlz:
      case Intrinsic::cttz:
      case Intrinsic::sqrt:
      case Intrinsic::sin:
      case Intrinsic::cos:
      case Intrinsic::pow:
      case Intrinsic::exp:
      case Intrinsic::exp2:
      case Intrinsic::log:
      case Intrinsic::log10:
      case Intrinsic::log2:
      case Intrinsic::fma:
      case Intrinsic::fabs:
      case Intrinsic::minnum:
      case Intrinsic::maxnum:
      case Intrinsic::copysign:
      case Intrinsic::floor:
      case Intrinsic::ceil:
      case Intrinsic::trunc:
      case Intrinsic::rint:
      case Intrinsic::nearbyint:
      case Intrinsic::round:
      case Intrinsic::ctpop:
      case Intrinsic::fmuladd:
      case Intrinsic::fshl:
      case Intrinsic::fshr:
      case Intrinsic::sadd_sat:
      case Intrinsic::uadd_sat:
      case Intrinsic::ssub_sat:
      case Intrinsic::usub_sat:
      case Intrinsic::bitreverse:
        // All these function are overloadable and have both scalar and vector
        // versions.
        Properties |= eBuiltinPropertyVectorEquivalent;
        SafeIntrinsic = true;
        break;
      case Intrinsic::assume:
      case Intrinsic::dbg_declare:
      case Intrinsic::dbg_value:
      case Intrinsic::invariant_start:
      case Intrinsic::invariant_end:
      case Intrinsic::lifetime_start:
      case Intrinsic::lifetime_end:
      case Intrinsic::objectsize:
      case Intrinsic::ptr_annotation:
      case Intrinsic::var_annotation:
      case Intrinsic::experimental_noalias_scope_decl:
        SafeIntrinsic = true;
        break;
      case Intrinsic::memset:
      case Intrinsic::memcpy:
        Properties |= eBuiltinPropertyNoVectorEquivalent;
        Properties |= eBuiltinPropertySideEffects;
        break;
    }
    if (NoSideEffect || SafeIntrinsic) {
      Properties |= eBuiltinPropertyNoSideEffects;
      if (!AS.hasFnAttr(Attribute::NoDuplicate)) {
        Properties |= eBuiltinPropertySupportsInstantiation;
      }
    }
    return Builtin{F, eBuiltinUnknown, (BuiltinProperties)Properties};
  }

  BuiltinID ID = identifyMuxBuiltin(F);

  if (ID == eBuiltinInvalid) {
    // It's not a Mux builtin, so defer to the language implementation
    if (LangImpl) {
      return LangImpl->analyzeBuiltin(F);
    }
    return Builtin{F, ID, eBuiltinPropertyNone};
  }

  bool IsConvergent = false;
  unsigned Properties = eBuiltinPropertyNone;
  switch (ID) {
    default:
      break;
    case eMuxBuiltinMemBarrier:
      Properties = eBuiltinPropertySideEffects;
      break;
    case eMuxBuiltinSubGroupBarrier:
    case eMuxBuiltinWorkGroupBarrier:
      IsConvergent = true;
      Properties = eBuiltinPropertyExecutionFlow | eBuiltinPropertySideEffects;
      break;
    case eMuxBuiltinDMARead1D:
    case eMuxBuiltinDMARead2D:
    case eMuxBuiltinDMARead3D:
    case eMuxBuiltinDMAWrite1D:
    case eMuxBuiltinDMAWrite2D:
    case eMuxBuiltinDMAWrite3D:
    case eMuxBuiltinDMAWait:
      // Our DMA builtins, by default, rely on thread checks against specific
      // work-item IDs, so they must be convergent.
      IsConvergent = true;
      Properties = eBuiltinPropertyNoSideEffects;
      break;
    case eMuxBuiltinGetWorkDim:
    case eMuxBuiltinGetGroupId:
    case eMuxBuiltinGetGlobalSize:
    case eMuxBuiltinGetGlobalOffset:
    case eMuxBuiltinGetLocalSize:
    case eMuxBuiltinGetNumGroups:
    case eMuxBuiltinGetGlobalLinearId:
    case eMuxBuiltinGetLocalLinearId:
    case eMuxBuiltinGetGlobalId:
      Properties = eBuiltinPropertyWorkItem | eBuiltinPropertyRematerializable;
      break;
    case eMuxBuiltinGetLocalId:
      Properties = eBuiltinPropertyWorkItem | eBuiltinPropertyLocalID |
                   eBuiltinPropertyRematerializable;
      break;
    case eMuxBuiltinIsFTZ:
    case eMuxBuiltinIsEmbeddedProfile:
    case eMuxBuiltinUseFast:
      Properties = eBuiltinPropertyNoSideEffects;
      break;
  }
  if (!IsConvergent) {
    Properties |= eBuiltinPropertyKnownNonConvergent;
  }
  return Builtin{F, ID, (BuiltinProperties)Properties};
}

BuiltinCall BuiltinInfo::analyzeBuiltinCall(CallInst const &CI,
                                            unsigned SimdDimIdx) const {
  auto *const callee = CI.getCalledFunction();
  assert(callee && "Call instruction with no callee");
  auto const B = analyzeBuiltin(*callee);
  auto const U = isBuiltinUniform(B, &CI, SimdDimIdx);
  return BuiltinCall{B, CI, U};
}

Function *BuiltinInfo::getVectorEquivalent(Builtin const &B, unsigned Width,
                                           Module *M) {
  // We don't handle LLVM intrinsics here
  if (B.function.isIntrinsic()) {
    return nullptr;
  }

  if (LangImpl) {
    return LangImpl->getVectorEquivalent(B, Width, M);
  }
  return nullptr;
}

Function *BuiltinInfo::getScalarEquivalent(Builtin const &B, Module *M) {
  // We will first check to see if this is an LLVM intrinsic that has a scalar
  // equivalent.
  if (B.function.isIntrinsic()) {
    // Analyze the builtin. Some functions have no scalar equivalent.
    auto const Props = B.properties;
    if (!(Props & eBuiltinPropertyVectorEquivalent)) {
      return nullptr;
    }

    // Check the return type.
    auto *VecRetTy = dyn_cast<FixedVectorType>(B.function.getReturnType());
    if (!VecRetTy) {
      return nullptr;
    }

    auto IntrinsicID = B.function.getIntrinsicID();
    // Currently, we can only handle correctly intrinsics that have one
    // overloaded type, used for both the return type and all of the arguments.
    // TODO: More generic support for intrinsics with vector equivalents.
    for (Type *ArgTy : B.function.getFunctionType()->params()) {
      // If the argument isn't a vector, then it isn't going to get scalarized,
      // so don't worry about it.
      if (ArgTy->isVectorTy() && ArgTy != VecRetTy) {
        return nullptr;
      }
    }
    Type *ScalarType = VecRetTy->getElementType();
    // Get the scalar version of the intrinsic
    Function *ScalarIntrinsic =
        Intrinsic::getDeclaration(M, IntrinsicID, ScalarType);

    return ScalarIntrinsic;
  }

  if (LangImpl) {
    return LangImpl->getScalarEquivalent(B, M);
  }
  return nullptr;
}

BuiltinSubgroupReduceKind BuiltinInfo::getBuiltinSubgroupReductionKind(
    Builtin const &B) const {
  if (LangImpl) {
    return LangImpl->getBuiltinSubgroupReductionKind(B);
  }
  return eBuiltinSubgroupReduceInvalid;
}

BuiltinSubgroupScanKind BuiltinInfo::getBuiltinSubgroupScanKind(
    Builtin const &B) const {
  if (LangImpl) {
    return LangImpl->getBuiltinSubgroupScanKind(B);
  }
  return eBuiltinSubgroupScanInvalid;
}

Value *BuiltinInfo::emitBuiltinInline(Function *Builtin, IRBuilder<> &B,
                                      ArrayRef<Value *> Args) {
  if (LangImpl) {
    return LangImpl->emitBuiltinInline(Builtin, B, Args);
  }
  return nullptr;
}

multi_llvm::Optional<llvm::ConstantRange> BuiltinInfo::getBuiltinRange(
    CallInst &CI, std::array<multi_llvm::Optional<uint64_t>, 3> MaxLocalSizes,
    std::array<multi_llvm::Optional<uint64_t>, 3> MaxGlobalSizes) const {
  if (LangImpl) {
    return LangImpl->getBuiltinRange(CI, MaxLocalSizes, MaxGlobalSizes);
  }
  return multi_llvm::None;
}

CallInst *BuiltinInfo::mapSyncBuiltinToMuxSyncBuiltin(CallInst &CI) {
  if (LangImpl) {
    return LangImpl->mapSyncBuiltinToMuxSyncBuiltin(CI, *MuxImpl);
  }
  // We shouldn't be mapping mux builtins to mux builtins, so we can stop here.
  return nullptr;
}

BuiltinID BuiltinInfo::getPrintfBuiltin() const {
  if (LangImpl) {
    return LangImpl->getPrintfBuiltin();
  }
  return eBuiltinInvalid;
}

BuiltinID BuiltinInfo::getSubgroupLocalIdBuiltin() const {
  if (LangImpl) {
    return LangImpl->getSubgroupLocalIdBuiltin();
  }
  return eBuiltinInvalid;
}

BuiltinID BuiltinInfo::getSubgroupBroadcastBuiltin() const {
  if (LangImpl) {
    return LangImpl->getSubgroupBroadcastBuiltin();
  }
  return eBuiltinInvalid;
}

bool BuiltinInfo::requiresSchedulingParameters(BuiltinID ID) {
  // Defer to mux for the scheduling parameters.
  return MuxImpl->requiresSchedulingParameters(ID);
}

Type *BuiltinInfo::getRemappedTargetExtTy(Type *Ty) {
  // Defer to mux for the scheduling parameters.
  return MuxImpl->getRemappedTargetExtTy(Ty);
}

SmallVector<BuiltinInfo::SchedParamInfo, 4>
BuiltinInfo::getMuxSchedulingParameters(Module &M) {
  // Defer to mux for the scheduling parameters.
  return MuxImpl->getMuxSchedulingParameters(M);
}

SmallVector<BuiltinInfo::SchedParamInfo, 4>
BuiltinInfo::getFunctionSchedulingParameters(Function &F) {
  // Defer to mux for the scheduling parameters.
  return MuxImpl->getFunctionSchedulingParameters(F);
}

Value *BuiltinInfo::initializeSchedulingParamForWrappedKernel(
    const SchedParamInfo &Info, IRBuilder<> &B, Function &IntoF,
    Function &CalleeF) {
  return MuxImpl->initializeSchedulingParamForWrappedKernel(Info, B, IntoF,
                                                            CalleeF);
}

StringRef BuiltinInfo::getMuxBuiltinName(BuiltinID ID) {
  assert(isMuxBuiltinID(ID));
  switch (ID) {
    default:
      llvm_unreachable("Unhandled mux builtin");
    case eMuxBuiltinIsFTZ:
      return MuxBuiltins::isftz;
    case eMuxBuiltinUseFast:
      return MuxBuiltins::usefast;
    case eMuxBuiltinIsEmbeddedProfile:
      return MuxBuiltins::isembeddedprofile;
    case eMuxBuiltinGetGlobalSize:
      return MuxBuiltins::get_global_size;
    case eMuxBuiltinGetGlobalId:
      return MuxBuiltins::get_global_id;
    case eMuxBuiltinGetGlobalOffset:
      return MuxBuiltins::get_global_offset;
    case eMuxBuiltinGetLocalSize:
      return MuxBuiltins::get_local_size;
    case eMuxBuiltinGetLocalId:
      return MuxBuiltins::get_local_id;
    case eMuxBuiltinSetLocalId:
      return MuxBuiltins::set_local_id;
    case eMuxBuiltinGetSubGroupId:
      return MuxBuiltins::get_sub_group_id;
    case eMuxBuiltinSetSubGroupId:
      return MuxBuiltins::set_sub_group_id;
    case eMuxBuiltinGetNumGroups:
      return MuxBuiltins::get_num_groups;
    case eMuxBuiltinGetNumSubGroups:
      return MuxBuiltins::get_num_sub_groups;
    case eMuxBuiltinSetNumSubGroups:
      return MuxBuiltins::set_num_sub_groups;
    case eMuxBuiltinGetMaxSubGroupSize:
      return MuxBuiltins::get_max_sub_group_size;
    case eMuxBuiltinSetMaxSubGroupSize:
      return MuxBuiltins::set_max_sub_group_size;
    case eMuxBuiltinGetGroupId:
      return MuxBuiltins::get_group_id;
    case eMuxBuiltinGetWorkDim:
      return MuxBuiltins::get_work_dim;
    case eMuxBuiltinDMARead1D:
      return MuxBuiltins::dma_read_1d;
    case eMuxBuiltinDMARead2D:
      return MuxBuiltins::dma_read_2d;
    case eMuxBuiltinDMARead3D:
      return MuxBuiltins::dma_read_3d;
    case eMuxBuiltinDMAWrite1D:
      return MuxBuiltins::dma_write_1d;
    case eMuxBuiltinDMAWrite2D:
      return MuxBuiltins::dma_write_2d;
    case eMuxBuiltinDMAWrite3D:
      return MuxBuiltins::dma_write_3d;
    case eMuxBuiltinDMAWait:
      return MuxBuiltins::dma_wait;
    case eMuxBuiltinGetGlobalLinearId:
      return MuxBuiltins::get_global_linear_id;
    case eMuxBuiltinGetLocalLinearId:
      return MuxBuiltins::get_local_linear_id;
    case eMuxBuiltinGetEnqueuedLocalSize:
      return MuxBuiltins::get_enqueued_local_size;
    case eMuxBuiltinMemBarrier:
      return MuxBuiltins::mem_barrier;
    case eMuxBuiltinWorkGroupBarrier:
      return MuxBuiltins::work_group_barrier;
    case eMuxBuiltinSubGroupBarrier:
      return MuxBuiltins::sub_group_barrier;
  }
}

Function *BuiltinInfo::defineMuxBuiltin(BuiltinID ID, Module &M) {
  assert(isMuxBuiltinID(ID) && "Only handling mux builtins");
  Function *F = M.getFunction(getMuxBuiltinName(ID));
  // FIXME: We'd ideally want to declare it here to reduce pass
  // inter-dependencies.
  assert(F && "Function should have been pre-declared");
  if (!F->isDeclaration()) {
    return F;
  }
  // Defer to the mux implementation to define this builtin.
  return MuxImpl->defineMuxBuiltin(ID, M);
}

Function *BuiltinInfo::getOrDeclareMuxBuiltin(BuiltinID ID, Module &M) {
  assert(isMuxBuiltinID(ID) && "Only handling mux builtins");
  // Defer to the mux implementation to get/declare this builtin.
  return MuxImpl->getOrDeclareMuxBuiltin(ID, M);
}

}  // namespace utils
}  // namespace compiler
