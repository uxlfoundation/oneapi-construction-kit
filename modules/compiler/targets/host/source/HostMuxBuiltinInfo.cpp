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

#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <host/host_mux_builtin_info.h>
#include <llvm/IR/Module.h>

#include <optional>

using namespace host;
using namespace llvm;

namespace SchedParamIndices {
enum {
  WI = 0,
  SCHED = 1,
  MINIWG = 2,
};
}

StructType *HostBIMuxInfo::getMiniWGInfoStruct(Module &M) {
  static constexpr const char *HostStructName = "MiniWGInfo";
  if (auto *ty = StructType::getTypeByName(M.getContext(), HostStructName)) {
    return ty;
  }

  auto *const size_type = compiler::utils::getSizeType(M);
  auto *const array_type = ArrayType::get(size_type, 3);

  SmallVector<Type *, ScheduleInfoStruct::total> elements(2);

  elements[MiniWGInfoStruct::group_id] = array_type;
  elements[MiniWGInfoStruct::num_groups] = array_type;

  return StructType::create(elements, HostStructName);
}

StructType *HostBIMuxInfo::getScheduleInfoStruct(Module &M) {
  static constexpr const char *HostStructName = "Mux_schedule_info_s";
  if (auto *ty = StructType::getTypeByName(M.getContext(), HostStructName)) {
    return ty;
  }
  auto &Ctx = M.getContext();
  auto uint_type = Type::getInt32Ty(Ctx);
  // Define size_t based on the pointer size in the default address space (0).
  // This won't necessarily be correct for all devices but it's a good enough
  // guess for host.
  auto *const size_type = compiler::utils::getSizeType(M);
  auto *const array_type = ArrayType::get(size_type, 3);

  SmallVector<Type *, ScheduleInfoStruct::total> elements(
      ScheduleInfoStruct::total);

  elements[ScheduleInfoStruct::global_size] = array_type;
  elements[ScheduleInfoStruct::global_offset] = array_type;
  elements[ScheduleInfoStruct::local_size] = array_type;
  elements[ScheduleInfoStruct::slice] = size_type;
  elements[ScheduleInfoStruct::total_slices] = size_type;
  elements[ScheduleInfoStruct::work_dim] = uint_type;

  return StructType::create(elements, HostStructName);
}

SmallVector<compiler::utils::BuiltinInfo::SchedParamInfo, 4>
HostBIMuxInfo::getMuxSchedulingParameters(Module &M) {
  auto &Ctx = M.getContext();
  auto &DL = M.getDataLayout();
  AttributeSet DefaultAttrs;
  DefaultAttrs = DefaultAttrs.addAttribute(Ctx, Attribute::NonNull);
  DefaultAttrs = DefaultAttrs.addAttribute(Ctx, Attribute::NoAlias);

  compiler::utils::BuiltinInfo::SchedParamInfo WIInfo;
  {
    auto *const WIInfoS = compiler::utils::getWorkItemInfoStructTy(M);
    WIInfo.ID = SchedParamIndices::WI;
    WIInfo.ParamTy = WIInfoS->getPointerTo();
    WIInfo.ParamPointeeTy = WIInfoS;
    WIInfo.ParamName = "wi-info";
    WIInfo.ParamDebugName = WIInfoS->getStructName().str();
    WIInfo.PassedExternally = false;

    auto AB = AttrBuilder(Ctx, DefaultAttrs);
    AB.addAlignmentAttr(DL.getABITypeAlign(WIInfoS));
    AB.addDereferenceableAttr(DL.getTypeAllocSize(WIInfoS));
    WIInfo.ParamAttrs = AttributeSet::get(Ctx, AB);
  }

  compiler::utils::BuiltinInfo::SchedParamInfo SchedInfo;
  {
    auto *const SchedInfoS = getScheduleInfoStruct(M);
    SchedInfo.ID = SchedParamIndices::SCHED;
    SchedInfo.ParamTy = SchedInfoS->getPointerTo();
    SchedInfo.ParamPointeeTy = SchedInfoS;
    SchedInfo.ParamName = "sched-info";
    SchedInfo.ParamDebugName = SchedInfoS->getStructName().str();
    SchedInfo.PassedExternally = true;

    auto AB = AttrBuilder(Ctx, DefaultAttrs);
    AB.addAlignmentAttr(DL.getABITypeAlign(SchedInfoS));
    AB.addDereferenceableAttr(DL.getTypeAllocSize(SchedInfoS));
    SchedInfo.ParamAttrs = AttributeSet::get(Ctx, AB);
  }

  compiler::utils::BuiltinInfo::SchedParamInfo WGInfo;
  {
    auto *const WGInfoS = getMiniWGInfoStruct(M);
    WGInfo.ID = SchedParamIndices::MINIWG;
    WGInfo.ParamTy = WGInfoS->getPointerTo();
    WGInfo.ParamPointeeTy = WGInfoS;
    WGInfo.ParamName = "mini-wg-info";
    WGInfo.ParamDebugName = WGInfoS->getStructName().str();
    WGInfo.PassedExternally = false;

    auto AB = AttrBuilder(Ctx, DefaultAttrs);
    AB.addAlignmentAttr(DL.getABITypeAlign(WGInfoS));
    AB.addDereferenceableAttr(DL.getTypeAllocSize(WGInfoS));
    WGInfo.ParamAttrs = AttributeSet::get(Ctx, AB);
  }

  return {WIInfo, SchedInfo, WGInfo};
}

