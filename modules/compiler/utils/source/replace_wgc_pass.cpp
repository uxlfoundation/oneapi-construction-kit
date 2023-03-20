// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Defines the work-group collective builtins.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
#include <multi_llvm/creation_apis_helper.h>
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
  auto *const BarrierCall = multi_llvm::createCallInst(
      Barrier, Barrier->getFunctionType(), {ID, Scope, Semantics}, "",
      Builder.GetInsertBlock());
  return BarrierCall;
}

/// @brief Helper function to create subgroup reduction calls.
Value *createSubgroupReduction(IRBuilder<> &Builder, llvm::Value *Src,
                               const compiler::utils::GroupCollective &WGC) {
  StringRef name;
  compiler::utils::TypeQualifier Q = compiler::utils::eTypeQualNone;
  switch (WGC.recurKind) {
    default:
      return nullptr;
    case multi_llvm::RecurKind::And:
      if (WGC.isAnyAll()) {
        name = "sub_group_all";
        Q = compiler::utils::eTypeQualSignedInt;
      } else {
        name = !WGC.isLogical ? "sub_group_reduce_and"
                              : "sub_group_reduce_logical_and";
      }
      break;
    case multi_llvm::RecurKind::Or:
      if (WGC.isAnyAll()) {
        name = "sub_group_any";
        Q = compiler::utils::eTypeQualSignedInt;
      } else {
        name = !WGC.isLogical ? "sub_group_reduce_or"
                              : "sub_group_reduce_logical_or";
      }
      break;
    case multi_llvm::RecurKind::FAdd:
    case multi_llvm::RecurKind::Add:
      name = "sub_group_reduce_add";
      break;
    case multi_llvm::RecurKind::FMin:
    case multi_llvm::RecurKind::UMin:
      name = "sub_group_reduce_min";
      break;
    case multi_llvm::RecurKind::SMin:
      name = "sub_group_reduce_min";
      Q = compiler::utils::eTypeQualSignedInt;
      break;
    case multi_llvm::RecurKind::FMax:
    case multi_llvm::RecurKind::UMax:
      name = "sub_group_reduce_max";
      break;
    case multi_llvm::RecurKind::SMax:
      name = "sub_group_reduce_max";
      Q = compiler::utils::eTypeQualSignedInt;
      break;
      // SPV_KHR_uniform_group_instructions
    case multi_llvm::RecurKind::Mul:
    case multi_llvm::RecurKind::FMul:
      name = "sub_group_reduce_mul";
      break;
    case multi_llvm::RecurKind::Xor:
      name = !WGC.isLogical ? "sub_group_reduce_xor"
                            : "sub_group_reduce_logical_xor";
      break;
  }

  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();

  // Mangle the function name and look it up in the module.
  compiler::utils::NameMangler Mangler(&M->getContext());
  std::string MangledName = Mangler.mangleName(name, {Ty}, {Q});
  Function *Builtin = M->getFunction(MangledName);

  // Declare the builtin if necessary.
  if (!Builtin) {
    FunctionType *FT = FunctionType::get(Ty, {Ty}, false);
    M->getOrInsertFunction(MangledName, FT);
    Builtin = M->getFunction(MangledName);
    Builtin->setCallingConv(CallingConv::SPIR_FUNC);
  }

  return Builder.CreateCall(Builtin, {Src}, "wgc");
}

