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

/// @file
///
/// @brief Defines the work-group collective builtins.

#include <compiler/utils/address_spaces.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/dma.h>
#include <compiler/utils/group_collective_helpers.h>
#include <compiler/utils/mangling.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/replace_wgc_pass.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/ErrorHandling.h>
#include <multi_llvm/multi_llvm.h>

using namespace llvm;

namespace {

/// @brief Helper function that inserts a local barrier call via a builder.
///
/// @param[in] Builder ir builder used to create the barrier call. This function
/// will create the call at the current insert point of the builder.
CallInst *createLocalBarrierCall(IRBuilder<> &Builder,
                                 compiler::utils::BuiltinInfo &BI) {
  auto *const M = Builder.GetInsertBlock()->getModule();
  auto *const Barrier = BI.getOrDeclareMuxBuiltin(
      compiler::utils::eMuxBuiltinWorkGroupBarrier, *M);
  assert(Barrier && "__mux_work_group_barrier is not in module");

  auto *ID = Builder.getInt32(0);
  auto *Scope =
      Builder.getInt32(compiler::utils::BIMuxInfoConcept::MemScopeWorkGroup);
  auto *Semantics = Builder.getInt32(
      compiler::utils::BIMuxInfoConcept::MemSemanticsSequentiallyConsistent |
      compiler::utils::BIMuxInfoConcept::MemSemanticsWorkGroupMemory);
  auto *const BarrierCall =
      CallInst::Create(Barrier->getFunctionType(), Barrier,
                       {ID, Scope, Semantics}, "", Builder.GetInsertBlock());
  return BarrierCall;
}

/// @brief Helper function to create subgroup reduction calls.
Value *createSubgroupReduction(IRBuilder<> &Builder, llvm::Value *Src,
                               const compiler::utils::GroupCollective &WGC,
                               compiler::utils::BuiltinInfo &BI) {
  using namespace compiler::utils;
  BuiltinID ReductionID = eBuiltinInvalid;
  switch (WGC.Recurrence) {
    default:
      return nullptr;
    case RecurKind::And:
      if (WGC.isAnyAll()) {
        ReductionID = eMuxBuiltinSubgroupAll;
      } else {
        ReductionID = !WGC.IsLogical ? eMuxBuiltinSubgroupReduceAnd
                                     : eMuxBuiltinSubgroupReduceLogicalAnd;
      }
      break;
    case RecurKind::Or:
      if (WGC.isAnyAll()) {
        ReductionID = eMuxBuiltinSubgroupAny;
      } else {
        ReductionID = !WGC.IsLogical ? eMuxBuiltinSubgroupReduceOr
                                     : eMuxBuiltinSubgroupReduceLogicalOr;
      }
      break;
    case RecurKind::Add:
      ReductionID = eMuxBuiltinSubgroupReduceAdd;
      break;
    case RecurKind::FAdd:
      ReductionID = eMuxBuiltinSubgroupReduceFAdd;
      break;
    case RecurKind::UMin:
      ReductionID = eMuxBuiltinSubgroupReduceUMin;
      break;
    case RecurKind::SMin:
      ReductionID = eMuxBuiltinSubgroupReduceSMin;
      break;
    case RecurKind::FMin:
      ReductionID = eMuxBuiltinSubgroupReduceFMin;
      break;
    case RecurKind::UMax:
      ReductionID = eMuxBuiltinSubgroupReduceUMax;
      break;
    case RecurKind::SMax:
      ReductionID = eMuxBuiltinSubgroupReduceSMax;
      break;
    case RecurKind::FMax:
      ReductionID = eMuxBuiltinSubgroupReduceFMax;
      break;
    case RecurKind::Mul:
      ReductionID = eMuxBuiltinSubgroupReduceMul;
      break;
    case RecurKind::FMul:
      ReductionID = eMuxBuiltinSubgroupReduceFMul;
      break;
    case RecurKind::Xor:
      ReductionID = !WGC.IsLogical ? eMuxBuiltinSubgroupReduceXor
                                   : eMuxBuiltinSubgroupReduceLogicalXor;
      break;
  }
  assert(ReductionID != eBuiltinInvalid);

  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();

  Function *Builtin = BI.getOrDeclareMuxBuiltin(ReductionID, *M, {Ty});
  assert(Builtin && "Missing reduction builtin");

  return Builder.CreateCall(Builtin, {Src}, "wgc");
}

Value *createSubgroupScan(IRBuilder<> &Builder, llvm::Value *Src,
                          RecurKind Kind, bool IsInclusive, bool IsLogical,
                          compiler::utils::BuiltinInfo &BI) {
  using namespace compiler::utils;
  BuiltinID ScanBuiltinID = eBuiltinInvalid;
  switch (Kind) {
    default:
      return nullptr;
    case RecurKind::Add:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanAddInclusive
                                  : eMuxBuiltinSubgroupScanAddExclusive;
      break;
    case RecurKind::FAdd:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanFAddInclusive
                                  : eMuxBuiltinSubgroupScanFAddExclusive;
      break;
    case RecurKind::SMin:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanSMinInclusive
                                  : eMuxBuiltinSubgroupScanSMinExclusive;
      break;
    case RecurKind::UMin:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanUMinInclusive
                                  : eMuxBuiltinSubgroupScanUMinExclusive;
      break;
    case RecurKind::FMin:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanFMinInclusive
                                  : eMuxBuiltinSubgroupScanFMinExclusive;
      break;
    case RecurKind::SMax:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanSMaxInclusive
                                  : eMuxBuiltinSubgroupScanSMaxExclusive;
      break;
    case RecurKind::UMax:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanUMaxInclusive
                                  : eMuxBuiltinSubgroupScanUMaxExclusive;
      break;
    case RecurKind::FMax:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanFMaxInclusive
                                  : eMuxBuiltinSubgroupScanFMaxExclusive;
      break;
    case RecurKind::Mul:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanMulInclusive
                                  : eMuxBuiltinSubgroupScanMulExclusive;
      break;
    case RecurKind::FMul:
      ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanFMulInclusive
                                  : eMuxBuiltinSubgroupScanFMulExclusive;
      break;
    case RecurKind::And:
      if (!IsLogical) {
        ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanAndInclusive
                                    : eMuxBuiltinSubgroupScanAndExclusive;
      } else {
        ScanBuiltinID = IsInclusive
                            ? eMuxBuiltinSubgroupScanLogicalAndInclusive
                            : eMuxBuiltinSubgroupScanLogicalAndExclusive;
      }
      break;
    case RecurKind::Or:
      if (!IsLogical) {
        ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanOrInclusive
                                    : eMuxBuiltinSubgroupScanOrExclusive;
      } else {
        ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanLogicalOrInclusive
                                    : eMuxBuiltinSubgroupScanLogicalOrExclusive;
      }
      break;
    case RecurKind::Xor:
      if (!IsLogical) {
        ScanBuiltinID = IsInclusive ? eMuxBuiltinSubgroupScanXorInclusive
                                    : eMuxBuiltinSubgroupScanXorExclusive;
      } else {
        ScanBuiltinID = IsInclusive
                            ? eMuxBuiltinSubgroupScanLogicalXorInclusive
                            : eMuxBuiltinSubgroupScanLogicalXorExclusive;
      }
      break;
  }
  assert(ScanBuiltinID != eBuiltinInvalid);

  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();

  Function *Builtin = BI.getOrDeclareMuxBuiltin(ScanBuiltinID, *M, {Ty});
  assert(Builtin && "Missing scan builtin");

  return Builder.CreateCall(Builtin, {Src}, "wgc_scan");
}

/// @brief Helper function to create get subgroup size calls.
Value *createGetSubgroupSize(IRBuilder<> &Builder,
                             compiler::utils::BuiltinInfo &BI,
                             const Twine &Name = Twine()) {
  auto &M = *Builder.GetInsertBlock()->getModule();
  auto *const Builtin =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetSubGroupSize, M);
  assert(Builtin && "__mux_get_sub_group_size is not in module");