Function *HostBIMuxInfo::defineMuxBuiltin(compiler::utils::BuiltinID ID,
                                          Module &M,
                                          ArrayRef<Type *> OverloadInfo) {
  assert(compiler::utils::BuiltinInfo::isMuxBuiltinID(ID) &&
         "Only handling mux builtins");
  Function *F = M.getFunction(
      compiler::utils::BuiltinInfo::getMuxBuiltinName(ID, OverloadInfo));
  // FIXME: We'd ideally want to declare it here to reduce pass
  // inter-dependencies.
  assert(F && "Function should have been pre-declared");
  if (!F->isDeclaration()) {
    return F;
  }

  bool HasRankArg = true;
  size_t DefaultVal = 0;
  std::optional<unsigned> ParamIdx;
  std::optional<unsigned> WGFieldIdx;

  switch (ID) {
    default:
      return compiler::utils::BIMuxInfoConcept::defineMuxBuiltin(ID, M,
                                                                 OverloadInfo);
    case compiler::utils::eMuxBuiltinGetLocalSize:
      ParamIdx = SchedParamIndices::SCHED;
      DefaultVal = 1;
      WGFieldIdx = ScheduleInfoStruct::local_size;
      break;
    case compiler::utils::eMuxBuiltinGetGroupId:
      ParamIdx = SchedParamIndices::MINIWG;
      DefaultVal = 0;
      WGFieldIdx = MiniWGInfoStruct::group_id;
      break;
    case compiler::utils::eMuxBuiltinGetNumGroups:
      ParamIdx = SchedParamIndices::MINIWG;
      DefaultVal = 1;
      WGFieldIdx = MiniWGInfoStruct::num_groups;
      break;
    case compiler::utils::eMuxBuiltinGetGlobalOffset:
      ParamIdx = SchedParamIndices::SCHED;
      DefaultVal = 0;
      WGFieldIdx = ScheduleInfoStruct::global_offset;
      break;
    case compiler::utils::eMuxBuiltinGetWorkDim:
      ParamIdx = SchedParamIndices::SCHED;
      DefaultVal = 1;
      HasRankArg = false;
      WGFieldIdx = ScheduleInfoStruct::work_dim;
      break;
  }

  assert(F && WGFieldIdx && ParamIdx);

  auto SchedParams = getFunctionSchedulingParameters(*F);
  assert(SchedParams.size() > *ParamIdx && "Missing scheduling parameters");

  const auto &SchedParam = SchedParams[*ParamIdx];
  compiler::utils::populateStructGetterFunction(
      *F, *SchedParam.ArgVal, cast<StructType>(SchedParam.ParamPointeeTy),
      *WGFieldIdx, HasRankArg, DefaultVal);

  return nullptr;
}