Value *createSubgroupScan(IRBuilder<> &Builder, llvm::Value *Src,
                          multi_llvm::RecurKind Kind, bool IsInclusive,
                          bool IsLogical) {
  StringRef name;
  compiler::utils::TypeQualifier Q = compiler::utils::eTypeQualNone;
  switch (Kind) {
    default:
      return nullptr;
    case multi_llvm::RecurKind::FAdd:
    case multi_llvm::RecurKind::Add:
      name = IsInclusive ? StringRef("sub_group_scan_inclusive_add")
                         : StringRef("sub_group_scan_exclusive_add");
      break;
    case multi_llvm::RecurKind::SMin:
      Q = compiler::utils::eTypeQualSignedInt;
      LLVM_FALLTHROUGH;
    case multi_llvm::RecurKind::UMin:
    case multi_llvm::RecurKind::FMin:
      name = IsInclusive ? StringRef("sub_group_scan_inclusive_min")
                         : StringRef("sub_group_scan_exclusive_min");
      break;
    case multi_llvm::RecurKind::SMax:
      Q = compiler::utils::eTypeQualSignedInt;
      LLVM_FALLTHROUGH;
    case multi_llvm::RecurKind::UMax:
    case multi_llvm::RecurKind::FMax:
      name = IsInclusive ? StringRef("sub_group_scan_inclusive_max")
                         : StringRef("sub_group_scan_exclusive_max");
      break;
    case multi_llvm::RecurKind::Mul:
    case multi_llvm::RecurKind::FMul:
      name = IsInclusive ? StringRef("sub_group_scan_inclusive_mul")
                         : StringRef("sub_group_scan_exclusive_mul");
      break;
    case multi_llvm::RecurKind::And:
      if (!IsLogical) {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_and")
                           : StringRef("sub_group_scan_exclusive_and");
      } else {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_logical_and")
                           : StringRef("sub_group_scan_exclusive_logical_and");
      }
      break;
    case multi_llvm::RecurKind::Or:
      if (!IsLogical) {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_or")
                           : StringRef("sub_group_scan_exclusive_or");
      } else {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_logical_or")
                           : StringRef("sub_group_scan_exclusive_logical_or");
      }
      break;
    case multi_llvm::RecurKind::Xor:
      if (!IsLogical) {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_xor")
                           : StringRef("sub_group_scan_exclusive_xor");
      } else {
        name = IsInclusive ? StringRef("sub_group_scan_inclusive_logical_xor")
                           : StringRef("sub_group_scan_exclusive_logical_xor");
      }
      break;
  }

  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();

  // Mangle the function name and look it up in the module.
  compiler::utils::NameMangler Mangler(&M->getContext());
  std::string MangledName = Mangler.mangleName(name, {Ty}, {Q});
  Function *Builtin = M->getFunction(MangledName);

  // Declare the builtin if necessary.
  if (!Builtin) {
    FunctionType *FT = FunctionType::get(Ty, {Ty}, false);
    M->getOrInsertFunction(MangledName, FT);
    Builtin = M->getFunction(MangledName);
    Builtin->setCallingConv(CallingConv::SPIR_FUNC);
  }

  return Builder.CreateCall(Builtin, {Src}, "wgc_scan");
}

/// @brief Helper function to create get subgroup size calls.
Value *createGetSubgroupSize(IRBuilder<> &Builder,
                             const Twine &Name = Twine()) {
  auto &M = *Builder.GetInsertBlock()->getModule();
  auto *const Builtin = cast<Function>(
      M.getOrInsertFunction(
           "_Z18get_sub_group_sizev",
           FunctionType::get(Builder.getInt32Ty(), /*isVarArg*/ false))
          .getCallee());
  assert(Builtin && "get_sub_group_size is not in module");

  Builtin->setCallingConv(CallingConv::SPIR_FUNC);
  return Builder.CreateCall(Builtin, {}, Name);
}

/// @brief Helper function to create subgroup broadcast calls.
Value *createSubgroupBroadcast(IRBuilder<> &Builder, Value *Src, Value *ID,
                               const Twine &Name = Twine()) {
  StringRef BuiltinName = "sub_group_broadcast";
  compiler::utils::TypeQualifier Q = compiler::utils::eTypeQualNone;

  auto *const Ty = Src->getType();
  auto *const M = Builder.GetInsertBlock()->getModule();
  auto &Ctx = M->getContext();

  auto *const Int32Ty = Type::getInt32Ty(Ctx);

  // Mangle the function name and look it up in the module.
  compiler::utils::NameMangler Mangler(&Ctx);
  std::string MangledName = Mangler.mangleName(
      BuiltinName, {Ty, Int32Ty}, {Q, compiler::utils::eTypeQualNone});
  Function *Builtin = M->getFunction(MangledName);

  // Declare the builtin if necessary.
  if (!Builtin) {
    FunctionType *FT = FunctionType::get(Ty, {Ty, Int32Ty}, false);
    M->getOrInsertFunction(MangledName, FT);
    Builtin = M->getFunction(MangledName);
    Builtin->setCallingConv(CallingConv::SPIR_FUNC);
  }

  return Builder.CreateCall(Builtin, {Src, ID}, Name);
}