  return Builder.CreateCall(Builtin, {}, Name);
}

/// @brief Helper function to create subgroup broadcast calls.
Value *createSubgroupBroadcast(IRBuilder<> &Builder, Value *Src, Value *ID,
                               compiler::utils::BuiltinInfo &BI,
                               const Twine &Name = Twine()) {
  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();

  auto *Builtin = BI.getOrDeclareMuxBuiltin(
      compiler::utils::eMuxBuiltinSubgroupBroadcast, *M, {Ty});

  return Builder.CreateCall(Builtin, {Src, ID}, Name);
}

/// @brief Helper function to emit the binary op on the global accumulator
///
/// @param[in] Builder IR builder to build the operation with.
/// @param[in] CurrentVal The global accumulator which forms the lhs of the
/// binary operation.
/// @param[in] Operand The rhs of the binary operation.
/// @param[in] Kind The operation kind.
///
/// @return The result of the operation.
Value *createBinOp(llvm::IRBuilder<> &Builder, llvm::Value *CurrentVal,
                   llvm::Value *Operand, RecurKind Kind) {
  return compiler::utils::createBinOpForRecurKind(Builder, CurrentVal, Operand,
                                                  Kind);
}

/// @brief Helper function to define the work-group collective reductions.
///
/// param[in] F work-group collective reduction to define, must be one of
/// work_group_any, work_group_all, work_group_reduce_add,
/// work_group_reduce_min, work_group_reduce_max
///
/// In terms of CL C this function defines a work-group reduction as follows:
///
/// local T accumulator;
/// T work_group_reduce_<op>(T x) {
///    reduce = sub_group_reduce_<op>(x);
///
///    barrier(CLK_LOCAL_MEM_FENCE); // BarrierSchedule = Once
///    accumulator = I;
///
///    barrier(CLK_LOCAL_MEM_FENCE);
///    accumulator += reduce;
///
///    barrier(CLK_LOCAL_MEM_FENCE);
///    T result = accumulator;
///    return result;
/// }
///
///  where I is the neutral value for the operation <op> on type T. This
///  function also handles work_group_all and work_group_any since they are
///  essentially work_group_reduce_and work_group_reduce_or on the int type
///  only.
void emitWorkGroupReductionBody(Function &F,
                                const compiler::utils::GroupCollective &WGC,
                                compiler::utils::BuiltinInfo &BI) {
  // Create a global variable to do the reduction on.
  auto *const Operand = F.getArg(1);
  auto *const ReductionType{Operand->getType()};
  auto *const ReductionNeutralValue{
      compiler::utils::getNeutralVal(WGC.Recurrence, ReductionType)};
  auto *const Accumulator =
      new GlobalVariable{*F.getParent(),
                         ReductionType,
                         /* isConstant */ false,
                         GlobalVariable::LinkageTypes::InternalLinkage,
                         UndefValue::get(ReductionType),
                         F.getName() + ".accumulator",
                         /* InsertBefore */ nullptr,
                         GlobalVariable::ThreadLocalMode::NotThreadLocal,
                         compiler::utils::AddressSpace::Local};

  auto &Ctx = F.getContext();
  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);

  IRBuilder<> Builder{EntryBB};

  // We can create the subgroup reduction *before* the barrier, since its
  // implementation does not involve memory access. This way, when it gets
  // vectorized, only the scalar result will need to be in the barrier struct,
  // not its vectorized operand.
  auto *const SubReduce = createSubgroupReduction(Builder, Operand, WGC, BI);
  assert(SubReduce && "Invalid subgroup reduce");

  // We need three barriers:
  // The barrier after the store ensures that the initialization is complete
  // before the accumulation begins. The barrer after the accumulation ensures
  // that the result is complete for reloading afterwards. And this, the first
  // barrier, ensures that if there are two (or more) calls to this function,
  // multiple uses of the same accumulator cannot get tangled up.
  compiler::utils::setBarrierSchedule(*createLocalBarrierCall(Builder, BI),
                                      compiler::utils::BarrierSchedule::Once);

  // Initialize the accumulator.
  Builder.CreateStore(ReductionNeutralValue, Accumulator);
  createLocalBarrierCall(Builder, BI);

  // Read-modify-write the accumulator.
  auto *const CurrentVal =
      Builder.CreateLoad(ReductionType, Accumulator, "current.val");
  auto *const NextVal =
      createBinOp(Builder, CurrentVal, SubReduce, WGC.Recurrence);
  Builder.CreateStore(NextVal, Accumulator);

  // Barrier, then read result and exit.
  createLocalBarrierCall(Builder, BI);
  auto *const Result = Builder.CreateLoad(ReductionType, Accumulator);

  Builder.CreateRet(Result);
}