Value *HostBIMuxInfo::initializeSchedulingParamForWrappedKernel(
    const compiler::utils::BuiltinInfo::SchedParamInfo &Info, IRBuilder<> &B,
    Function &IntoF, Function &) {
  auto &M = *IntoF.getParent();
  assert(!Info.PassedExternally);
  // Stack-allocate work-item info, not initializing anything. The work-item
  // loops initialize all data here. This is the default behaviour
  if (Info.ID == SchedParamIndices::WI && Info.ParamName == "wi-info") {
    assert(Info.ParamPointeeTy == compiler::utils::getWorkItemInfoStructTy(M) &&
           "Unexpected work-item info type");
    return B.CreateAlloca(Info.ParamPointeeTy,
                          /*ArraySize*/ nullptr, Info.ParamName);
  }

  // Stack-allocate the mini work-group info, initializing only the
  // 'num_groups' via data from the scheduling struct. The 'group_id' field is
  // initialized by the work-group loops before it's ever used.
  if (Info.ID == SchedParamIndices::MINIWG &&
      Info.ParamName == "mini-wg-info") {
    auto IntoSchedParams = getFunctionSchedulingParameters(IntoF);
    const auto &SchedParam = IntoSchedParams[SchedParamIndices::SCHED];
    auto *const SchedInfo = SchedParam.ArgVal;
    assert(SchedInfo && "Should have access to the scheduling info struct");
    auto *const I32Zero = B.getInt32(0);
    auto *const I32One = B.getInt32(1);
    auto *const I32Two = B.getInt32(2);
    auto *const SizeTy = compiler::utils::getSizeType(M);
    auto *const MiniWGInfoStructTy = cast<StructType>(Info.ParamPointeeTy);
    auto *const SchedInfoStructTy = cast<StructType>(SchedParam.ParamPointeeTy);
    assert(MiniWGInfoStructTy == getMiniWGInfoStruct(M) &&
           SchedInfoStructTy == getScheduleInfoStruct(M) &&
           "Unexpected scheduling parameter types");

    auto *const AllocaI = B.CreateAlloca(MiniWGInfoStructTy);

    auto *GEPGlobalSize =
        B.CreateGEP(SchedInfoStructTy, SchedInfo,
                    {I32Zero, B.getInt32(ScheduleInfoStruct::global_size)});
    auto *GEPLocalSize =
        B.CreateGEP(SchedInfoStructTy, SchedInfo,
                    {I32Zero, B.getInt32(ScheduleInfoStruct::local_size)});

    auto *const GEPGlobalSizeTy =
        SchedInfoStructTy->getTypeAtIndex(ScheduleInfoStruct::global_size);
    auto *GEPLocalSizeTy =
        SchedInfoStructTy->getTypeAtIndex(ScheduleInfoStruct::local_size);

    Value *GlobalSizes[3] = {
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPGlobalSizeTy, GEPGlobalSize, {I32Zero, I32Zero}),
            "global_size_x"),
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPGlobalSizeTy, GEPGlobalSize, {I32Zero, I32One}),
            "global_size_y"),
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPGlobalSizeTy, GEPGlobalSize, {I32Zero, I32Two}),
            "global_size_z"),
    };

    Value *LocalSizes[3] = {
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPLocalSizeTy, GEPLocalSize, {I32Zero, I32Zero}),
            "local_size_x"),
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPLocalSizeTy, GEPLocalSize, {I32Zero, I32One}),
            "local_size_y"),
        B.CreateLoad(
            SizeTy,
            B.CreateGEP(GEPLocalSizeTy, GEPLocalSize, {I32Zero, I32Two}),
            "local_size_z"),
    };

    auto *DstNumGroups =
        B.CreateGEP(MiniWGInfoStructTy, AllocaI,
                    {I32Zero, B.getInt32(ScheduleInfoStruct::global_offset)});

    // calculate the number of work groups we are running
    std::array<Value *, 3> NumGroups;
    if (auto LocalSize = compiler::utils::getLocalSizeMetadata(IntoF)) {
      NumGroups = {
          B.CreateUDiv(GlobalSizes[0],
                       ConstantInt::get(SizeTy, (*LocalSize)[0]),
                       "num_groups_x"),
          B.CreateUDiv(GlobalSizes[1],
                       ConstantInt::get(SizeTy, (*LocalSize)[1]),
                       "num_groups_y"),
          B.CreateUDiv(GlobalSizes[2],
                       ConstantInt::get(SizeTy, (*LocalSize)[2]),
                       "num_groups_z"),
      };
    } else {  // use runtime scheduling info load for local size
      NumGroups = {
          B.CreateUDiv(GlobalSizes[0], LocalSizes[0], "num_groups_x"),
          B.CreateUDiv(GlobalSizes[1], LocalSizes[1], "num_groups_y"),
          B.CreateUDiv(GlobalSizes[2], LocalSizes[2], "num_groups_z"),
      };
    }

    // copy our num groups into the work group info structs
    auto *DstNumGroupTy =
        MiniWGInfoStructTy->getTypeAtIndex(MiniWGInfoStruct::num_groups);
    B.CreateStore(NumGroups[0],
                  B.CreateGEP(DstNumGroupTy, DstNumGroups, {I32Zero, I32Zero}));
    B.CreateStore(NumGroups[1],
                  B.CreateGEP(DstNumGroupTy, DstNumGroups, {I32Zero, I32One}));
    B.CreateStore(NumGroups[2],
                  B.CreateGEP(DstNumGroupTy, DstNumGroups, {I32Zero, I32Two}));

    return AllocaI;
  }
  assert(false && "unknown param");
  return nullptr;
}
