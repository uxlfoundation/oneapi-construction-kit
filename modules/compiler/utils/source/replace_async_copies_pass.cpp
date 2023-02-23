// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Defines the OpenCL C async copy builtins in terms of __mux builtins.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/address_spaces.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/dma.h>
#include <compiler/utils/mangling.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/replace_async_copies_pass.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/opaque_pointers.h>

#include <memory>
#include <type_traits>

using namespace llvm;

namespace {

llvm::StringRef getMuxDMAFunctionName(bool IsRead, unsigned Dims) {
  static constexpr std::array<StringRef, 6> func_names{
      compiler::utils::MuxBuiltins::dma_write_1d,
      compiler::utils::MuxBuiltins::dma_write_2d,
      compiler::utils::MuxBuiltins::dma_write_3d,
      compiler::utils::MuxBuiltins::dma_read_1d,
      compiler::utils::MuxBuiltins::dma_read_2d,
      compiler::utils::MuxBuiltins::dma_read_3d,
  };
  assert(Dims <= 3 && Dims > 0 && "Max Dimension is of size 3");
  const unsigned index = IsRead ? 1 : 0;
  const unsigned offset = (3 * index) + (Dims - 1);
  return func_names[offset];
}

/// @brief Creates declarations for the __mux dma intrinsics.
///
/// Creates the __mux_dma_read_1D, __mux_dma_read_2D, __mux_dma_write_1D,
/// __mux_dma_write_2D, __mux_dma_read_3D, __mux_dma_write_3D declarations.
///
/// @param[in] Module to create the declarations in.
/// @param[in] IsRead Whether the declaration should be a read or a write.
/// @param[in] Dims The dimensionality of the desired builtin.
///
/// @return The __mux buitlin declaration.
Function *getOrCreateMuxDMA(Module *Module, bool IsRead, unsigned Dims = 1) {
  auto &Context = Module->getContext();
  // Assume to begin with we are dmaing __global -> __lobal
  auto DstPointerAS = compiler::utils::AddressSpace::Local;
  auto SrcPointerAS = compiler::utils::AddressSpace::Global;
  auto MuxDMAName = getMuxDMAFunctionName(IsRead, Dims);
  // Then swap if it's actually the other way around.
  if (!IsRead) {
    std::swap(DstPointerAS, SrcPointerAS);
  }

  FunctionType *MuxDMAType = nullptr;

  PointerType *DstPointerType = Type::getInt8PtrTy(Context, DstPointerAS);
  PointerType *SrcPointerType = Type::getInt8PtrTy(Context, SrcPointerAS);

  IntegerType *SizeType = compiler::utils::getSizeType(*Module);
  StructType *MuxEventType =
      compiler::utils::getOrCreateMuxDMAEventType(*Module);
  PointerType *MuxEventTypePtr = PointerType::getUnqual(MuxEventType);

  if (Dims == 1) {
    MuxDMAType = FunctionType::get(
        MuxEventTypePtr,
        {DstPointerType, SrcPointerType, SizeType /*width*/, MuxEventTypePtr},
        false);
  } else if (Dims == 2) {
    MuxDMAType = FunctionType::get(
        MuxEventTypePtr,
        {DstPointerType, SrcPointerType, SizeType /*line_size*/,
         SizeType /* src_stride*/, SizeType /*dst stride*/,
         SizeType /*num_lines*/, MuxEventTypePtr},
        false);
  } else if (Dims == 3) {
    MuxDMAType = FunctionType::get(
        MuxEventTypePtr,
        {DstPointerType, SrcPointerType, SizeType /*line_size*/,
         SizeType /*dst_line_stride*/, SizeType /*src_line stride*/,
         SizeType /*num_lines_per_plane*/, SizeType /*dst_plane_stride*/,
         SizeType /*src_plane_stride*/, SizeType /*num_planes*/,
         MuxEventTypePtr},
        false);
  } else {
    return nullptr;
  }
  auto *MuxDMA = dyn_cast<Function>(
      Module->getOrInsertFunction(MuxDMAName, MuxDMAType).getCallee());
  if (!MuxDMA) {
    return nullptr;
  }
  MuxDMA->setCallingConv(CallingConv::SPIR_FUNC);
  return MuxDMA;
}

/// @brief Define async copy builtins.
///
/// Defines async_work_group_copy and async_work_group_strided_copy in
/// terms of __mux builtins.
///
/// At a high level the mapping is:
/// * async_work_group_copy (global -> local) -> __mux_dma_read_1D
/// * async_work_group_copy (local -> global) -> __mux_dma_write_1D
/// * async_work_group_strided_copy (global -> local) ->
/// __mux_dma_read_2D
/// * async_work_group_strided_copy (local -> global) ->
/// __mux_dma_write_2D
///
/// @param[in,out] AsyncWorkGroupCopy The async work group copy function to be
/// defined.
/// @param[in] IsStrided Whether the async work group copy is the strided
/// variant.
void defineAsyncWorkGroupCopy(Function &AsyncWorkGroupCopy, Type *DataTy,
                              bool IsStrided) {
  // Unpack the arguments for ease of access.
  auto ArgIterator = AsyncWorkGroupCopy.arg_begin();
  auto *Dst = &*ArgIterator;
  auto *Src = &*(ArgIterator + 1);
  auto *NumElements = &*(ArgIterator + 2);
  auto *EventIn = IsStrided ? &*(ArgIterator + 4) : &*(ArgIterator + 3);

  // Find out which way the DMA is going and declare the appropriate mux
  // builtin.
  const bool IsRead = Dst->getType()->getPointerAddressSpace() ==
                      compiler::utils::AddressSpace::Local;
  auto *Module = AsyncWorkGroupCopy.getParent();
  auto *MuxDMA = getOrCreateMuxDMA(Module, IsRead, IsStrided ? 2 : 1);

  auto &Context = AsyncWorkGroupCopy.getContext();
  auto *BB = BasicBlock::Create(Context, "bb", &AsyncWorkGroupCopy);

  // Get the size in bytes of the elements being copied.
  const DataLayout &DataLayout =
      AsyncWorkGroupCopy.getParent()->getDataLayout();
  auto ElementTypeWidthInBytes =
      DataLayout.getTypeAllocSize(DataTy).getFixedSize();
  auto *ElementSize =
      ConstantInt::get(NumElements->getType(), ElementTypeWidthInBytes);

  IRBuilder<> BBBuilder(BB);
  // Scale up the number of elements by the size of the type since the __mux
  // builtins take a byte width rather than a count (they are type agnostic).
  // For a strided copy we are doing a scatter or gather, so we don't scale.
  auto *WidthInBytes =
      IsStrided ? ElementSize
                : BBBuilder.CreateMul(ElementSize, NumElements, "width.bytes");

  // Cast the OpenCL C event_t* into a __mux_dma_event_t*.
  auto *MuxEventPtrType = PointerType::getUnqual(
      compiler::utils::getOrCreateMuxDMAEventType(*Module));
  auto *MuxEventPtr =
      BBBuilder.CreateBitCast(EventIn, MuxEventPtrType, "mux.in.event");

  auto *MuxDstType = PointerType::get(IntegerType::getInt8Ty(Context),
                                      Dst->getType()->getPointerAddressSpace());
  auto *MuxSrcType = PointerType::get(IntegerType::getInt8Ty(Context),
                                      Src->getType()->getPointerAddressSpace());
  // Cast the src and destination into i8* so they can be passed to the type
  // agnostic mux builtin.
  auto *MuxDst = BBBuilder.CreateBitCast(Dst, MuxDstType, "mux.dst");
  auto *MuxSrc = BBBuilder.CreateBitCast(Src, MuxSrcType, "mux.src");

  CallInst *ResultEvent = nullptr;
  if (IsStrided) {
    // The stride from async_work_group_strided_copy is in elements, but the
    // stride in the __mux builtins are in bytes so we need to scale the value.
    auto *Stride = &*(AsyncWorkGroupCopy.arg_begin() + 3);
    auto *StrideInBytes =
        BBBuilder.CreateMul(ElementSize, Stride, "stride.bytes");

    // For async_work_group_strided_copy, the stride only applies to the global
    // memory, as we are doing scatters/gathers.
    auto *DstStride = IsRead ? ElementSize : StrideInBytes;
    auto *SrcStride = IsRead ? StrideInBytes : ElementSize;

    ResultEvent = BBBuilder.CreateCall(MuxDMA,
                                       {MuxDst, MuxSrc, WidthInBytes, DstStride,
                                        SrcStride, NumElements, MuxEventPtr},
                                       "mux.out.event");
  } else {
    ResultEvent = BBBuilder.CreateCall(
        MuxDMA, {MuxDst, MuxSrc, WidthInBytes, MuxEventPtr}, "mux.out.event");
  }

  ResultEvent->setCallingConv(MuxDMA->getCallingConv());
  auto CLReturnEvent =
      BBBuilder.CreateBitCast(ResultEvent, EventIn->getType(), "clc.out.event");
  BBBuilder.CreateRet(CLReturnEvent);
}

void defineAsyncWorkGroupCopy2D(Function &AsyncWorkGroupCopy) {
  // Unpack the arguments for ease of access.
  auto ArgIterator = AsyncWorkGroupCopy.arg_begin();
  auto *Dst = &*ArgIterator;
  auto *DstOffset = &*(ArgIterator + 1);
  auto *Src = &*(ArgIterator + 2);
  auto *SrcOffset = &*(ArgIterator + 3);
  auto *NumBytesPerEl = &*(ArgIterator + 4);
  auto *NumElsPerLine = &*(ArgIterator + 5);
  auto *NumLines = &*(ArgIterator + 6);
  auto *SrcTotalLineLength = &*(ArgIterator + 7);
  auto *DstTotalLineLength = &*(ArgIterator + 8);
  auto *EventIn = &*(ArgIterator + 9);

  // Find out which way the DMA is going and declare the appropriate mux
  // builtin.
  const bool IsRead = Dst->getType()->getPointerAddressSpace() ==
                      compiler::utils::AddressSpace::Local;
  auto *Module = AsyncWorkGroupCopy.getParent();
  auto *MuxDMA = getOrCreateMuxDMA(Module, IsRead, 2);

  auto &Context = AsyncWorkGroupCopy.getContext();
  auto *BB = BasicBlock::Create(Context, "entry", &AsyncWorkGroupCopy);

  IRBuilder<> IR(BB);

  // Cast the OpenCL C event_t* into a __mux_dma_event_t*.
  auto *MuxEventPtrType = PointerType::getUnqual(
      compiler::utils::getOrCreateMuxDMAEventType(*Module));
  auto *MuxEventPtr =
      IR.CreateBitCast(EventIn, MuxEventPtrType, "mux.in.event");

  auto *DstOffsetBytes = IR.CreateMul(DstOffset, NumBytesPerEl);
  auto *SrcOffsetBytes = IR.CreateMul(SrcOffset, NumBytesPerEl);
  auto *LineSizeBytes = IR.CreateMul(NumElsPerLine, NumBytesPerEl);
  auto *ByteTy = IR.getInt8Ty();
  auto *DstWithOffset = IR.CreateGEP(ByteTy, Dst, DstOffsetBytes);
  auto *SrcWithOffset = IR.CreateGEP(ByteTy, Src, SrcOffsetBytes);
  auto *SrcStrideBytes = IR.CreateMul(SrcTotalLineLength, NumBytesPerEl);
  auto *DstStrideBytes = IR.CreateMul(DstTotalLineLength, NumBytesPerEl);
  auto *MuxDMACall = IR.CreateCall(
      MuxDMA, {DstWithOffset, SrcWithOffset, LineSizeBytes, DstStrideBytes,
               SrcStrideBytes, NumLines, MuxEventPtr});
  MuxDMACall->setCallingConv(MuxDMA->getCallingConv());
  auto *CLReturnEvent =
      IR.CreateBitCast(MuxDMACall, EventIn->getType(), "clc.out.event");
  IR.CreateRet(CLReturnEvent);
}

void defineAsyncWorkGroupCopy3D(Function &AsyncWorkGroupCopy) {
  // Unpack the arguments for ease of access.
  auto ArgIterator = AsyncWorkGroupCopy.arg_begin();
  auto *Dst = &*ArgIterator;
  auto *DstOffset = &*(ArgIterator + 1);
  auto *Src = &*(ArgIterator + 2);
  auto *SrcOffset = &*(ArgIterator + 3);
  auto *NumBytesPerEl = &*(ArgIterator + 4);
  auto *NumElsPerLine = &*(ArgIterator + 5);
  auto *NumLines = &*(ArgIterator + 6);
  auto *NumPlanes = &*(ArgIterator + 7);
  auto *SrcTotalLineLength = &*(ArgIterator + 8);
  auto *SrcTotalPlaneArea = &*(ArgIterator + 9);
  auto *DstTotalLineLength = &*(ArgIterator + 10);
  auto *DstTotalPlaneArea = &*(ArgIterator + 11);
  auto *EventIn = &*(ArgIterator + 12);

  // Find out which way the DMA is going and declare the appropriate mux
  // builtin.
  const bool IsRead = Dst->getType()->getPointerAddressSpace() ==
                      compiler::utils::AddressSpace::Local;
  auto *Module = AsyncWorkGroupCopy.getParent();
  auto *MuxDMA = getOrCreateMuxDMA(Module, IsRead, 3);

  auto &Context = AsyncWorkGroupCopy.getContext();
  auto *BB = BasicBlock::Create(Context, "entry", &AsyncWorkGroupCopy);

  IRBuilder<> IR(BB);

  // Cast the OpenCL C event_t* into a __mux_dma_event_t*.
  auto *MuxEventPtrType = PointerType::getUnqual(
      compiler::utils::getOrCreateMuxDMAEventType(*Module));
  auto *MuxEventPtr =
      IR.CreateBitCast(EventIn, MuxEventPtrType, "mux.in.event");

  auto *DstOffsetBytes = IR.CreateMul(DstOffset, NumBytesPerEl);
  auto *SrcOffsetBytes = IR.CreateMul(SrcOffset, NumBytesPerEl);
  auto *LineSizeBytes = IR.CreateMul(NumElsPerLine, NumBytesPerEl);
  auto *ByteTy = IR.getInt8Ty();
  auto *DstWithOffset = IR.CreateGEP(ByteTy, Dst, DstOffsetBytes);
  auto *SrcWithOffset = IR.CreateGEP(ByteTy, Src, SrcOffsetBytes);
  auto *SrcLineStrideBytes = IR.CreateMul(SrcTotalLineLength, NumBytesPerEl);
  auto *DstLineStrideBytes = IR.CreateMul(DstTotalLineLength, NumBytesPerEl);
  auto *SrcPlaneStrideBytes = IR.CreateMul(SrcTotalPlaneArea, NumBytesPerEl);
  auto *DstPlaneStrideBytes = IR.CreateMul(DstTotalPlaneArea, NumBytesPerEl);
  auto *MuxDMACall = IR.CreateCall(
      MuxDMA, {DstWithOffset, SrcWithOffset, LineSizeBytes, DstLineStrideBytes,
               SrcLineStrideBytes, NumLines, DstPlaneStrideBytes,
               SrcPlaneStrideBytes, NumPlanes, MuxEventPtr});
  MuxDMACall->setCallingConv(MuxDMA->getCallingConv());
  auto *CLReturnEvent =
      IR.CreateBitCast(MuxDMACall, EventIn->getType(), "clc.out.event");
  IR.CreateRet(CLReturnEvent);
}

/// @brief Get or create the __mux_dma_wait builtin.
///
/// This may have been declared previously by another compiler pass, hence we
/// "get or create".
///
/// @param[in] Module LLVM Module to get or create the declaration in.
///
/// @return The __mux_dma_wait builtin declaration.
Function *getOrCreateMuxWait(Module *Module) {
  auto &Context = Module->getContext();
  auto *CountType = Type::getInt32Ty(Context);
  auto *MuxEventTypePtrPtr = PointerType::getUnqual(PointerType::getUnqual(
      compiler::utils::getOrCreateMuxDMAEventType(*Module)));
  auto *MuxWaitType = FunctionType::get(llvm::Type::getVoidTy(Context),
                                        {CountType, MuxEventTypePtrPtr}, false);
  auto *MuxWait = dyn_cast<Function>(
      Module
          ->getOrInsertFunction(compiler::utils::MuxBuiltins::dma_wait,
                                MuxWaitType)
          .getCallee());
  if (!MuxWait) {
    return nullptr;
  }
  MuxWait->arg_begin()->setName("num.events");
  (MuxWait->arg_begin() + 1)->setName("events");
  MuxWait->setCallingConv(CallingConv::SPIR_FUNC);
  return MuxWait;
}

/// @brief Defines the wait_group_events builtin.
///
/// @param[in,out] WaitGroupEvents Declaration of the clc wait_group_events
/// builtin to define a body for.
void defineWaitGroupEvents(Function &WaitGroupEvents) {
  const auto Module = WaitGroupEvents.getParent();
  auto *MuxWait = getOrCreateMuxWait(Module);

  auto &Context = WaitGroupEvents.getContext();
  auto *EntryBB = BasicBlock::Create(Context, "Entry", &WaitGroupEvents);

  auto *Count = &*WaitGroupEvents.arg_begin();
  auto *Events = &*(WaitGroupEvents.arg_begin() + 1);

  auto *MuxEventTypePtrPtr = PointerType::getUnqual(PointerType::getUnqual(
      compiler::utils::getOrCreateMuxDMAEventType(*Module)));

  IRBuilder<> EntryBBBuilder(EntryBB);
  // Cast the OpenCL C event_t* into a __mux_dma_event_t*.
  auto *MuxEvents =
      EntryBBBuilder.CreateBitCast(Events, MuxEventTypePtrPtr, "mux.events");
  EntryBBBuilder.CreateCall(MuxWait, {Count, MuxEvents})
      ->setCallingConv(MuxWait->getCallingConv());
  EntryBBBuilder.CreateRetVoid();
}

/// @brief Define async copy builtin.
///
/// Checks whether the given function is a CLC async builtin and then defines
/// it in terms of __mux builtins.
///
/// @param[in,out] Function IR function which is to be defined.
///
/// @return Boolean indicating whether Function was async builtin and was
/// defined.
/// @retval True if @p Function is async builtin and is now defined.
/// @retval False if @p Function was not async builtin.
bool runOnFunction(Function &Function) {
  compiler::utils::NameMangler Mangler(&Function.getContext(),
                                       Function.getParent());
  // Parse the name part.
  StringRef DemangledName = Mangler.demangleName(Function.getName());

  if (DemangledName == "async_work_group_copy" ||
      DemangledName == "async_work_group_strided_copy") {
    // Now do a full demangle to determing the pointer element type of the
    // first argument.
    SmallVector<Type *, 4> BuiltinArgTypes, BuiltinArgPointeeTypes;
    SmallVector<compiler::utils::TypeQualifiers, 4> BuiltinArgQuals;
    StringRef BuiltinName =
        Mangler.demangleName(Function.getName(), BuiltinArgTypes,
                             BuiltinArgPointeeTypes, BuiltinArgQuals);
    // Double-check we've demangled something sensible.
    (void)BuiltinName;
    assert(
        !BuiltinName.empty() && BuiltinArgTypes[0]->isPointerTy() &&
        multi_llvm::isOpaqueOrPointeeTypeMatches(
            cast<PointerType>(BuiltinArgTypes[0]), BuiltinArgPointeeTypes[0]));
    bool IsStrided = DemangledName == "async_work_group_strided_copy";
    defineAsyncWorkGroupCopy(Function, BuiltinArgPointeeTypes[0], IsStrided);
  } else if (DemangledName == "async_work_group_copy_2D2D") {
    defineAsyncWorkGroupCopy2D(Function);
  } else if (DemangledName == "async_work_group_copy_3D3D") {
    defineAsyncWorkGroupCopy3D(Function);
  } else if (DemangledName == "wait_group_events") {
    defineWaitGroupEvents(Function);
  } else {
    return false;
  }

  return true;
}

}  // namespace

PreservedAnalyses compiler::utils::ReplaceAsyncCopiesPass::run(
    Module &M, ModuleAnalysisManager &) {
  // Loop over all functions since there are overloads of each builtin.
  auto Result = false;
  for (auto &F : M) {
    Result |= runOnFunction(F);
  }
  return Result ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