/// @brief Helper function to define the work-group collective broadcasts.
///
/// param[in] F work-group collective broadcast to define, must be one of
/// work_group_broadcast(x, local_id), work_group_broadcast(x, local_id_x,
/// local_id_y) or work_group_broadcast(x, local_id_x, local_id_y, local_id_z).
///
/// In terms of CL C this function defines a work-group reduction as follows:
///
/// local T broadcast;
/// T work_group_broadcast(T a, local_id {x, y z}) {
///   if(get_local_id(0) == x && get_local_id(1) == y && get_local_id(2) == z) {
///     broadcast = a;
///   }
///
///   barrier(CLK_LOCAL_MEM_FENCE);
///   T result = broadcast;
///   barrier(CLK_LOCAL_MEM_FENCE);
///   return result;
/// }
void emitWorkGroupBroadcastBody(Function &F,
                                const compiler::utils::GroupCollective &,
                                compiler::utils::BuiltinInfo &BI) {
  // First arg is always the value to broadcast.
  auto *const ValueToBroadcast = F.getArg(1);

  // Create a global variable to do the broadcast through.
  auto *const BroadcastType{ValueToBroadcast->getType()};
  auto &M = *F.getParent();
  auto *const Broadcast =
      new GlobalVariable{M,
                         BroadcastType,
                         /* isConstant */ false,
                         GlobalVariable::LinkageTypes::InternalLinkage,
                         UndefValue::get(BroadcastType),
                         F.getName() + ".accumulator",
                         /* InsertBefore */ nullptr,
                         GlobalVariable::ThreadLocalMode::NotThreadLocal,
                         compiler::utils::AddressSpace::Local};

  // Create the basic blocks the function will contain.
  auto &Ctx = F.getContext();
  auto *const ExitBB = BasicBlock::Create(Ctx, "exit", &F);
  auto *const BroadcastBB = BasicBlock::Create(Ctx, "broadcast", &F, ExitBB);
  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F, BroadcastBB);
  IRBuilder<> Builder{EntryBB};

  // Check if we are on the thread that needs to broadcast.
  auto *const GetLocalID =
      BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalId, M);
  assert(GetLocalID && "get_local_id is not in module");
  GetLocalID->setCallingConv(llvm::CallingConv::SPIR_FUNC);

  Value *IsBroadcastingThread = ConstantInt::getTrue(Ctx);
  for (unsigned i = 2; i < F.arg_size(); ++i) {
    Value *LocalIDCall = Builder.CreateCall(
        GetLocalID, ConstantInt::get(GetLocalID->getArg(0)->getType(), i - 2));
    LocalIDCall = Builder.CreateIntCast(LocalIDCall, F.getArg(i)->getType(),
                                        /* isSigned */ false);
    auto *LocalIDCmp = Builder.CreateICmpEQ(LocalIDCall, F.getArg(i));
    IsBroadcastingThread = Builder.CreateAnd(IsBroadcastingThread, LocalIDCmp);
  }
  Builder.CreateCondBr(IsBroadcastingThread, BroadcastBB, ExitBB);

  // Set up the broadcast.
  Builder.SetInsertPoint(BroadcastBB);
  Builder.CreateStore(ValueToBroadcast, Broadcast);
  Builder.CreateBr(ExitBB);

  // Now synchronize to ensure the broadcast is visible to all threads.
  Builder.SetInsertPoint(ExitBB);
  createLocalBarrierCall(Builder, BI);

  // Load the result and synchronize a second time to ensure no eager threads
  // update the local value before any work-item reads it.
  auto *const Result =
      Builder.CreateLoad(BroadcastType, Broadcast, "broadcast");
  createLocalBarrierCall(Builder, BI);
  Builder.CreateRet(Result);
}