/// @brief Helper function to get-or-create get_sub_group_local_id.
///
/// @param[in] M Module to create the function in.
///
/// @return The builtin.
Function *getOrCreateGetSubGroupLocalID(Module &M) {
  auto *const Int32Ty = Type::getInt32Ty(M.getContext());
  auto *const GetSubGroupLocalIDTy =
      FunctionType::get(Int32Ty, /*isVarArg*/ false);

  auto *const GetSubGroupLocalID = cast<Function>(
      M.getOrInsertFunction("_Z22get_sub_group_local_idv", GetSubGroupLocalIDTy)
          .getCallee());

  assert(GetSubGroupLocalID && "get_sub_group_local_id is not in module");
  GetSubGroupLocalID->setCallingConv(CallingConv::SPIR_FUNC);
  return GetSubGroupLocalID;
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
                   llvm::Value *Operand, multi_llvm::RecurKind Kind,
                   bool IsAnyAll) {
  /// The semantics of bitwise "and" don't quite match the semantics of "all"
  /// (bitwise and isn't equivalent to logical and in a boolean context e.g.
  /// 01 & 10 = 00 but both 1 (01) and 2 (10) would be considered "true"), so
  /// for the sub_group_all reduction we need to work around this by emitting a
  /// few extra instructions.
  if (IsAnyAll && Kind == multi_llvm::RecurKind::And) {
    auto *const IntType = Operand->getType();
    Value *Cmp = Builder.CreateICmpNE(Operand, ConstantInt::get(IntType, 0));
    Cmp = Builder.CreateIntCast(Cmp, IntType, /* isSigned */ true);
    return Builder.CreateAnd(Cmp, CurrentVal);
  }
  // Otherwise we can just use a single binary op.
  return multi_llvm::createBinOpForRecurKind(Builder, CurrentVal, Operand,
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
void emitWorkGroupReductionBody(const compiler::utils::GroupCollective &WGC,
                                compiler::utils::BuiltinInfo &BI) {
  // Create a global variable to do the reduction on.
  auto &F = *WGC.func;
  auto *const Operand = F.getArg(0);
  auto *const ReductionType{Operand->getType()};
  auto *const ReductionNeutralValue{
      compiler::utils::getNeutralVal(WGC.recurKind, WGC.type)};
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
  auto *const SubReduce = createSubgroupReduction(Builder, Operand, WGC);
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
  auto *const NextVal = createBinOp(Builder, CurrentVal, SubReduce,
                                    WGC.recurKind, WGC.isAnyAll());
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
void emitWorkGroupBroadcastBody(const compiler::utils::GroupCollective &WGC,
                                compiler::utils::BuiltinInfo &BI) {
  // First arg is always the value to broadcast.
  auto &F = *WGC.func;
  auto *const ValueToBroadcast = F.getArg(0);

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
  auto *const SizeType = compiler::utils::getSizeType(M);
  auto *const Int32Type = llvm::Type::getInt32Ty(Ctx);
  auto *GetLocalIDType = llvm::FunctionType::get(SizeType, Int32Type, false);

  auto *const GetLocalID = llvm::dyn_cast<llvm::Function>(
      M.getOrInsertFunction("_Z12get_local_idj", GetLocalIDType).getCallee());
  assert(GetLocalID && "get_local_id is not in module");
  GetLocalID->setCallingConv(llvm::CallingConv::SPIR_FUNC);

  Value *IsBroadcastingThread = ConstantInt::getTrue(Ctx);
  for (unsigned i = 1; i < F.arg_size(); ++i) {
    Value *LocalIDCall = Builder.CreateCall(
        GetLocalID, ConstantInt::get(GetLocalID->getArg(0)->getType(), i - 1));
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
void emitWorkGroupScanBody(const compiler::utils::GroupCollective &WGC,
                           compiler::utils::BuiltinInfo &BI) {
  // Create a global variable to do the scan on.
  auto &F = *WGC.func;
  auto *const Operand = F.getArg(0);
  auto *const ReductionType{Operand->getType()};
  auto *const ReductionNeutralValue{
      compiler::utils::getNeutralVal(WGC.recurKind, WGC.type)};
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
  auto *SubScan = createSubgroupScan(Builder, Operand, WGC.recurKind,
                                     IsInclusive, WGC.isLogical);
  assert(SubScan && "Invalid subgroup scan");

  bool const NeedsIdentityFix =
      !IsInclusive && (WGC.recurKind == multi_llvm::RecurKind::FAdd ||
                       WGC.recurKind == multi_llvm::RecurKind::FMin ||
                       WGC.recurKind == multi_llvm::RecurKind::FMax);

  // For FMin/FMax, we need to fix up the identity element on the zeroth
  // subgroup ID, because it will be +/-INFINITY, but we need it to be NaN.
  // Likewise for FAdd, the zeroth element is defined to be 0.0, but the true
  // neutral value is -0.0.
  if (NeedsIdentityFix) {
    auto *const GetSubGroupLocalID = getOrCreateGetSubGroupLocalID(M);
    auto *const SubGroupLocalID =
        Builder.CreateCall(GetSubGroupLocalID, {}, "subgroup.id");
    auto *const IsZero =
        Builder.CreateICmp(CmpInst::ICMP_EQ, SubGroupLocalID,
                           ConstantInt::get(SubGroupLocalID->getType(), 0));
    SubScan = Builder.CreateSelect(IsZero, ReductionNeutralValue, SubScan);
  }

  auto *const Result =
      createBinOp(Builder, CurrentVal, SubScan, WGC.recurKind, WGC.isAnyAll());

  // Update the accumulator with the last element of the subgroup scan
  auto *const LastElement = Builder.CreateNUWSub(
      createGetSubgroupSize(Builder, "wgc_sg_size"), Builder.getInt32(1));
  auto *const LastValue = createSubgroupBroadcast(Builder, SubScan, LastElement,
                                                  "wgc_sg_scan_tail");
  auto *SubReduce = LastValue;

  // If it's an exclusive scan, we have to add on the last element of the source
  // as well.
  if (!IsInclusive) {
    auto *const LastSrcValue =
        createSubgroupBroadcast(Builder, Operand, LastElement, "wgc_sg_tail");
    SubReduce = createBinOp(Builder, LastValue, LastSrcValue, WGC.recurKind,
                            WGC.isAnyAll());
  }
  auto *const NextVal = createBinOp(Builder, CurrentVal, SubReduce,
                                    WGC.recurKind, WGC.isAnyAll());
  Builder.CreateStore(NextVal, Accumulator);

  // A third barrier ensures that if there are two or more scans, they can't get
  // tangled up.
  createLocalBarrierCall(Builder, BI);

  if (NeedsIdentityFix) {
    auto *const Identity =
        compiler::utils::getIdentityVal(WGC.recurKind, WGC.type);
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
void emitWorkGroupCollectiveBody(const compiler::utils::GroupCollective &WGC,
                                 compiler::utils::BuiltinInfo &BI) {
  switch (WGC.op) {
    case compiler::utils::GroupCollective::Op::All:
    case compiler::utils::GroupCollective::Op::Any:
    case compiler::utils::GroupCollective::Op::Reduction:
      emitWorkGroupReductionBody(WGC, BI);
      break;
    case compiler::utils::GroupCollective::Op::Broadcast:
      emitWorkGroupBroadcastBody(WGC, BI);
      break;
    case compiler::utils::GroupCollective::Op::ScanExclusive:
    case compiler::utils::GroupCollective::Op::ScanInclusive:
      emitWorkGroupScanBody(WGC, BI);
      break;
    default:
      llvm_unreachable("unhandled work-group collective");
  }
}
}  // namespace

PreservedAnalyses compiler::utils::ReplaceWGCPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);
  // This pass may insert new builtins into the module e.g. local barriers, so
  // we need to create a work-list before doing any work to avoid invalidating
  // iterators.
  SmallVector<GroupCollective, 8> WGCollectives{};
  for (auto &F : M) {
    auto WGC = isGroupCollective(&F);
    if (WGC && WGC->scope == GroupCollective::Scope::WorkGroup) {
      WGCollectives.push_back(*WGC);
    }
  }

  for (auto const WGC : WGCollectives) {
    emitWorkGroupCollectiveBody(WGC, BI);
  }
  return !WGCollectives.empty() ? PreservedAnalyses::none()
                                : PreservedAnalyses::all();
}
