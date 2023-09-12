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

#include <cargo/endian.h>
#include <llvm/ADT/StringSwitch.h>
#include <spirv-ll/module.h>
#include <spirv-ll/opcodes.h>

#include <unordered_map>

namespace {
spv::Op getOpCode(const uint32_t *data, bool endianSwap) {
  return static_cast<spv::Op>(endianSwap
                                  ? cargo::byte_swap(*data) & spv::OpCodeMask
                                  : *data & spv::OpCodeMask);
}
}  // namespace

namespace spirv_ll {
OpCode::OpCode(const iterator &iter)
    : code(getOpCode(iter.word, iter.endianSwap)),
      data(iter.word),
      endianSwap(iter.endianSwap) {}

OpCode::OpCode(const OpCode &other, spv::Op code)
    : code(code), data(other.data), endianSwap(other.endianSwap) {}

uint16_t OpCode::wordCount() const {
  return endianSwap ? cargo::byte_swap(*data) >> spv::WordCountShift
                    : *data >> spv::WordCountShift;
}

uint16_t OpCode::opCode() const {
  // TODO: Is this even required now the constructor takes care of this?
  return endianSwap ? cargo::byte_swap(*data) & spv::OpCodeMask
                    : *data & spv::OpCodeMask;
}

uint32_t OpCode::getValueAtOffset(int offset) const {
  return endianSwap ? cargo::byte_swap(*(data + offset)) : *(data + offset);
}

uint64_t OpCode::getValueAtOffset(int offset, int words) const {
  uint64_t value = 0;
  switch (words) {
    case 1:
      value =
          endianSwap ? cargo::byte_swap(*(data + offset)) : *(data + offset);
      break;
    case 2:
      // We can't guarantee that data+offset is uint64_t aligned, so memcpy.
      uint64_t load;
      memcpy(&load, data + offset, sizeof(uint64_t));
      value = endianSwap ? cargo::byte_swap(load) : load;
      break;
    default:
      SPIRV_LL_ABORT("Unsupported value width in getValueAtOffset!");
  }
  return value;
}

bool OpCode::isType() const {
  switch (code) {
    case spv::OpTypeVoid:
    case spv::OpTypeBool:
    case spv::OpTypeInt:
    case spv::OpTypeFloat:
    case spv::OpTypeVector:
    case spv::OpTypeMatrix:
    case spv::OpTypeImage:
    case spv::OpTypeSampler:
    case spv::OpTypeSampledImage:
    case spv::OpTypeArray:
    case spv::OpTypeRuntimeArray:
    case spv::OpTypeStruct:
    case spv::OpTypeOpaque:
    case spv::OpTypePointer:
    case spv::OpTypeFunction:
    case spv::OpTypeEvent:
    case spv::OpTypeDeviceEvent:
    case spv::OpTypeReserveId:
    case spv::OpTypeQueue:
    case spv::OpTypePipe:
    case spv::OpTypeForwardPointer:
      return true;
    default:
      return false;
  }
}

bool OpCode::hasResult() const {
  switch (code) {
    case spv::OpAccessChain:
    case spv::OpAll:
    case spv::OpAny:
    case spv::OpArrayLength:
    case spv::OpAtomicAnd:
    case spv::OpAtomicCompareExchange:
    case spv::OpAtomicCompareExchangeWeak:
    case spv::OpAtomicExchange:
    case spv::OpAtomicFlagTestAndSet:
    case spv::OpAtomicIAdd:
    case spv::OpAtomicIDecrement:
    case spv::OpAtomicIIncrement:
    case spv::OpAtomicISub:
    case spv::OpAtomicLoad:
    case spv::OpAtomicOr:
    case spv::OpAtomicSMax:
    case spv::OpAtomicSMin:
    case spv::OpAtomicUMax:
    case spv::OpAtomicUMin:
    case spv::OpAtomicFAddEXT:
    case spv::OpAtomicFMaxEXT:
    case spv::OpAtomicFMinEXT:
    case spv::OpAtomicXor:
    case spv::OpBitCount:
    case spv::OpBitFieldInsert:
    case spv::OpBitFieldSExtract:
    case spv::OpBitFieldUExtract:
    case spv::OpBitReverse:
    case spv::OpBitcast:
    case spv::OpBitwiseAnd:
    case spv::OpBitwiseOr:
    case spv::OpBitwiseXor:
    case spv::OpBuildNDRange:
    case spv::OpGetKernelLocalSizeForSubgroupCount:
    case spv::OpGetKernelMaxNumSubgroups:
    case spv::OpCompositeConstruct:
    case spv::OpCompositeExtract:
    case spv::OpCompositeInsert:
    case spv::OpConstant:
    case spv::OpConstantComposite:
    case spv::OpConstantFalse:
    case spv::OpConstantNull:
    case spv::OpConstantSampler:
    case spv::OpConstantTrue:
    case spv::OpConvertFToS:
    case spv::OpConvertFToU:
    case spv::OpConvertPtrToU:
    case spv::OpConvertSToF:
    case spv::OpConvertUToF:
    case spv::OpConvertUToPtr:
    case spv::OpCopyObject:
    case spv::OpCreateUserEvent:
    case spv::OpDPdx:
    case spv::OpDPdxCoarse:
    case spv::OpDPdxFine:
    case spv::OpDPdy:
    case spv::OpDPdyCoarse:
    case spv::OpDPdyFine:
    case spv::OpDot:
    case spv::OpEnqueueKernel:
    case spv::OpEnqueueMarker:
    case spv::OpExtInst:
    case spv::OpFAdd:
    case spv::OpFConvert:
    case spv::OpFDiv:
    case spv::OpFMod:
    case spv::OpFMul:
    case spv::OpFNegate:
    case spv::OpFOrdEqual:
    case spv::OpFOrdGreaterThan:
    case spv::OpFOrdGreaterThanEqual:
    case spv::OpFOrdLessThan:
    case spv::OpFOrdLessThanEqual:
    case spv::OpFOrdNotEqual:
    case spv::OpFRem:
    case spv::OpFSub:
    case spv::OpFUnordEqual:
    case spv::OpFUnordGreaterThan:
    case spv::OpFUnordGreaterThanEqual:
    case spv::OpFUnordLessThan:
    case spv::OpFUnordLessThanEqual:
    case spv::OpFUnordNotEqual:
    case spv::OpFunction:
    case spv::OpFunctionCall:
    case spv::OpFunctionParameter:
    case spv::OpFwidth:
    case spv::OpFwidthCoarse:
    case spv::OpFwidthFine:
    case spv::OpGenericCastToPtr:
    case spv::OpGenericCastToPtrExplicit:
    case spv::OpGenericPtrMemSemantics:
    case spv::OpGetDefaultQueue:
    case spv::OpGetKernelNDrangeMaxSubGroupSize:
    case spv::OpGetKernelNDrangeSubGroupCount:
    case spv::OpGetKernelPreferredWorkGroupSizeMultiple:
    case spv::OpGetKernelWorkGroupSize:
    case spv::OpGetMaxPipePackets:
    case spv::OpGetNumPipePackets:
    case spv::OpGroupAll:
    case spv::OpGroupAny:
    case spv::OpGroupAsyncCopy:
    case spv::OpGroupBroadcast:
    case spv::OpGroupFAdd:
    case spv::OpGroupFMax:
    case spv::OpGroupFMin:
    case spv::OpGroupIAdd:
    case spv::OpGroupReserveReadPipePackets:
    case spv::OpGroupReserveWritePipePackets:
    case spv::OpGroupSMax:
    case spv::OpGroupSMin:
    case spv::OpGroupUMax:
    case spv::OpGroupUMin:
    case spv::OpIAdd:
    case spv::OpIAddCarry:
    case spv::OpIEqual:
    case spv::OpIMul:
    case spv::OpINotEqual:
    case spv::OpISub:
    case spv::OpISubBorrow:
    case spv::OpImage:
    case spv::OpImageDrefGather:
    case spv::OpImageFetch:
    case spv::OpImageGather:
    case spv::OpImageQueryFormat:
    case spv::OpImageQueryLevels:
    case spv::OpImageQueryLod:
    case spv::OpImageQueryOrder:
    case spv::OpImageQuerySamples:
    case spv::OpImageQuerySize:
    case spv::OpImageQuerySizeLod:
    case spv::OpImageRead:
    case spv::OpImageSampleDrefExplicitLod:
    case spv::OpImageSampleDrefImplicitLod:
    case spv::OpImageSampleExplicitLod:
    case spv::OpImageSampleImplicitLod:
    case spv::OpImageSampleProjDrefExplicitLod:
    case spv::OpImageSampleProjDrefImplicitLod:
    case spv::OpImageSampleProjExplicitLod:
    case spv::OpImageSampleProjImplicitLod:
    case spv::OpImageSparseDrefGather:
    case spv::OpImageSparseFetch:
    case spv::OpImageSparseGather:
    case spv::OpImageSparseRead:
    case spv::OpImageSparseSampleDrefExplicitLod:
    case spv::OpImageSparseSampleDrefImplicitLod:
    case spv::OpImageSparseSampleExplicitLod:
    case spv::OpImageSparseSampleImplicitLod:
    case spv::OpImageSparseSampleProjDrefExplicitLod:
    case spv::OpImageSparseSampleProjDrefImplicitLod:
    case spv::OpImageSparseSampleProjExplicitLod:
    case spv::OpImageSparseSampleProjImplicitLod:
    case spv::OpImageSparseTexelsResident:
    case spv::OpImageTexelPointer:
    case spv::OpInBoundsAccessChain:
    case spv::OpInBoundsPtrAccessChain:
    case spv::OpIsFinite:
    case spv::OpIsInf:
    case spv::OpIsNan:
    case spv::OpIsNormal:
    case spv::OpIsValidEvent:
    case spv::OpIsValidReserveId:
    case spv::OpLessOrGreater:
    case spv::OpLoad:
    case spv::OpLogicalAnd:
    case spv::OpLogicalEqual:
    case spv::OpLogicalNot:
    case spv::OpLogicalNotEqual:
    case spv::OpLogicalOr:
    case spv::OpMatrixTimesMatrix:
    case spv::OpMatrixTimesScalar:
    case spv::OpMatrixTimesVector:
    case spv::OpNot:
    case spv::OpOrdered:
    case spv::OpOuterProduct:
    case spv::OpPhi:
    case spv::OpPtrAccessChain:
    case spv::OpPtrCastToGeneric:
    case spv::OpQuantizeToF16:
    case spv::OpReadPipe:
    case spv::OpReserveReadPipePackets:
    case spv::OpReserveWritePipePackets:
    case spv::OpReservedReadPipe:
    case spv::OpReservedWritePipe:
    case spv::OpSConvert:
    case spv::OpSDiv:
    case spv::OpSGreaterThan:
    case spv::OpSGreaterThanEqual:
    case spv::OpSLessThan:
    case spv::OpSLessThanEqual:
    case spv::OpSMod:
    case spv::OpSMulExtended:
    case spv::OpSNegate:
    case spv::OpSRem:
    case spv::OpSampledImage:
    case spv::OpSatConvertSToU:
    case spv::OpSatConvertUToS:
    case spv::OpSelect:
    case spv::OpShiftLeftLogical:
    case spv::OpShiftRightArithmetic:
    case spv::OpShiftRightLogical:
    case spv::OpSignBitSet:
    case spv::OpSpecConstant:
    case spv::OpSpecConstantComposite:
    case spv::OpSpecConstantFalse:
    case spv::OpSpecConstantOp:
    case spv::OpSpecConstantTrue:
    case spv::OpSubgroupAllEqualKHR:
    case spv::OpSubgroupAllKHR:
    case spv::OpSubgroupAnyKHR:
    case spv::OpSubgroupBallotKHR:
    case spv::OpSubgroupFirstInvocationKHR:
    case spv::OpSubgroupReadInvocationKHR:
    case spv::OpSubgroupShuffleINTEL:
    case spv::OpSubgroupShuffleUpINTEL:
    case spv::OpSubgroupShuffleDownINTEL:
    case spv::OpSubgroupShuffleXorINTEL:
    case spv::OpTranspose:
    case spv::OpUConvert:
    case spv::OpUDiv:
    case spv::OpUGreaterThan:
    case spv::OpUGreaterThanEqual:
    case spv::OpULessThan:
    case spv::OpULessThanEqual:
    case spv::OpUMod:
    case spv::OpUMulExtended:
    case spv::OpUndef:
    case spv::OpUnordered:
    case spv::OpVariable:
    case spv::OpVectorExtractDynamic:
    case spv::OpVectorInsertDynamic:
    case spv::OpVectorShuffle:
    case spv::OpVectorTimesMatrix:
    case spv::OpVectorTimesScalar:
    case spv::OpWritePipe:
      return true;
    default:
      return false;
  }
}

spv::Id OpResult::IdResultType() const { return getValueAtOffset(1); }

spv::Id OpResult::IdResult() const { return getValueAtOffset(2); }

spv::Decoration OpDecorateBase::getDecoration() const {
  return static_cast<spv::Decoration>(
      code == spv::OpDecorate ? getValueAtOffset(2) : getValueAtOffset(3));
}

llvm::StringRef OpSourceContinued::ContinuedSource() const {
  return reinterpret_cast<char const *>(data + 1);
}

spv::SourceLanguage OpSource::SourceLanguage() const {
  return static_cast<spv::SourceLanguage>(getValueAtOffset(1));
}

uint32_t OpSource::Version() const { return getValueAtOffset(2); }

spv::Id OpSource::File() const { return getValueAtOffset(3); }

llvm::StringRef OpSource::Source() const {
  return reinterpret_cast<char const *>(data + 4);
}

llvm::StringRef OpSourceExtension::Extension() const {
  return reinterpret_cast<char const *>(data + 1);
}

spv::Id OpName::Target() const { return getValueAtOffset(1); }

llvm::StringRef OpName::Name() const {
  return reinterpret_cast<char const *>(data + 2);
}

spv::Id OpMemberName::Type() const { return getValueAtOffset(1); }

uint32_t OpMemberName::Member() const { return getValueAtOffset(2); }

llvm::StringRef OpMemberName::Name() const {
  return reinterpret_cast<char const *>(data + 3);
}

spv::Id OpString::IdResult() const { return getValueAtOffset(1); }

llvm::StringRef OpString::String() const {
  return reinterpret_cast<char const *>(data + 2);
}

spv::Id OpLine::File() const { return getValueAtOffset(1); }

uint32_t OpLine::Line() const { return getValueAtOffset(2); }

uint32_t OpLine::Column() const { return getValueAtOffset(3); }

llvm::StringRef OpExtension::Name() const {
  return reinterpret_cast<char const *>(data + 1);
}

spv::Id OpExtInstImport::IdResult() const { return getValueAtOffset(1); }

llvm::StringRef OpExtInstImport::Name() const {
  return reinterpret_cast<char const *>(data + 2);
}

spv::Id OpExtInst::Set() const { return getValueAtOffset(3); }

uint32_t OpExtInst::Instruction() const { return getValueAtOffset(4); }

llvm::SmallVector<spv::Id, 8> OpExtInst::Operands() const {
  llvm::SmallVector<spv::Id, 8> operands;
  for (uint16_t i = 5; i < wordCount(); i++) {
    operands.push_back(getValueAtOffset(i));
  }
  return operands;
}

spv::AddressingModel OpMemoryModel::AddressingModel() const {
  return static_cast<spv::AddressingModel>(getValueAtOffset(1));
}

spv::MemoryModel OpMemoryModel::MemoryModel() const {
  return static_cast<spv::MemoryModel>(getValueAtOffset(2));
}

spv::ExecutionModel OpEntryPoint::ExecutionModel() const {
  return static_cast<spv::ExecutionModel>(getValueAtOffset(1));
}

spv::Id OpEntryPoint::EntryPoint() const { return getValueAtOffset(2); }

llvm::StringRef OpEntryPoint::Name() const {
  return reinterpret_cast<char const *>(data + 3);
}

llvm::SmallVector<spv::Id, 8> OpEntryPoint::Interface() const {
  llvm::SmallVector<spv::Id, 8> interface;
  // In SPIR-V since everything is 32 bit words, including null terminators, we
  // need to add one to the offset here for the null terminator on the string.
  // TODO CA-2650: This divide rounds down, but should round up.
  for (uint16_t i = (3 + (Name().size() / 4)) + 1; i < wordCount(); ++i) {
    interface.push_back(getValueAtOffset(i));
  }
  return interface;
}

spv::Id OpExecutionMode::EntryPoint() const { return getValueAtOffset(1); }

spv::ExecutionMode OpExecutionMode::Mode() const {
  return static_cast<spv::ExecutionMode>(getValueAtOffset(2));
}

spv::Capability OpCapability::Capability() const {
  return static_cast<spv::Capability>(getValueAtOffset(1));
}

uint32_t OpTypeInt::Width() const { return getValueAtOffset(2); }

uint32_t OpTypeInt::Signedness() const { return getValueAtOffset(3); }

uint32_t OpTypeFloat::Width() const { return getValueAtOffset(2); }

spv::Id OpTypeVector::ComponentType() const { return getValueAtOffset(2); }

uint32_t OpTypeVector::ComponentCount() const { return getValueAtOffset(3); }

spv::Id OpTypeMatrix::ColumnType() const { return getValueAtOffset(2); }

uint32_t OpTypeMatrix::ColumnCount() const { return getValueAtOffset(3); }

spv::Id OpTypeImage::SampledType() const { return getValueAtOffset(2); }

spv::Dim OpTypeImage::Dim() const {
  return static_cast<spv::Dim>(getValueAtOffset(3));
}

uint32_t OpTypeImage::Depth() const { return getValueAtOffset(4); }

uint32_t OpTypeImage::Arrayed() const { return getValueAtOffset(5); }

uint32_t OpTypeImage::MS() const { return getValueAtOffset(6); }

uint32_t OpTypeImage::Sampled() const { return getValueAtOffset(7); }

spv::ImageFormat OpTypeImage::ImageFormat() const {
  return static_cast<spv::ImageFormat>(getValueAtOffset(8));
}

spv::AccessQualifier OpTypeImage::AccessQualifier() const {
  return static_cast<spv::AccessQualifier>(getValueAtOffset(9));
}

spv::Id OpTypeSampledImage::ImageType() const { return getValueAtOffset(2); }

spv::Id OpTypeArray::ElementType() const { return getValueAtOffset(2); }

spv::Id OpTypeArray::Length() const { return getValueAtOffset(3); }

spv::Id OpTypeRuntimeArray::ElementType() const { return getValueAtOffset(2); }

llvm::SmallVector<spv::Id, 8> OpTypeStruct::MemberTypes() const {
  llvm::SmallVector<spv::Id, 8> memberTypes;
  for (uint16_t i = 2; i < wordCount(); i++) {
    memberTypes.push_back(getValueAtOffset(i));
  }
  return memberTypes;
}

llvm::StringRef OpTypeOpaque::Name() const {
  return reinterpret_cast<char const *>(data + 2);
}

uint32_t OpTypePointer::StorageClass() const {
  return static_cast<spv::StorageClass>(getValueAtOffset(2));
}

spv::Id OpTypePointer::Type() const { return getValueAtOffset(3); }

spv::Id OpTypeFunction::ReturnType() const { return getValueAtOffset(2); }

llvm::SmallVector<spv::Id, 8> OpTypeFunction::ParameterTypes() const {
  llvm::SmallVector<spv::Id, 8> parameterTypes;
  for (uint16_t i = 3; i < wordCount(); i++) {
    parameterTypes.push_back(getValueAtOffset(i));
  }
  return parameterTypes;
}

spv::AccessQualifier OpTypePipe::Qualifier() const {
  return static_cast<spv::AccessQualifier>(getValueAtOffset(2));
}

spv::Id OpTypeForwardPointer::PointerType() const {
  return getValueAtOffset(1);
}

spv::StorageClass OpTypeForwardPointer::StorageClass() const {
  return static_cast<spv::StorageClass>(getValueAtOffset(2));
}

uint32_t OpConstant::Value32() const { return getValueAtOffset(3); }

uint64_t OpConstant::Value64() const {
  uint64_t lowWord = getValueAtOffset(3);
  uint64_t highWord = getValueAtOffset(4);
  return lowWord | (highWord << 32);
}

llvm::SmallVector<spv::Id, 8> OpConstantComposite::Constituents() const {
  llvm::SmallVector<spv::Id, 8> constituents;
  for (uint16_t i = 3; i < wordCount(); i++) {
    constituents.push_back(getValueAtOffset(i));
  }
  return constituents;
}

spv::SamplerAddressingMode OpConstantSampler::SamplerAddressingMode() const {
  return static_cast<spv::SamplerAddressingMode>(getValueAtOffset(3));
}

uint32_t OpConstantSampler::Param() const { return getValueAtOffset(4); }

spv::SamplerFilterMode OpConstantSampler::SamplerFilterMode() const {
  return static_cast<spv::SamplerFilterMode>(getValueAtOffset(5));
}

uint32_t OpSpecConstant::Value32() const { return getValueAtOffset(3); }

uint64_t OpSpecConstant::Value64() const {
  uint64_t lowWord = getValueAtOffset(3);
  uint64_t highWord = getValueAtOffset(4);
  return lowWord | (highWord << 32);
}

llvm::SmallVector<spv::Id, 8> OpSpecConstantComposite::Constituents() const {
  llvm::SmallVector<spv::Id, 8> constituents;
  for (uint16_t i = 3; i < wordCount(); i++) {
    constituents.push_back(getValueAtOffset(i));
  }
  return constituents;
}

uint32_t OpSpecConstantOp::Opcode() const { return getValueAtOffset(3); }

uint32_t OpFunction::FunctionControl() const { return getValueAtOffset(3); }

spv::Id OpFunction::FunctionType() const { return getValueAtOffset(4); }

spv::Id OpFunctionCall::Function() const { return getValueAtOffset(3); }

llvm::SmallVector<spv::Id, 8> OpFunctionCall::Arguments() const {
  llvm::SmallVector<spv::Id, 8> arguments;
  for (uint16_t i = 4; i < wordCount(); i++) {
    arguments.push_back(getValueAtOffset(i));
  }
  return arguments;
}

uint32_t OpVariable::StorageClass() const {
  return static_cast<spv::StorageClass>(getValueAtOffset(3));
}

spv::Id OpVariable::Initializer() const { return getValueAtOffset(4); }

spv::Id OpImageTexelPointer::Image() const { return getValueAtOffset(3); }

spv::Id OpImageTexelPointer::Coordinate() const { return getValueAtOffset(4); }

spv::Id OpImageTexelPointer::Sample() const { return getValueAtOffset(5); }

spv::Id OpLoad::Pointer() const { return getValueAtOffset(3); }

uint32_t OpLoad::MemoryAccess() const { return getValueAtOffset(4); }

spv::Id OpStore::Pointer() const { return getValueAtOffset(1); }

spv::Id OpStore::Object() const { return getValueAtOffset(2); }

uint32_t OpStore::MemoryAccess() const { return getValueAtOffset(3); }

spv::Id OpCopyMemory::Target() const { return getValueAtOffset(1); }

spv::Id OpCopyMemory::Source() const { return getValueAtOffset(2); }

uint32_t OpCopyMemory::MemoryAccess() const { return getValueAtOffset(3); }

spv::Id OpCopyMemorySized::Target() const { return getValueAtOffset(1); }

spv::Id OpCopyMemorySized::Source() const { return getValueAtOffset(2); }

spv::Id OpCopyMemorySized::Size() const { return getValueAtOffset(3); }

uint32_t OpCopyMemorySized::MemoryAccess() const { return getValueAtOffset(4); }

spv::Id OpAccessChain::Base() const { return getValueAtOffset(3); }

llvm::SmallVector<spv::Id, 8> OpAccessChain::Indexes() const {
  llvm::SmallVector<spv::Id, 8> indexes;
  for (uint16_t i = 4; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpInBoundsAccessChain::Base() const { return getValueAtOffset(3); }

llvm::SmallVector<spv::Id, 8> OpInBoundsAccessChain::Indexes() const {
  llvm::SmallVector<spv::Id, 8> indexes;
  for (uint16_t i = 4; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpPtrAccessChain::Base() const { return getValueAtOffset(3); }

spv::Id OpPtrAccessChain::Element() const { return getValueAtOffset(4); }

llvm::SmallVector<spv::Id, 8> OpPtrAccessChain::Indexes() const {
  llvm::SmallVector<spv::Id, 8> indexes;
  for (uint16_t i = 5; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpArrayLength::Structure() const { return getValueAtOffset(3); }

uint32_t OpArrayLength::Arraymember() const { return getValueAtOffset(4); }

spv::Id OpGenericPtrMemSemantics::Pointer() const {
  return getValueAtOffset(3);
}

spv::Id OpInBoundsPtrAccessChain::Base() const { return getValueAtOffset(3); }

spv::Id OpInBoundsPtrAccessChain::Element() const {
  return getValueAtOffset(4);
}

llvm::SmallVector<spv::Id, 8> OpInBoundsPtrAccessChain::Indexes() const {
  llvm::SmallVector<spv::Id, 8> indexes;
  for (uint16_t i = 5; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpDecorate::Target() const { return getValueAtOffset(1); }

spv::Decoration OpDecorate::Decoration() const {
  return static_cast<spv::Decoration>(getValueAtOffset(2));
}

const char *OpDecorate::getDecorationString() const {
  return reinterpret_cast<const char *>(data + 3);
}

spv::Id OpMemberDecorate::StructureType() const { return getValueAtOffset(1); }

uint32_t OpMemberDecorate::Member() const { return getValueAtOffset(2); }

spv::Decoration OpMemberDecorate::Decoration() const {
  return static_cast<spv::Decoration>(getValueAtOffset(3));
}

spv::Id OpDecorationGroup::IdResult() const { return getValueAtOffset(1); }

spv::Id OpGroupDecorate::DecorationGroup() const { return getValueAtOffset(1); }

llvm::SmallVector<spv::Id, 8> OpGroupDecorate::Targets() const {
  llvm::SmallVector<spv::Id, 8> targets;
  for (uint16_t i = 2; i < wordCount(); i++) {
    targets.push_back(getValueAtOffset(i));
  }
  return targets;
}

spv::Id OpGroupMemberDecorate::DecorationGroup() const {
  return getValueAtOffset(1);
}

llvm::SmallVector<OpGroupMemberDecorate::TargetsT, 4>
OpGroupMemberDecorate::Targets() const {
  llvm::SmallVector<TargetsT, 4> targets;
  for (uint16_t i = 2; i < wordCount(); i += 2) {
    targets.push_back({getValueAtOffset(i), getValueAtOffset(i + 1)});
  }
  return targets;
}

spv::Id OpVectorExtractDynamic::Vector() const { return getValueAtOffset(3); }

spv::Id OpVectorExtractDynamic::Index() const { return getValueAtOffset(4); }

spv::Id OpVectorInsertDynamic::Vector() const { return getValueAtOffset(3); }

spv::Id OpVectorInsertDynamic::Component() const { return getValueAtOffset(4); }

spv::Id OpVectorInsertDynamic::Index() const { return getValueAtOffset(5); }

spv::Id OpVectorShuffle::Vector1() const { return getValueAtOffset(3); }

spv::Id OpVectorShuffle::Vector2() const { return getValueAtOffset(4); }

llvm::SmallVector<uint32_t, 16> OpVectorShuffle::Components() const {
  llvm::SmallVector<uint32_t, 16> components;
  for (uint16_t i = 5; i < wordCount(); ++i) {
    components.push_back(getValueAtOffset(i));
  }
  return components;
}

llvm::SmallVector<spv::Id, 8> OpCompositeConstruct::Constituents() const {
  llvm::SmallVector<spv::Id, 8> constituents;
  for (uint16_t i = 3; i < wordCount(); i++) {
    constituents.push_back(getValueAtOffset(i));
  }
  return constituents;
}

spv::Id OpCompositeExtract::Composite() const { return getValueAtOffset(3); }

llvm::SmallVector<uint32_t, 4> OpCompositeExtract::Indexes() const {
  llvm::SmallVector<uint32_t, 4> indexes;
  for (uint16_t i = 4; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpCompositeInsert::Object() const { return getValueAtOffset(3); }

spv::Id OpCompositeInsert::Composite() const { return getValueAtOffset(4); }

llvm::SmallVector<uint32_t, 4> OpCompositeInsert::Indexes() const {
  llvm::SmallVector<uint32_t, 4> indexes;
  for (uint16_t i = 5; i < wordCount(); i++) {
    indexes.push_back(getValueAtOffset(i));
  }
  return indexes;
}

spv::Id OpCopyObject::Operand() const { return getValueAtOffset(3); }

spv::Id OpTranspose::Matrix() const { return getValueAtOffset(3); }

spv::Id OpSampledImage::Image() const { return getValueAtOffset(3); }

spv::Id OpSampledImage::Sampler() const { return getValueAtOffset(4); }

spv::Id OpImageSampleImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSampleImplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSampleExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSampleExplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSampleDrefImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleDrefImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSampleDrefImplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSampleDrefImplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSampleDrefExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleDrefExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSampleDrefExplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSampleDrefExplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSampleProjImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleProjImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSampleProjImplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSampleProjExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleProjExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSampleProjExplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSampleProjDrefImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleProjDrefImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSampleProjDrefImplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSampleProjDrefImplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSampleProjDrefExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSampleProjDrefExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSampleProjDrefExplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSampleProjDrefExplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageFetch::Image() const { return getValueAtOffset(3); }

spv::Id OpImageFetch::Coordinate() const { return getValueAtOffset(4); }

uint32_t OpImageFetch::ImageOperands() const { return getValueAtOffset(5); }

spv::Id OpImageGather::SampledImage() const { return getValueAtOffset(3); }

spv::Id OpImageGather::Coordinate() const { return getValueAtOffset(4); }

spv::Id OpImageGather::Component() const { return getValueAtOffset(5); }

uint32_t OpImageGather::ImageOperands() const { return getValueAtOffset(6); }

spv::Id OpImageDrefGather::SampledImage() const { return getValueAtOffset(3); }

spv::Id OpImageDrefGather::Coordinate() const { return getValueAtOffset(4); }

spv::Id OpImageDrefGather::Dref() const { return getValueAtOffset(5); }

uint32_t OpImageDrefGather::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageRead::Image() const { return getValueAtOffset(3); }

spv::Id OpImageRead::Coordinate() const { return getValueAtOffset(4); }

uint32_t OpImageRead::ImageOperands() const { return getValueAtOffset(5); }

spv::Id OpImageWrite::Image() const { return getValueAtOffset(1); }

spv::Id OpImageWrite::Coordinate() const { return getValueAtOffset(2); }

spv::Id OpImageWrite::Texel() const { return getValueAtOffset(3); }

uint32_t OpImageWrite::ImageOperands() const { return getValueAtOffset(4); }

spv::Id OpImage::SampledImage() const { return getValueAtOffset(3); }

spv::Id OpImageQueryFormat::Image() const { return getValueAtOffset(3); }

spv::Id OpImageQueryOrder::Image() const { return getValueAtOffset(3); }

spv::Id OpImageQuerySizeLod::Image() const { return getValueAtOffset(3); }

spv::Id OpImageQuerySizeLod::LevelofDetail() const {
  return getValueAtOffset(4);
}

spv::Id OpImageQuerySize::Image() const { return getValueAtOffset(3); }

spv::Id OpImageQueryLod::SampledImage() const { return getValueAtOffset(3); }

spv::Id OpImageQueryLod::Coordinate() const { return getValueAtOffset(4); }

spv::Id OpImageQueryLevels::Image() const { return getValueAtOffset(3); }

spv::Id OpImageQuerySamples::Image() const { return getValueAtOffset(3); }

spv::Id OpConvertFToU::FloatValue() const { return getValueAtOffset(3); }

spv::Id OpConvertFToS::FloatValue() const { return getValueAtOffset(3); }

spv::Id OpConvertSToF::SignedValue() const { return getValueAtOffset(3); }

spv::Id OpConvertUToF::UnsignedValue() const { return getValueAtOffset(3); }

spv::Id OpUConvert::UnsignedValue() const { return getValueAtOffset(3); }

spv::Id OpSConvert::SignedValue() const { return getValueAtOffset(3); }

spv::Id OpFConvert::FloatValue() const { return getValueAtOffset(3); }

spv::Id OpQuantizeToF16::Value() const { return getValueAtOffset(3); }

spv::Id OpConvertPtrToU::Pointer() const { return getValueAtOffset(3); }

spv::Id OpSatConvertSToU::SignedValue() const { return getValueAtOffset(3); }

spv::Id OpSatConvertUToS::UnsignedValue() const { return getValueAtOffset(3); }

spv::Id OpConvertUToPtr::IntegerValue() const { return getValueAtOffset(3); }

spv::Id OpPtrCastToGeneric::Pointer() const { return getValueAtOffset(3); }

spv::Id OpGenericCastToPtr::Pointer() const { return getValueAtOffset(3); }

spv::Id OpGenericCastToPtrExplicit::Pointer() const {
  return getValueAtOffset(3);
}

spv::StorageClass OpGenericCastToPtrExplicit::Storage() const {
  return static_cast<spv::StorageClass>(getValueAtOffset(4));
}

spv::Id OpBitcast::Operand() const { return getValueAtOffset(3); }

spv::Id OpSNegate::Operand() const { return getValueAtOffset(3); }

spv::Id OpFNegate::Operand() const { return getValueAtOffset(3); }

spv::Id OpIAdd::Operand1() const { return getValueAtOffset(3); }

spv::Id OpIAdd::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFAdd::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFAdd::Operand2() const { return getValueAtOffset(4); }

spv::Id OpISub::Operand1() const { return getValueAtOffset(3); }

spv::Id OpISub::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFSub::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFSub::Operand2() const { return getValueAtOffset(4); }

spv::Id OpIMul::Operand1() const { return getValueAtOffset(3); }

spv::Id OpIMul::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFMul::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFMul::Operand2() const { return getValueAtOffset(4); }

spv::Id OpUDiv::Operand1() const { return getValueAtOffset(3); }

spv::Id OpUDiv::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSDiv::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSDiv::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFDiv::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFDiv::Operand2() const { return getValueAtOffset(4); }

spv::Id OpUMod::Operand1() const { return getValueAtOffset(3); }

spv::Id OpUMod::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSRem::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSRem::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSMod::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSMod::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFRem::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFRem::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFMod::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFMod::Operand2() const { return getValueAtOffset(4); }

spv::Id OpVectorTimesScalar::Vector() const { return getValueAtOffset(3); }

spv::Id OpVectorTimesScalar::Scalar() const { return getValueAtOffset(4); }

spv::Id OpMatrixTimesScalar::Matrix() const { return getValueAtOffset(3); }

spv::Id OpMatrixTimesScalar::Scalar() const { return getValueAtOffset(4); }

spv::Id OpVectorTimesMatrix::Vector() const { return getValueAtOffset(3); }

spv::Id OpVectorTimesMatrix::Matrix() const { return getValueAtOffset(4); }

spv::Id OpMatrixTimesVector::Matrix() const { return getValueAtOffset(3); }

spv::Id OpMatrixTimesVector::Vector() const { return getValueAtOffset(4); }

spv::Id OpMatrixTimesMatrix::LeftMatrix() const { return getValueAtOffset(3); }

spv::Id OpMatrixTimesMatrix::RightMatrix() const { return getValueAtOffset(4); }

spv::Id OpOuterProduct::Vector1() const { return getValueAtOffset(3); }

spv::Id OpOuterProduct::Vector2() const { return getValueAtOffset(4); }

spv::Id OpDot::Vector1() const { return getValueAtOffset(3); }

spv::Id OpDot::Vector2() const { return getValueAtOffset(4); }

spv::Id OpIAddCarry::Operand1() const { return getValueAtOffset(3); }

spv::Id OpIAddCarry::Operand2() const { return getValueAtOffset(4); }

spv::Id OpISubBorrow::Operand1() const { return getValueAtOffset(3); }

spv::Id OpISubBorrow::Operand2() const { return getValueAtOffset(4); }

spv::Id OpUMulExtended::Operand1() const { return getValueAtOffset(3); }

spv::Id OpUMulExtended::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSMulExtended::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSMulExtended::Operand2() const { return getValueAtOffset(4); }

spv::Id OpAny::Vector() const { return getValueAtOffset(3); }

spv::Id OpAll::Vector() const { return getValueAtOffset(3); }

spv::Id OpIsNan::x() const { return getValueAtOffset(3); }

spv::Id OpIsInf::x() const { return getValueAtOffset(3); }

spv::Id OpIsFinite::x() const { return getValueAtOffset(3); }

spv::Id OpIsNormal::x() const { return getValueAtOffset(3); }

spv::Id OpSignBitSet::x() const { return getValueAtOffset(3); }

spv::Id OpLessOrGreater::x() const { return getValueAtOffset(3); }

spv::Id OpLessOrGreater::y() const { return getValueAtOffset(4); }

spv::Id OpOrdered::x() const { return getValueAtOffset(3); }

spv::Id OpOrdered::y() const { return getValueAtOffset(4); }

spv::Id OpUnordered::x() const { return getValueAtOffset(3); }

spv::Id OpUnordered::y() const { return getValueAtOffset(4); }

spv::Id OpLogicalEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpLogicalEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpLogicalNotEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpLogicalNotEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpLogicalOr::Operand1() const { return getValueAtOffset(3); }

spv::Id OpLogicalOr::Operand2() const { return getValueAtOffset(4); }

spv::Id OpLogicalAnd::Operand1() const { return getValueAtOffset(3); }

spv::Id OpLogicalAnd::Operand2() const { return getValueAtOffset(4); }

spv::Id OpLogicalNot::Operand() const { return getValueAtOffset(3); }

spv::Id OpSelect::Condition() const { return getValueAtOffset(3); }

spv::Id OpSelect::Object1() const { return getValueAtOffset(4); }

spv::Id OpSelect::Object2() const { return getValueAtOffset(5); }

spv::Id OpIEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpIEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpINotEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpINotEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpUGreaterThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpUGreaterThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSGreaterThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSGreaterThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpUGreaterThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpUGreaterThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSGreaterThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSGreaterThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpULessThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpULessThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSLessThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSLessThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpULessThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpULessThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpSLessThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpSLessThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFUnordEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdNotEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdNotEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordNotEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFUnordNotEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdLessThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdLessThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordLessThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFUnordLessThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdGreaterThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdGreaterThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordGreaterThan::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFUnordGreaterThan::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdLessThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdLessThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordLessThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFUnordLessThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFOrdGreaterThanEqual::Operand1() const { return getValueAtOffset(3); }

spv::Id OpFOrdGreaterThanEqual::Operand2() const { return getValueAtOffset(4); }

spv::Id OpFUnordGreaterThanEqual::Operand1() const {
  return getValueAtOffset(3);
}

spv::Id OpFUnordGreaterThanEqual::Operand2() const {
  return getValueAtOffset(4);
}

spv::Id OpShiftRightLogical::Base() const { return getValueAtOffset(3); }

spv::Id OpShiftRightLogical::Shift() const { return getValueAtOffset(4); }

spv::Id OpShiftRightArithmetic::Base() const { return getValueAtOffset(3); }

spv::Id OpShiftRightArithmetic::Shift() const { return getValueAtOffset(4); }

spv::Id OpShiftLeftLogical::Base() const { return getValueAtOffset(3); }

spv::Id OpShiftLeftLogical::Shift() const { return getValueAtOffset(4); }

spv::Id OpBitwiseOr::Operand1() const { return getValueAtOffset(3); }

spv::Id OpBitwiseOr::Operand2() const { return getValueAtOffset(4); }

spv::Id OpBitwiseXor::Operand1() const { return getValueAtOffset(3); }

spv::Id OpBitwiseXor::Operand2() const { return getValueAtOffset(4); }

spv::Id OpBitwiseAnd::Operand1() const { return getValueAtOffset(3); }

spv::Id OpBitwiseAnd::Operand2() const { return getValueAtOffset(4); }

spv::Id OpNot::Operand() const { return getValueAtOffset(3); }

spv::Id OpBitFieldInsert::Base() const { return getValueAtOffset(3); }

spv::Id OpBitFieldInsert::Insert() const { return getValueAtOffset(4); }

spv::Id OpBitFieldInsert::Offset() const { return getValueAtOffset(5); }

spv::Id OpBitFieldInsert::Count() const { return getValueAtOffset(6); }

spv::Id OpBitFieldSExtract::Base() const { return getValueAtOffset(3); }

spv::Id OpBitFieldSExtract::Offset() const { return getValueAtOffset(4); }

spv::Id OpBitFieldSExtract::Count() const { return getValueAtOffset(5); }

spv::Id OpBitFieldUExtract::Base() const { return getValueAtOffset(3); }

spv::Id OpBitFieldUExtract::Offset() const { return getValueAtOffset(4); }

spv::Id OpBitFieldUExtract::Count() const { return getValueAtOffset(5); }

spv::Id OpBitReverse::Base() const { return getValueAtOffset(3); }

spv::Id OpBitCount::Base() const { return getValueAtOffset(3); }

spv::Id OpDPdx::P() const { return getValueAtOffset(3); }

spv::Id OpDPdy::P() const { return getValueAtOffset(3); }

spv::Id OpFwidth::P() const { return getValueAtOffset(3); }

spv::Id OpDPdxFine::P() const { return getValueAtOffset(3); }

spv::Id OpDPdyFine::P() const { return getValueAtOffset(3); }

spv::Id OpFwidthFine::P() const { return getValueAtOffset(3); }

spv::Id OpDPdxCoarse::P() const { return getValueAtOffset(3); }

spv::Id OpDPdyCoarse::P() const { return getValueAtOffset(3); }

spv::Id OpFwidthCoarse::P() const { return getValueAtOffset(3); }

spv::Id OpEmitStreamVertex::Stream() const { return getValueAtOffset(1); }

spv::Id OpEndStreamPrimitive::Stream() const { return getValueAtOffset(1); }

spv::Id OpControlBarrier::Execution() const { return getValueAtOffset(1); }

spv::Id OpControlBarrier::Memory() const { return getValueAtOffset(2); }

spv::Id OpControlBarrier::Semantics() const { return getValueAtOffset(3); }

spv::Id OpMemoryBarrier::Memory() const { return getValueAtOffset(1); }

spv::Id OpMemoryBarrier::Semantics() const { return getValueAtOffset(2); }

spv::Id OpAtomicLoad::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicLoad::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicLoad::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicStore::Pointer() const { return getValueAtOffset(1); }

spv::Id OpAtomicStore::Scope() const { return getValueAtOffset(2); }

spv::Id OpAtomicStore::Semantics() const { return getValueAtOffset(3); }

spv::Id OpAtomicStore::Value() const { return getValueAtOffset(4); }

spv::Id OpAtomicExchange::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicExchange::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicExchange::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicExchange::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicCompareExchange::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicCompareExchange::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicCompareExchange::Equal() const { return getValueAtOffset(5); }

spv::Id OpAtomicCompareExchange::Unequal() const { return getValueAtOffset(6); }

spv::Id OpAtomicCompareExchange::Value() const { return getValueAtOffset(7); }

spv::Id OpAtomicCompareExchange::Comparator() const {
  return getValueAtOffset(8);
}

spv::Id OpAtomicCompareExchangeWeak::Pointer() const {
  return getValueAtOffset(3);
}

spv::Id OpAtomicCompareExchangeWeak::Scope() const {
  return getValueAtOffset(4);
}

spv::Id OpAtomicCompareExchangeWeak::Equal() const {
  return getValueAtOffset(5);
}

spv::Id OpAtomicCompareExchangeWeak::Unequal() const {
  return getValueAtOffset(6);
}

spv::Id OpAtomicCompareExchangeWeak::Value() const {
  return getValueAtOffset(7);
}

spv::Id OpAtomicCompareExchangeWeak::Comparator() const {
  return getValueAtOffset(8);
}

spv::Id OpAtomicIIncrement::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicIIncrement::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicIIncrement::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicIDecrement::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicIDecrement::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicIDecrement::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicIAdd::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicIAdd::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicIAdd::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicIAdd::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicISub::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicISub::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicISub::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicISub::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicSMin::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicSMin::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicSMin::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicSMin::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicUMin::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicUMin::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicUMin::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicUMin::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicSMax::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicSMax::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicSMax::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicSMax::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicUMax::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicUMax::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicUMax::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicUMax::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicFAddEXT::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicFAddEXT::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicFAddEXT::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicFAddEXT::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicFMinEXT::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicFMinEXT::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicFMinEXT::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicFMinEXT::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicFMaxEXT::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicFMaxEXT::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicFMaxEXT::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicFMaxEXT::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicAnd::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicAnd::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicAnd::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicAnd::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicOr::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicOr::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicOr::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicOr::Value() const { return getValueAtOffset(6); }

spv::Id OpAtomicXor::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicXor::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicXor::Semantics() const { return getValueAtOffset(5); }

spv::Id OpAtomicXor::Value() const { return getValueAtOffset(6); }

llvm::SmallVector<OpPhi::VariableParentT, 4> OpPhi::VariableParent() const {
  llvm::SmallVector<VariableParentT, 4> variableParents;
  for (uint16_t i = 3; i < wordCount(); i += 2) {
    variableParents.push_back({getValueAtOffset(i), getValueAtOffset(i + 1)});
  }
  return variableParents;
}

spv::Id OpLoopMerge::MergeBlock() const { return getValueAtOffset(1); }

spv::Id OpLoopMerge::ContinueTarget() const { return getValueAtOffset(2); }

uint32_t OpLoopMerge::LoopControl() const { return getValueAtOffset(3); }

spv::Id OpSelectionMerge::MergeBlock() const { return getValueAtOffset(1); }

uint32_t OpSelectionMerge::SelectionControl() const {
  return getValueAtOffset(2);
}

spv::Id OpLabel::IdResult() const { return getValueAtOffset(1); }

spv::Id OpBranch::TargetLabel() const { return getValueAtOffset(1); }

spv::Id OpBranchConditional::Condition() const { return getValueAtOffset(1); }

spv::Id OpBranchConditional::TrueLabel() const { return getValueAtOffset(2); }

spv::Id OpBranchConditional::FalseLabel() const { return getValueAtOffset(3); }

llvm::SmallVector<uint32_t, 2> OpBranchConditional::BranchWeights() const {
  llvm::SmallVector<uint32_t, 2> branchWeights;
  for (uint16_t i = 4; i < wordCount(); ++i) {
    branchWeights.push_back(getValueAtOffset(i));
  }
  return branchWeights;
}

spv::Id OpSwitch::Selector() const { return getValueAtOffset(1); }

spv::Id OpSwitch::Default() const { return getValueAtOffset(2); }

llvm::SmallVector<OpSwitch::TargetT, 4> OpSwitch::Target(
    uint16_t literalWords) const {
  llvm::SmallVector<TargetT, 4> targets;
  // Target pairs are an ID which is always one word and a literal which can be
  // one or two words.
  for (uint16_t i = 3; i < wordCount(); i += 1 + literalWords) {
    targets.push_back({getValueAtOffset(i, literalWords),
                       getValueAtOffset(i + literalWords)});
  }
  return targets;
}

spv::Id OpReturnValue::Value() const { return getValueAtOffset(1); }

spv::Id OpLifetimeStart::Pointer() const { return getValueAtOffset(1); }

uint32_t OpLifetimeStart::Size() const { return getValueAtOffset(2); }

spv::Id OpLifetimeStop::Pointer() const { return getValueAtOffset(1); }

uint32_t OpLifetimeStop::Size() const { return getValueAtOffset(2); }

spv::Id OpGroupAsyncCopy::Execution() const { return getValueAtOffset(3); }

spv::Id OpGroupAsyncCopy::Destination() const { return getValueAtOffset(4); }

spv::Id OpGroupAsyncCopy::Source() const { return getValueAtOffset(5); }

spv::Id OpGroupAsyncCopy::NumElements() const { return getValueAtOffset(6); }

spv::Id OpGroupAsyncCopy::Stride() const { return getValueAtOffset(7); }

spv::Id OpGroupAsyncCopy::Event() const { return getValueAtOffset(8); }

spv::Id OpGroupWaitEvents::Execution() const { return getValueAtOffset(1); }

spv::Id OpGroupWaitEvents::NumEvents() const { return getValueAtOffset(2); }

spv::Id OpGroupWaitEvents::EventsList() const { return getValueAtOffset(3); }

spv::Id OpGroupAll::Execution() const { return getValueAtOffset(3); }

spv::Id OpGroupAll::Predicate() const { return getValueAtOffset(4); }

spv::Id OpGroupAny::Execution() const { return getValueAtOffset(3); }

spv::Id OpGroupAny::Predicate() const { return getValueAtOffset(4); }

spv::Id OpGroupBroadcast::Execution() const { return getValueAtOffset(3); }

spv::Id OpGroupBroadcast::Value() const { return getValueAtOffset(4); }

spv::Id OpGroupBroadcast::LocalId() const { return getValueAtOffset(5); }

spv::Id OpReadPipe::Pipe() const { return getValueAtOffset(3); }

spv::Id OpReadPipe::Pointer() const { return getValueAtOffset(4); }

spv::Id OpReadPipe::PacketSize() const { return getValueAtOffset(5); }

spv::Id OpReadPipe::PacketAlignment() const { return getValueAtOffset(6); }

spv::Id OpWritePipe::Pipe() const { return getValueAtOffset(3); }

spv::Id OpWritePipe::Pointer() const { return getValueAtOffset(4); }

spv::Id OpWritePipe::PacketSize() const { return getValueAtOffset(5); }

spv::Id OpWritePipe::PacketAlignment() const { return getValueAtOffset(6); }

spv::Id OpReservedReadPipe::Pipe() const { return getValueAtOffset(3); }

spv::Id OpReservedReadPipe::ReserveId() const { return getValueAtOffset(4); }

spv::Id OpReservedReadPipe::Index() const { return getValueAtOffset(5); }

spv::Id OpReservedReadPipe::Pointer() const { return getValueAtOffset(6); }

spv::Id OpReservedReadPipe::PacketSize() const { return getValueAtOffset(7); }

spv::Id OpReservedReadPipe::PacketAlignment() const {
  return getValueAtOffset(8);
}

spv::Id OpReservedWritePipe::Pipe() const { return getValueAtOffset(3); }

spv::Id OpReservedWritePipe::ReserveId() const { return getValueAtOffset(4); }

spv::Id OpReservedWritePipe::Index() const { return getValueAtOffset(5); }

spv::Id OpReservedWritePipe::Pointer() const { return getValueAtOffset(6); }

spv::Id OpReservedWritePipe::PacketSize() const { return getValueAtOffset(7); }

spv::Id OpReservedWritePipe::PacketAlignment() const {
  return getValueAtOffset(8);
}

spv::Id OpReserveReadPipePackets::Pipe() const { return getValueAtOffset(3); }

spv::Id OpReserveReadPipePackets::NumPackets() const {
  return getValueAtOffset(4);
}

spv::Id OpReserveReadPipePackets::PacketSize() const {
  return getValueAtOffset(5);
}

spv::Id OpReserveReadPipePackets::PacketAlignment() const {
  return getValueAtOffset(6);
}

spv::Id OpReserveWritePipePackets::Pipe() const { return getValueAtOffset(3); }

spv::Id OpReserveWritePipePackets::NumPackets() const {
  return getValueAtOffset(4);
}

spv::Id OpReserveWritePipePackets::PacketSize() const {
  return getValueAtOffset(5);
}

spv::Id OpReserveWritePipePackets::PacketAlignment() const {
  return getValueAtOffset(6);
}

spv::Id OpCommitReadPipe::Pipe() const { return getValueAtOffset(1); }

spv::Id OpCommitReadPipe::ReserveId() const { return getValueAtOffset(2); }

spv::Id OpCommitReadPipe::PacketSize() const { return getValueAtOffset(3); }

spv::Id OpCommitReadPipe::PacketAlignment() const {
  return getValueAtOffset(4);
}

spv::Id OpCommitWritePipe::Pipe() const { return getValueAtOffset(1); }

spv::Id OpCommitWritePipe::ReserveId() const { return getValueAtOffset(2); }

spv::Id OpCommitWritePipe::PacketSize() const { return getValueAtOffset(3); }

spv::Id OpCommitWritePipe::PacketAlignment() const {
  return getValueAtOffset(4);
}

spv::Id OpIsValidReserveId::ReserveId() const { return getValueAtOffset(3); }

spv::Id OpGetNumPipePackets::Pipe() const { return getValueAtOffset(3); }

spv::Id OpGetNumPipePackets::PacketSize() const { return getValueAtOffset(4); }

spv::Id OpGetNumPipePackets::PacketAlignment() const {
  return getValueAtOffset(5);
}

spv::Id OpGetMaxPipePackets::Pipe() const { return getValueAtOffset(3); }

spv::Id OpGetMaxPipePackets::PacketSize() const { return getValueAtOffset(4); }

spv::Id OpGetMaxPipePackets::PacketAlignment() const {
  return getValueAtOffset(5);
}

spv::Id OpGroupReserveReadPipePackets::Execution() const {
  return getValueAtOffset(3);
}

spv::Id OpGroupReserveReadPipePackets::Pipe() const {
  return getValueAtOffset(4);
}

spv::Id OpGroupReserveReadPipePackets::NumPackets() const {
  return getValueAtOffset(5);
}

spv::Id OpGroupReserveReadPipePackets::PacketSize() const {
  return getValueAtOffset(6);
}

spv::Id OpGroupReserveReadPipePackets::PacketAlignment() const {
  return getValueAtOffset(7);
}

spv::Id OpGroupReserveWritePipePackets::Execution() const {
  return getValueAtOffset(3);
}

spv::Id OpGroupReserveWritePipePackets::Pipe() const {
  return getValueAtOffset(4);
}

spv::Id OpGroupReserveWritePipePackets::NumPackets() const {
  return getValueAtOffset(5);
}

spv::Id OpGroupReserveWritePipePackets::PacketSize() const {
  return getValueAtOffset(6);
}

spv::Id OpGroupReserveWritePipePackets::PacketAlignment() const {
  return getValueAtOffset(7);
}

spv::Id OpGroupCommitReadPipe::Execution() const { return getValueAtOffset(1); }

spv::Id OpGroupCommitReadPipe::Pipe() const { return getValueAtOffset(2); }

spv::Id OpGroupCommitReadPipe::ReserveId() const { return getValueAtOffset(3); }

spv::Id OpGroupCommitReadPipe::PacketSize() const {
  return getValueAtOffset(4);
}

spv::Id OpGroupCommitReadPipe::PacketAlignment() const {
  return getValueAtOffset(5);
}

spv::Id OpGroupCommitWritePipe::Execution() const {
  return getValueAtOffset(1);
}

spv::Id OpGroupCommitWritePipe::Pipe() const { return getValueAtOffset(2); }

spv::Id OpGroupCommitWritePipe::ReserveId() const {
  return getValueAtOffset(3);
}

spv::Id OpGroupCommitWritePipe::PacketSize() const {
  return getValueAtOffset(4);
}

spv::Id OpGroupCommitWritePipe::PacketAlignment() const {
  return getValueAtOffset(5);
}

spv::Id OpEnqueueMarker::Queue() const { return getValueAtOffset(3); }

spv::Id OpEnqueueMarker::NumEvents() const { return getValueAtOffset(4); }

spv::Id OpEnqueueMarker::WaitEvents() const { return getValueAtOffset(5); }

spv::Id OpEnqueueMarker::RetEvent() const { return getValueAtOffset(6); }

spv::Id OpEnqueueKernel::Queue() const { return getValueAtOffset(3); }

spv::Id OpEnqueueKernel::Flags() const { return getValueAtOffset(4); }

spv::Id OpEnqueueKernel::NDRange() const { return getValueAtOffset(5); }

spv::Id OpEnqueueKernel::NumEvents() const { return getValueAtOffset(6); }

spv::Id OpEnqueueKernel::WaitEvents() const { return getValueAtOffset(7); }

spv::Id OpEnqueueKernel::RetEvent() const { return getValueAtOffset(8); }

spv::Id OpEnqueueKernel::Invoke() const { return getValueAtOffset(9); }

spv::Id OpEnqueueKernel::Param() const { return getValueAtOffset(10); }

spv::Id OpEnqueueKernel::ParamSize() const { return getValueAtOffset(11); }

spv::Id OpEnqueueKernel::ParamAlign() const { return getValueAtOffset(12); }

llvm::SmallVector<spv::Id, 3> OpEnqueueKernel::LocalSize() const {
  llvm::SmallVector<spv::Id, 3> localSize;
  for (uint16_t i = 13; i < wordCount(); i++) {
    localSize.push_back(getValueAtOffset(i));
  }
  return localSize;
}

spv::Id OpGetKernelNDrangeSubGroupCount::NDRange() const {
  return getValueAtOffset(3);
}

spv::Id OpGetKernelNDrangeSubGroupCount::Invoke() const {
  return getValueAtOffset(4);
}

spv::Id OpGetKernelNDrangeSubGroupCount::Param() const {
  return getValueAtOffset(5);
}

spv::Id OpGetKernelNDrangeSubGroupCount::ParamSize() const {
  return getValueAtOffset(6);
}

spv::Id OpGetKernelNDrangeSubGroupCount::ParamAlign() const {
  return getValueAtOffset(7);
}

spv::Id OpGetKernelNDrangeMaxSubGroupSize::NDRange() const {
  return getValueAtOffset(3);
}

spv::Id OpGetKernelNDrangeMaxSubGroupSize::Invoke() const {
  return getValueAtOffset(4);
}

spv::Id OpGetKernelNDrangeMaxSubGroupSize::Param() const {
  return getValueAtOffset(5);
}

spv::Id OpGetKernelNDrangeMaxSubGroupSize::ParamSize() const {
  return getValueAtOffset(6);
}

spv::Id OpGetKernelNDrangeMaxSubGroupSize::ParamAlign() const {
  return getValueAtOffset(7);
}

spv::Id OpGetKernelWorkGroupSize::Invoke() const { return getValueAtOffset(3); }

spv::Id OpGetKernelWorkGroupSize::Param() const { return getValueAtOffset(4); }

spv::Id OpGetKernelWorkGroupSize::ParamSize() const {
  return getValueAtOffset(5);
}

spv::Id OpGetKernelWorkGroupSize::ParamAlign() const {
  return getValueAtOffset(6);
}

spv::Id OpGetKernelPreferredWorkGroupSizeMultiple::Invoke() const {
  return getValueAtOffset(3);
}

spv::Id OpGetKernelPreferredWorkGroupSizeMultiple::Param() const {
  return getValueAtOffset(4);
}

spv::Id OpGetKernelPreferredWorkGroupSizeMultiple::ParamSize() const {
  return getValueAtOffset(5);
}

spv::Id OpGetKernelPreferredWorkGroupSizeMultiple::ParamAlign() const {
  return getValueAtOffset(6);
}

spv::Id OpRetainEvent::Event() const { return getValueAtOffset(1); }

spv::Id OpReleaseEvent::Event() const { return getValueAtOffset(1); }

spv::Id OpIsValidEvent::Event() const { return getValueAtOffset(3); }

spv::Id OpSetUserEventStatus::Event() const { return getValueAtOffset(1); }

spv::Id OpSetUserEventStatus::Status() const { return getValueAtOffset(2); }

spv::Id OpCaptureEventProfilingInfo::Event() const {
  return getValueAtOffset(1);
}

spv::Id OpCaptureEventProfilingInfo::ProfilingInfo() const {
  return getValueAtOffset(2);
}

spv::Id OpCaptureEventProfilingInfo::Value() const {
  return getValueAtOffset(3);
}

spv::Id OpBuildNDRange::GlobalWorkSize() const { return getValueAtOffset(3); }

spv::Id OpBuildNDRange::LocalWorkSize() const { return getValueAtOffset(4); }

spv::Id OpBuildNDRange::GlobalWorkOffset() const { return getValueAtOffset(5); }

spv::Id OpGetKernelLocalSizeForSubgroupCount::SubgroupCount() const {
  return getValueAtOffset(3);
}
spv::Id OpGetKernelLocalSizeForSubgroupCount::Invoke() const {
  return getValueAtOffset(4);
}
spv::Id OpGetKernelLocalSizeForSubgroupCount::Param() const {
  return getValueAtOffset(5);
}
spv::Id OpGetKernelLocalSizeForSubgroupCount::ParamSize() const {
  return getValueAtOffset(6);
}
spv::Id OpGetKernelLocalSizeForSubgroupCount::ParamAlign() const {
  return getValueAtOffset(7);
}

spv::Id OpGetKernelMaxNumSubgroups::Invoke() const {
  return getValueAtOffset(3);
}
spv::Id OpGetKernelMaxNumSubgroups::Param() const {
  return getValueAtOffset(4);
}
spv::Id OpGetKernelMaxNumSubgroups::ParamSize() const {
  return getValueAtOffset(5);
}
spv::Id OpGetKernelMaxNumSubgroups::ParamAlign() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseSampleImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSparseSampleImplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSparseSampleExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSparseSampleExplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSparseSampleDrefImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleDrefImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSparseSampleDrefImplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSparseSampleDrefImplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseSampleDrefExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleDrefExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSparseSampleDrefExplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSparseSampleDrefExplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseSampleProjImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleProjImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSparseSampleProjImplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSparseSampleProjExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleProjExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

uint32_t OpImageSparseSampleProjExplicitLod::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSparseSampleProjDrefImplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleProjDrefImplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSparseSampleProjDrefImplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSparseSampleProjDrefImplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseSampleProjDrefExplicitLod::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseSampleProjDrefExplicitLod::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSparseSampleProjDrefExplicitLod::Dref() const {
  return getValueAtOffset(5);
}

uint32_t OpImageSparseSampleProjDrefExplicitLod::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseFetch::Image() const { return getValueAtOffset(3); }

spv::Id OpImageSparseFetch::Coordinate() const { return getValueAtOffset(4); }

uint32_t OpImageSparseFetch::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpImageSparseGather::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseGather::Coordinate() const { return getValueAtOffset(4); }

spv::Id OpImageSparseGather::Component() const { return getValueAtOffset(5); }

uint32_t OpImageSparseGather::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseDrefGather::SampledImage() const {
  return getValueAtOffset(3);
}

spv::Id OpImageSparseDrefGather::Coordinate() const {
  return getValueAtOffset(4);
}

spv::Id OpImageSparseDrefGather::Dref() const { return getValueAtOffset(5); }

uint32_t OpImageSparseDrefGather::ImageOperands() const {
  return getValueAtOffset(6);
}

spv::Id OpImageSparseTexelsResident::ResidentCode() const {
  return getValueAtOffset(3);
}

spv::Id OpAtomicFlagTestAndSet::Pointer() const { return getValueAtOffset(3); }

spv::Id OpAtomicFlagTestAndSet::Scope() const { return getValueAtOffset(4); }

spv::Id OpAtomicFlagTestAndSet::Semantics() const {
  return getValueAtOffset(5);
}

spv::Id OpAtomicFlagClear::Pointer() const { return getValueAtOffset(1); }

spv::Id OpAtomicFlagClear::Scope() const { return getValueAtOffset(2); }

spv::Id OpAtomicFlagClear::Semantics() const { return getValueAtOffset(3); }

spv::Id OpImageSparseRead::Image() const { return getValueAtOffset(3); }

spv::Id OpImageSparseRead::Coordinate() const { return getValueAtOffset(4); }

uint32_t OpImageSparseRead::ImageOperands() const {
  return getValueAtOffset(5);
}

spv::Id OpSubgroupBallotKHR::Predicate() const { return getValueAtOffset(3); }

spv::Id OpSubgroupFirstInvocationKHR::Value() const {
  return getValueAtOffset(3);
}

spv::Id OpSubgroupAllKHR::Predicate() const { return getValueAtOffset(3); }

spv::Id OpSubgroupAnyKHR::Predicate() const { return getValueAtOffset(3); }

spv::Id OpSubgroupAllEqualKHR::Predicate() const { return getValueAtOffset(3); }

spv::Id OpSubgroupReadInvocationKHR::Value() const {
  return getValueAtOffset(3);
}

spv::Id OpSubgroupReadInvocationKHR::Index() const {
  return getValueAtOffset(4);
}

spv::Id OpAssumeTrueKHR::Condition() const { return getValueAtOffset(1); }

spv::Id OpExpectKHR::Value() const { return getValueAtOffset(3); }
spv::Id OpExpectKHR::ExpectedValue() const { return getValueAtOffset(4); }

spv::Id OpenCLstd::Printf::format() const { return getValueAtOffset(5); }

llvm::SmallVector<spv::Id, 8> OpenCLstd::Printf::AdditionalArguments() const {
  llvm::SmallVector<spv::Id, 8> additionalArguments;
  for (uint16_t i = 6; i < wordCount(); i++) {
    additionalArguments.push_back(getValueAtOffset(i));
  }
  return additionalArguments;
}

std::string getCapabilityName(spv::Capability cap) {
  // Note: this can't be a switch because there are multiple capability names
  // with the same enum value. We must provide a full mapping from string to
  // capability, so in the reverse direction we accept some clashes.
  std::unordered_map<spv::Capability, const char *> capability_map = {
#define CAPABILITY(ENUM, NAME) {ENUM, NAME},
#include <spirv-ll/name_utils.inc>
  };
  const char *cap_name = "Unknown";
  auto it = capability_map.find(cap);
  if (it != capability_map.end()) {
    cap_name = it->second;
  }
  return std::string(cap_name) + " (#" + std::to_string(cap) + ")";
}

std::optional<spv::Capability> getCapabilityFromString(const std::string &cap) {
  return llvm::StringSwitch<spv::Capability>(llvm::StringRef(cap))
#define CAPABILITY(ENUM, NAME) .Case(NAME, ENUM)
#include <spirv-ll/name_utils.inc>
      .Default(static_cast<spv::Capability>(0));
}

}  // namespace spirv_ll