/// @brief Helper function to define the work-group collective scans.
///
/// param[in] F work-group collective scan to define, must be one of
/// work_group_exclusive_scan_add, work_group_exclusive_scan_min,
/// work_group_exclusive_scan_max, work_group_inclusive_scan_add,
/// work_group_inclusive_scan_min, work_group_inclusive_scan_max.
///
///
/// In terms of CL C this function defines a work-group inclusive scan as
/// follows:
///
/// local T accumulator;
/// T work_group_<inclusive:exclusive>_scan_<op>(T x) {
///    barrier(CLK_LOCAL_MEM_FENCE);  // Schedule = Once
///    accumulator = I;
///    barrier(CLK_LOCAL_MEM_FENCE);  // Schedule = Linear
///
///    T scan = sub_group_scan_<op>(x);
///    T result = accumulator + scan;
///
///    uint last = get_sub_group_size() - 1;
///    T reduce = sub_group_broadcast(scan, last);
/// #if exclusive
///    reduce += sub_group_broadcast(x, last);
/// #endif
///    accumulator += reduce;
///    barrier(CLK_LOCAL_MEM_FENCE);
///
///    return result;
/// }
///
/// where I is the neutral value value for the operation <op> on type T.
/// For exclusive scans on FMin and FMax, there is an added complexity caused
/// by the zeroth element of a scan, which is +/-INFINITY, but the true neutral
/// value of these operations is NaN. Thus we have to replace the zeroth element
/// of the subgroup scan with Nan, and replace the zeroth element of the final
/// result with +/INFINITY. There is a similar situation for FAdd, where the
/// identity element is defined to be `0.0` but the true neutral value is
/// `-0.0`.
void emitWorkGroupScanBody(Function &F,
                           const compiler::utils::GroupCollective &WGC,
                           compiler::utils::BuiltinInfo &BI) {
  // Create a global variable to do the scan on.
  auto *const Operand = F.getArg(1);
  auto *const ReductionType{Operand->getType()};
  auto *const ReductionNeutralValue{
      compiler::utils::getNeutralVal(WGC.Recurrence, ReductionType)};
  assert(ReductionNeutralValue && "Invalid neutral value");
  auto &M = *F.getParent();
  auto *const Accumulator =
      new GlobalVariable{M,
                         ReductionType,
                         /* isConstant */ false,
                         GlobalVariable::LinkageTypes::InternalLinkage,
                         UndefValue::get(ReductionType),
                         F.getName() + ".accumulator",
                         /* InsertBefore */ nullptr,
                         GlobalVariable::ThreadLocalMode::NotThreadLocal,
                         compiler::utils::AddressSpace::Local};

  const auto IsInclusive = F.getName().contains("inclusive");

  auto &Ctx = F.getContext();
  auto *const EntryBB = BasicBlock::Create(Ctx, "entry", &F);

  IRBuilder<> Builder{EntryBB};

  // We need two barriers to isolate the accumulator initialization.
  compiler::utils::setBarrierSchedule(*createLocalBarrierCall(Builder, BI),
                                      compiler::utils::BarrierSchedule::Once);

  // Initialize the accumulator.
  Builder.CreateStore(ReductionNeutralValue, Accumulator);

  // The scans are defined in Linear order, so we must create a Linear barrier.
  compiler::utils::setBarrierSchedule(*createLocalBarrierCall(Builder, BI),
                                      compiler::utils::BarrierSchedule::Linear);

  // Read the accumulator.
  auto *const CurrentVal =
      Builder.CreateLoad(ReductionType, Accumulator, "current.val");

  // Perform the subgroup scan operation and add it to the accumulator.
  auto *SubScan = createSubgroupScan(Builder, Operand, WGC.Recurrence,
                                     IsInclusive, WGC.IsLogical, BI);
  assert(SubScan && "Invalid subgroup scan");

  const bool NeedsIdentityFix =
      !IsInclusive &&
      (WGC.Recurrence == RecurKind::FAdd || WGC.Recurrence == RecurKind::FMin ||
       WGC.Recurrence == RecurKind::FMax);

  // For FMin/FMax, we need to fix up the identity element on the zeroth
  // subgroup ID, because it will be +/-INFINITY, but we need it to be NaN.
  // Likewise for FAdd, the zeroth element is defined to be 0.0, but the true
  // neutral value is -0.0.
  if (NeedsIdentityFix) {
    auto *const GetSubGroupLocalID = BI.getOrDeclareMuxBuiltin(
        compiler::utils::eMuxBuiltinGetSubGroupLocalId, M);
    auto *const SubGroupLocalID =
        Builder.CreateCall(GetSubGroupLocalID, {}, "subgroup.id");
    auto *const IsZero =
        Builder.CreateICmp(CmpInst::ICMP_EQ, SubGroupLocalID,
                           ConstantInt::get(SubGroupLocalID->getType(), 0));
    SubScan = Builder.CreateSelect(IsZero, ReductionNeutralValue, SubScan);
  }

  auto *const Result =
      createBinOp(Builder, CurrentVal, SubScan, WGC.Recurrence);

  // Update the accumulator with the last element of the subgroup scan
  auto *const LastElement = Builder.CreateNUWSub(
      createGetSubgroupSize(Builder, BI, "wgc_sg_size"), Builder.getInt32(1));
  auto *const LastValue = createSubgroupBroadcast(Builder, SubScan, LastElement,
                                                  BI, "wgc_sg_scan_tail");
  auto *SubReduce = LastValue;

  // If it's an exclusive scan, we have to add on the last element of the source
  // as well.
  if (!IsInclusive) {
    auto *const LastSrcValue = createSubgroupBroadcast(
        Builder, Operand, LastElement, BI, "wgc_sg_tail");
    SubReduce = createBinOp(Builder, LastValue, LastSrcValue, WGC.Recurrence);
  }
  auto *const NextVal =
      createBinOp(Builder, CurrentVal, SubReduce, WGC.Recurrence);
  Builder.CreateStore(NextVal, Accumulator);

  // A third barrier ensures that if there are two or more scans, they can't get
  // tangled up.
  createLocalBarrierCall(Builder, BI);

  if (NeedsIdentityFix) {
    auto *const Identity =
        compiler::utils::getIdentityVal(WGC.Recurrence, Result->getType());
    auto *const getLocalIDFn =
        BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetLocalId, M);
    auto *const IsZero = compiler::utils::isThreadZero(EntryBB, *getLocalIDFn);
    auto *const FixedResult = Builder.CreateSelect(IsZero, Identity, Result);
    Builder.CreateRet(FixedResult);
  } else {
    Builder.CreateRet(Result);
  }
}

/// @brief Defines the work-group collective functions.
///
/// @param[in] WGC Work-group collective function to be defined.
void emitWorkGroupCollectiveBody(Function &F,
                                 const compiler::utils::GroupCollective &WGC,
                                 compiler::utils::BuiltinInfo &BI) {
  switch (WGC.Op) {
    case compiler::utils::GroupCollective::OpKind::All:
    case compiler::utils::GroupCollective::OpKind::Any:
    case compiler::utils::GroupCollective::OpKind::Reduction:
      emitWorkGroupReductionBody(F, WGC, BI);
      break;
    case compiler::utils::GroupCollective::OpKind::Broadcast:
      emitWorkGroupBroadcastBody(F, WGC, BI);
      break;
    case compiler::utils::GroupCollective::OpKind::ScanExclusive:
    case compiler::utils::GroupCollective::OpKind::ScanInclusive:
      emitWorkGroupScanBody(F, WGC, BI);
      break;
    default:
      llvm_unreachable("unhandled work-group collective");
  }
}
}  // namespace

PreservedAnalyses compiler::utils::ReplaceWGCPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  // Only run this pass on OpenCL 2.0+ modules.
  auto Version = getOpenCLVersion(M);
  if (Version < OpenCLC20) {
    return PreservedAnalyses::all();
  }

  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);
  // This pass may insert new builtins into the module e.g. local barriers, so
  // we need to create a work-list before doing any work to avoid invalidating
  // iterators.
  SmallVector<std::pair<Function *, GroupCollective>, 8> WGCollectives{};
  for (auto &F : M) {
    auto Builtin = BI.analyzeBuiltin(F);
    if (auto WGC = BI.isMuxGroupCollective(Builtin.ID);
        WGC && WGC->isWorkGroupScope()) {
      WGCollectives.push_back({&F, *WGC});
    }
  }

  for (auto [F, WGC] : WGCollectives) {
    emitWorkGroupCollectiveBody(*F, WGC, BI);
  }
  return !WGCollectives.empty() ? PreservedAnalyses::none()
                                : PreservedAnalyses::all();
}
