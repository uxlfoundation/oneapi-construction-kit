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

#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv/unified1/spirv.hpp>

spirv_ll::Context::Context()
    : llvmContext(new llvm::LLVMContext), llvmContextIsOwned(true) {}

spirv_ll::Context::Context(llvm::LLVMContext *llvmContext)
    : llvmContext(llvmContext), llvmContextIsOwned(false) {}

spirv_ll::Context::~Context() {
  if (llvmContextIsOwned && llvmContext) {
    delete llvmContext;
  }
}

cargo::expected<spirv_ll::SpecializableConstantsMap, spirv_ll::Error>
spirv_ll::Context::getSpecializableConstants(llvm::ArrayRef<uint32_t> code) {
  spirv_ll::Module module{*this, code};
  if (!module.isValid()) {
    return cargo::make_unexpected(Error{"invalid SPIR-V module binary"});
  }

  llvm::DenseMap<spv::Id, uint32_t> specIds;
  llvm::DenseMap<spv::Id, const OpType *> types;
  SpecializableConstantsMap specConstants;
  for (auto op : module) {
    // All spec constants must be defined before functions, when a function is
    // found we can exit early.
    if (op.code == spv::OpFunction) {
      break;
    }

    switch (op.code) {
      default:  // Ignore opcodes irrelevant to spec constants.
        break;

      case spv::OpDecorate: {
        auto *opDecorate = module.create<OpDecorate>(op);
        if (opDecorate->getDecoration() == spv::DecorationSpecId) {
          specIds[opDecorate->Target()] = opDecorate->getValueAtOffset(3);
        }
      } break;

      // Create the relevant type definitions for later use.
      case spv::OpTypeBool: {
        auto *opType = module.create<OpTypeBool>(op);
        types.insert(
            std::pair<spv::Id, const OpType *>{opType->IdResult(), opType});
      } break;
      case spv::OpTypeInt: {
        auto *opType = module.create<OpTypeInt>(op);
        types.insert(
            std::pair<spv::Id, const OpType *>{opType->IdResult(), opType});
      } break;
      case spv::OpTypeFloat: {
        auto *opType = module.create<OpTypeFloat>(op);
        types.insert(
            std::pair<spv::Id, const OpType *>{opType->IdResult(), opType});
      } break;

      // Boolean spec constants are given a size of 1 bit.
      case spv::OpSpecConstantTrue: {
        auto *opSpecConstantTrue = module.create<OpSpecConstantTrue>(op);
        if (specIds.count(opSpecConstantTrue->IdResult()) != 0) {
          specConstants.insert(std::pair<spv::Id, SpecializationDesc>{
              specIds[opSpecConstantTrue->IdResult()],
              {SpecializationType::BOOL, 1}});
        }
      } break;
      case spv::OpSpecConstantFalse: {
        auto *opSpecConstantFalse = module.create<OpSpecConstantFalse>(op);
        if (specIds.count(opSpecConstantFalse->IdResult()) != 0) {
          specConstants.insert(std::pair<spv::Id, SpecializationDesc>{
              specIds[opSpecConstantFalse->IdResult()],
              {SpecializationType::BOOL, 1}});
        }
      } break;

      // Look up the size in bits from the type for number spec cosntants.
      case spv::OpSpecConstant: {
        auto *opSpecConstant = module.create<OpSpecConstant>(op);
        if (types.count(opSpecConstant->IdResultType()) == 0) {
          return cargo::make_unexpected(
              Error{"unknown SPIR-V specialization constant result type"});
        }
        auto *opType = types[opSpecConstant->IdResultType()];
        if (opType->isIntType()) {
          if (specIds.count(opSpecConstant->IdResult()) != 0) {
            specConstants.insert(std::pair<spv::Id, SpecializationDesc>{
                specIds[opSpecConstant->IdResult()],
                {SpecializationType::INT, opType->getTypeInt()->Width()}});
          }
        } else if (opType->isFloatType()) {
          if (specIds.count(opSpecConstant->IdResult()) != 0) {
            specConstants.insert(std::pair<spv::Id, SpecializationDesc>{
                specIds[opSpecConstant->IdResult()],
                {SpecializationType::FLOAT, opType->getTypeFloat()->Width()}});
          }
        } else {
          return cargo::make_unexpected(
              Error{"invalid SPIR-V specialization constant type"});
        }
      } break;
    }
  }

  return specConstants;
}

cargo::expected<spirv_ll::Module, spirv_ll::Error> spirv_ll::Context::translate(
    llvm::ArrayRef<uint32_t> code, const spirv_ll::DeviceInfo &deviceInfo,
    cargo::optional<const spirv_ll::SpecializationInfo &> specInfo) {
  SPIRV_LL_ASSERT(llvmContext, "llvmContext must not be null");
  spirv_ll::Module module(*this, code, specInfo);
  if (!module.isValid()) {
    return cargo::make_unexpected(Error{"invalid SPIR-V module binary"});
  }

  spirv_ll::Builder builder(*this, module, deviceInfo);

  using IRInsertPoint = llvm::IRBuilder<>::InsertPoint;
  // InsertPoint stack for moving around the IR Module
  llvm::SmallVector<IRInsertPoint, 8> IPStack;
  // Store the branch instructions found in the current function, as we need to
  // generate them after all the basic blocks have been generated.
  using OpIRLocTy = std::pair<const OpCode, IRInsertPoint>;
  // Store the Phi nodes in order to add the values after all the basic blocks
  // have been generated
  llvm::SmallVector<OpIRLocTy, 8> Phis;

  for (auto op : module) {
    std::optional<llvm::Error> error;
    switch (op.code) {
        // Unsupported opcodes are ignored.
      default:
#ifndef NDEBUG
        // Only abort on unsupported opcodes in assert builds to help catch
        // possible bugs or missing features, however if we are consuming
        // SPIR-V which contains unsupported opcodes intentionally by the user
        // with the intent that the SPIR-V consumer simply ignores them, as is
        // allowed by the SPIR-V spec, then this abort should be removed.
        error =
            makeStringError("unsupported opcode: " + std::to_string(op.code));
#endif
        break;
      case spv::OpNop:
        error = builder.create<OpNop>(op);
        break;
      case spv::OpUndef:
        error = builder.create<OpUndef>(op);
        break;
      case spv::OpSourceContinued:
        error = builder.create<OpSourceContinued>(op);
        break;
      case spv::OpSource:
        error = builder.create<OpSource>(op);
        break;
      case spv::OpSourceExtension:
        error = builder.create<OpSourceExtension>(op);
        break;
      case spv::OpModuleProcessed:
        error = builder.create<OpModuleProcessed>(op);
        break;
      case spv::OpName:
        error = builder.create<OpName>(op);
        break;
      case spv::OpMemberName:
        error = builder.create<OpMemberName>(op);
        break;
      case spv::OpString:
        error = builder.create<OpString>(op);
        break;
      case spv::OpLine:
        error = builder.create<OpLine>(op);
        break;
      case spv::OpExtension:
        error = builder.create<OpExtension>(op);
        break;
      case spv::OpExtInstImport:
        error = builder.create<OpExtInstImport>(op);
        break;
      case spv::OpExtInst:
        error = builder.create<OpExtInst>(op);
        break;
      case spv::OpMemoryModel:
        error = builder.create<OpMemoryModel>(op);
        break;
      case spv::OpEntryPoint:
        error = builder.create<OpEntryPoint>(op);
        break;
      case spv::OpExecutionMode:
        error = builder.create<OpExecutionMode>(op);
        break;
      case spv::OpCapability:
        error = builder.create<OpCapability>(op);
        break;
      case spv::OpTypeVoid:
        error = builder.create<OpTypeVoid>(op);
        break;
      case spv::OpTypeBool:
        error = builder.create<OpTypeBool>(op);
        break;
      case spv::OpTypeInt:
        error = builder.create<OpTypeInt>(op);
        break;
      case spv::OpTypeFloat:
        error = builder.create<OpTypeFloat>(op);
        break;
      case spv::OpTypeVector:
        error = builder.create<OpTypeVector>(op);
        break;
      case spv::OpTypeMatrix:
        error = builder.create<OpTypeMatrix>(op);
        break;
      case spv::OpTypeImage:
        error = builder.create<OpTypeImage>(op);
        break;
      case spv::OpTypeSampler:
        error = builder.create<OpTypeSampler>(op);
        break;
      case spv::OpTypeSampledImage:
        error = builder.create<OpTypeSampledImage>(op);
        break;
      case spv::OpTypeArray:
        error = builder.create<OpTypeArray>(op);
        break;
      case spv::OpTypeRuntimeArray:
        error = builder.create<OpTypeRuntimeArray>(op);
        break;
      case spv::OpTypeStruct:
        error = builder.create<OpTypeStruct>(op);
        break;
      case spv::OpTypeOpaque:
        error = builder.create<OpTypeOpaque>(op);
        break;
      case spv::OpTypePointer:
        error = builder.create<OpTypePointer>(op);
        break;
      case spv::OpTypeFunction:
        error = builder.create<OpTypeFunction>(op);
        break;
      case spv::OpTypeEvent:
        error = builder.create<OpTypeEvent>(op);
        break;
      case spv::OpTypeDeviceEvent:
        error = builder.create<OpTypeDeviceEvent>(op);
        break;
      case spv::OpTypeReserveId:
        error = builder.create<OpTypeReserveId>(op);
        break;
      case spv::OpTypeQueue:
        error = builder.create<OpTypeQueue>(op);
        break;
      case spv::OpTypePipe:
        error = builder.create<OpTypePipe>(op);
        break;
      case spv::OpTypeForwardPointer:
        error = builder.create<OpTypeForwardPointer>(op);
        break;
      case spv::OpConstantTrue:
        error = builder.create<OpConstantTrue>(op);
        break;
      case spv::OpConstantFalse:
        error = builder.create<OpConstantFalse>(op);
        break;
      case spv::OpConstant:
        error = builder.create<OpConstant>(op);
        break;
      case spv::OpConstantComposite:
        error = builder.create<OpConstantComposite>(op);
        break;
      case spv::OpConstantSampler:
        error = builder.create<OpConstantSampler>(op);
        break;
      case spv::OpConstantNull:
        error = builder.create<OpConstantNull>(op);
        break;
      case spv::OpSpecConstantTrue:
        error = builder.create<OpSpecConstantTrue>(op);
        break;
      case spv::OpSpecConstantFalse:
        error = builder.create<OpSpecConstantFalse>(op);
        break;
      case spv::OpSpecConstant:
        error = builder.create<OpSpecConstant>(op);
        break;
      case spv::OpSpecConstantComposite:
        error = builder.create<OpSpecConstantComposite>(op);
        break;
      case spv::OpSpecConstantOp:
        error = builder.create<OpSpecConstantOp>(op);
        break;
      case spv::OpFunction:
        error = builder.create<OpFunction>(op);
        break;
      case spv::OpFunctionParameter:
        error = builder.create<OpFunctionParameter>(op);
        break;
      case spv::OpFunctionEnd:
        error = builder.create<OpFunctionEnd>(op);
        // Populate all the incoming edges for the Phi nodes we have generated
        IPStack.push_back(builder.getIRBuilder().saveIP());
        for (auto &IterPos : Phis) {
          builder.getIRBuilder().restoreIP(IterPos.second);
          const OpCode *phi = &IterPos.first;
          SPIRV_LL_ASSERT(phi->opCode() == spv::OpPhi,
                          "Bad phi instruction found while populating edges!");
          builder.populatePhi(*phi);
        }
        Phis.clear();
        IPStack.pop_back();
        break;
      case spv::OpFunctionCall:
        error = builder.create<OpFunctionCall>(op);
        break;
      case spv::OpVariable:
        error = builder.create<OpVariable>(op);
        break;
      case spv::OpImageTexelPointer:
        error = builder.create<OpImageTexelPointer>(op);
        break;
      case spv::OpLoad:
        error = builder.create<OpLoad>(op);
        break;
      case spv::OpStore:
        error = builder.create<OpStore>(op);
        break;
      case spv::OpCopyMemory:
        error = builder.create<OpCopyMemory>(op);
        break;
      case spv::OpCopyMemorySized:
        error = builder.create<OpCopyMemorySized>(op);
        break;
      case spv::OpAccessChain:
        error = builder.create<OpAccessChain>(op);
        break;
      case spv::OpInBoundsAccessChain:
        error = builder.create<OpInBoundsAccessChain>(op);
        break;
      case spv::OpPtrAccessChain:
        error = builder.create<OpPtrAccessChain>(op);
        break;
      case spv::OpArrayLength:
        error = builder.create<OpArrayLength>(op);
        break;
      case spv::OpGenericPtrMemSemantics:
        error = builder.create<OpGenericPtrMemSemantics>(op);
        break;
      case spv::OpInBoundsPtrAccessChain:
        error = builder.create<OpInBoundsPtrAccessChain>(op);
        break;
      case spv::OpDecorate:
        error = builder.create<OpDecorate>(op);
        break;
      case spv::OpMemberDecorate:
        error = builder.create<OpMemberDecorate>(op);
        break;
      case spv::OpDecorationGroup:
        error = builder.create<OpDecorationGroup>(op);
        break;
      case spv::OpGroupDecorate:
        error = builder.create<OpGroupDecorate>(op);
        break;
      case spv::OpGroupMemberDecorate:
        error = builder.create<OpGroupMemberDecorate>(op);
        break;
      case spv::OpVectorExtractDynamic:
        error = builder.create<OpVectorExtractDynamic>(op);
        break;
      case spv::OpVectorInsertDynamic:
        error = builder.create<OpVectorInsertDynamic>(op);
        break;
      case spv::OpVectorShuffle:
        error = builder.create<OpVectorShuffle>(op);
        break;
      case spv::OpCompositeConstruct:
        error = builder.create<OpCompositeConstruct>(op);
        break;
      case spv::OpCompositeExtract:
        error = builder.create<OpCompositeExtract>(op);
        break;
      case spv::OpCompositeInsert:
        error = builder.create<OpCompositeInsert>(op);
        break;
      case spv::OpCopyObject:
        error = builder.create<OpCopyObject>(op);
        break;
      case spv::OpTranspose:
        error = builder.create<OpTranspose>(op);
        break;
      case spv::OpSampledImage:
        error = builder.create<OpSampledImage>(op);
        break;
      case spv::OpImageSampleImplicitLod:
        error = builder.create<OpImageSampleImplicitLod>(op);
        break;
      case spv::OpImageSampleExplicitLod:
        error = builder.create<OpImageSampleExplicitLod>(op);
        break;
      case spv::OpImageSampleDrefImplicitLod:
        error = builder.create<OpImageSampleDrefImplicitLod>(op);
        break;
      case spv::OpImageSampleDrefExplicitLod:
        error = builder.create<OpImageSampleDrefExplicitLod>(op);
        break;
      case spv::OpImageSampleProjImplicitLod:
        error = builder.create<OpImageSampleProjImplicitLod>(op);
        break;
      case spv::OpImageSampleProjExplicitLod:
        error = builder.create<OpImageSampleProjExplicitLod>(op);
        break;
      case spv::OpImageSampleProjDrefImplicitLod:
        error = builder.create<OpImageSampleProjDrefImplicitLod>(op);
        break;
      case spv::OpImageSampleProjDrefExplicitLod:
        error = builder.create<OpImageSampleProjDrefExplicitLod>(op);
        break;
      case spv::OpImageFetch:
        error = builder.create<OpImageFetch>(op);
        break;
      case spv::OpImageGather:
        error = builder.create<OpImageGather>(op);
        break;
      case spv::OpImageDrefGather:
        error = builder.create<OpImageDrefGather>(op);
        break;
      case spv::OpImageRead:
        error = builder.create<OpImageRead>(op);
        break;
      case spv::OpImageWrite:
        error = builder.create<OpImageWrite>(op);
        break;
      case spv::OpImage:
        error = builder.create<OpImage>(op);
        break;
      case spv::OpImageQueryFormat:
        error = builder.create<OpImageQueryFormat>(op);
        break;
      case spv::OpImageQueryOrder:
        error = builder.create<OpImageQueryOrder>(op);
        break;
      case spv::OpImageQuerySizeLod:
        error = builder.create<OpImageQuerySizeLod>(op);
        break;
      case spv::OpImageQuerySize:
        error = builder.create<OpImageQuerySize>(op);
        break;
      case spv::OpImageQueryLod:
        error = builder.create<OpImageQueryLod>(op);
        break;
      case spv::OpImageQueryLevels:
        error = builder.create<OpImageQueryLevels>(op);
        break;
      case spv::OpImageQuerySamples:
        error = builder.create<OpImageQuerySamples>(op);
        break;
      case spv::OpConvertFToU:
        error = builder.create<OpConvertFToU>(op);
        break;
      case spv::OpConvertFToS:
        error = builder.create<OpConvertFToS>(op);
        break;
      case spv::OpConvertSToF:
        error = builder.create<OpConvertSToF>(op);
        break;
      case spv::OpConvertUToF:
        error = builder.create<OpConvertUToF>(op);
        break;
      case spv::OpUConvert:
        error = builder.create<OpUConvert>(op);
        break;
      case spv::OpSConvert:
        error = builder.create<OpSConvert>(op);
        break;
      case spv::OpFConvert:
        error = builder.create<OpFConvert>(op);
        break;
      case spv::OpQuantizeToF16:
        error = builder.create<OpQuantizeToF16>(op);
        break;
      case spv::OpConvertPtrToU:
        error = builder.create<OpConvertPtrToU>(op);
        break;
      case spv::OpSatConvertSToU:
        error = builder.create<OpSatConvertSToU>(op);
        break;
      case spv::OpSatConvertUToS:
        error = builder.create<OpSatConvertUToS>(op);
        break;
      case spv::OpConvertUToPtr:
        error = builder.create<OpConvertUToPtr>(op);
        break;
      case spv::OpPtrCastToGeneric:
        error = builder.create<OpPtrCastToGeneric>(op);
        break;
      case spv::OpGenericCastToPtr:
        error = builder.create<OpGenericCastToPtr>(op);
        break;
      case spv::OpGenericCastToPtrExplicit:
        error = builder.create<OpGenericCastToPtrExplicit>(op);
        break;
      case spv::OpBitcast:
        error = builder.create<OpBitcast>(op);
        break;
      case spv::OpSNegate:
        error = builder.create<OpSNegate>(op);
        break;
      case spv::OpFNegate:
        error = builder.create<OpFNegate>(op);
        break;
      case spv::OpIAdd:
        error = builder.create<OpIAdd>(op);
        break;
      case spv::OpFAdd:
        error = builder.create<OpFAdd>(op);
        break;
      case spv::OpISub:
        error = builder.create<OpISub>(op);
        break;
      case spv::OpFSub:
        error = builder.create<OpFSub>(op);
        break;
      case spv::OpIMul:
        error = builder.create<OpIMul>(op);
        break;
      case spv::OpFMul:
        error = builder.create<OpFMul>(op);
        break;
      case spv::OpUDiv:
        error = builder.create<OpUDiv>(op);
        break;
      case spv::OpSDiv:
        error = builder.create<OpSDiv>(op);
        break;
      case spv::OpFDiv:
        error = builder.create<OpFDiv>(op);
        break;
      case spv::OpUMod:
        error = builder.create<OpUMod>(op);
        break;
      case spv::OpSRem:
        error = builder.create<OpSRem>(op);
        break;
      case spv::OpSMod:
        error = builder.create<OpSMod>(op);
        break;
      case spv::OpFRem:
        error = builder.create<OpFRem>(op);
        break;
      case spv::OpFMod:
        error = builder.create<OpFMod>(op);
        break;
      case spv::OpVectorTimesScalar:
        error = builder.create<OpVectorTimesScalar>(op);
        break;
      case spv::OpMatrixTimesScalar:
        error = builder.create<OpMatrixTimesScalar>(op);
        break;
      case spv::OpVectorTimesMatrix:
        error = builder.create<OpVectorTimesMatrix>(op);
        break;
      case spv::OpMatrixTimesVector:
        error = builder.create<OpMatrixTimesVector>(op);
        break;
      case spv::OpMatrixTimesMatrix:
        error = builder.create<OpMatrixTimesMatrix>(op);
        break;
      case spv::OpOuterProduct:
        error = builder.create<OpOuterProduct>(op);
        break;
      case spv::OpDot:
        error = builder.create<OpDot>(op);
        break;
      case spv::OpIAddCarry:
        error = builder.create<OpIAddCarry>(op);
        break;
      case spv::OpISubBorrow:
        error = builder.create<OpISubBorrow>(op);
        break;
      case spv::OpUMulExtended:
        error = builder.create<OpUMulExtended>(op);
        break;
      case spv::OpSMulExtended:
        error = builder.create<OpSMulExtended>(op);
        break;
      case spv::OpAny:
        error = builder.create<OpAny>(op);
        break;
      case spv::OpAll:
        error = builder.create<OpAll>(op);
        break;
      case spv::OpIsNan:
        error = builder.create<OpIsNan>(op);
        break;
      case spv::OpIsInf:
        error = builder.create<OpIsInf>(op);
        break;
      case spv::OpIsFinite:
        error = builder.create<OpIsFinite>(op);
        break;
      case spv::OpIsNormal:
        error = builder.create<OpIsNormal>(op);
        break;
      case spv::OpSignBitSet:
        error = builder.create<OpSignBitSet>(op);
        break;
      case spv::OpLessOrGreater:
        error = builder.create<OpLessOrGreater>(op);
        break;
      case spv::OpOrdered:
        error = builder.create<OpOrdered>(op);
        break;
      case spv::OpUnordered:
        error = builder.create<OpUnordered>(op);
        break;
      case spv::OpLogicalEqual:
        error = builder.create<OpLogicalEqual>(op);
        break;
      case spv::OpLogicalNotEqual:
        error = builder.create<OpLogicalNotEqual>(op);
        break;
      case spv::OpLogicalOr:
        error = builder.create<OpLogicalOr>(op);
        break;
      case spv::OpLogicalAnd:
        error = builder.create<OpLogicalAnd>(op);
        break;
      case spv::OpLogicalNot:
        error = builder.create<OpLogicalNot>(op);
        break;
      case spv::OpSelect:
        error = builder.create<OpSelect>(op);
        break;
      case spv::OpIEqual:
        error = builder.create<OpIEqual>(op);
        break;
      case spv::OpINotEqual:
        error = builder.create<OpINotEqual>(op);
        break;
      case spv::OpUGreaterThan:
        error = builder.create<OpUGreaterThan>(op);
        break;
      case spv::OpSGreaterThan:
        error = builder.create<OpSGreaterThan>(op);
        break;
      case spv::OpUGreaterThanEqual:
        error = builder.create<OpUGreaterThanEqual>(op);
        break;
      case spv::OpSGreaterThanEqual:
        error = builder.create<OpSGreaterThanEqual>(op);
        break;
      case spv::OpULessThan:
        error = builder.create<OpULessThan>(op);
        break;
      case spv::OpSLessThan:
        error = builder.create<OpSLessThan>(op);
        break;
      case spv::OpULessThanEqual:
        error = builder.create<OpULessThanEqual>(op);
        break;
      case spv::OpSLessThanEqual:
        error = builder.create<OpSLessThanEqual>(op);
        break;
      case spv::OpFOrdEqual:
        error = builder.create<OpFOrdEqual>(op);
        break;
      case spv::OpFUnordEqual:
        error = builder.create<OpFUnordEqual>(op);
        break;
      case spv::OpFOrdNotEqual:
        error = builder.create<OpFOrdNotEqual>(op);
        break;
      case spv::OpFUnordNotEqual:
        error = builder.create<OpFUnordNotEqual>(op);
        break;
      case spv::OpFOrdLessThan:
        error = builder.create<OpFOrdLessThan>(op);
        break;
      case spv::OpFUnordLessThan:
        error = builder.create<OpFUnordLessThan>(op);
        break;
      case spv::OpFOrdGreaterThan:
        error = builder.create<OpFOrdGreaterThan>(op);
        break;
      case spv::OpFUnordGreaterThan:
        error = builder.create<OpFUnordGreaterThan>(op);
        break;
      case spv::OpFOrdLessThanEqual:
        error = builder.create<OpFOrdLessThanEqual>(op);
        break;
      case spv::OpFUnordLessThanEqual:
        error = builder.create<OpFUnordLessThanEqual>(op);
        break;
      case spv::OpFOrdGreaterThanEqual:
        error = builder.create<OpFOrdGreaterThanEqual>(op);
        break;
      case spv::OpFUnordGreaterThanEqual:
        error = builder.create<OpFUnordGreaterThanEqual>(op);
        break;
      case spv::OpShiftRightLogical:
        error = builder.create<OpShiftRightLogical>(op);
        break;
      case spv::OpShiftRightArithmetic:
        error = builder.create<OpShiftRightArithmetic>(op);
        break;
      case spv::OpShiftLeftLogical:
        error = builder.create<OpShiftLeftLogical>(op);
        break;
      case spv::OpBitwiseOr:
        error = builder.create<OpBitwiseOr>(op);
        break;
      case spv::OpBitwiseXor:
        error = builder.create<OpBitwiseXor>(op);
        break;
      case spv::OpBitwiseAnd:
        error = builder.create<OpBitwiseAnd>(op);
        break;
      case spv::OpNot:
        error = builder.create<OpNot>(op);
        break;
      case spv::OpBitFieldInsert:
        error = builder.create<OpBitFieldInsert>(op);
        break;
      case spv::OpBitFieldSExtract:
        error = builder.create<OpBitFieldSExtract>(op);
        break;
      case spv::OpBitFieldUExtract:
        error = builder.create<OpBitFieldUExtract>(op);
        break;
      case spv::OpBitReverse:
        error = builder.create<OpBitReverse>(op);
        break;
      case spv::OpBitCount:
        error = builder.create<OpBitCount>(op);
        break;
      case spv::OpDPdx:
        error = builder.create<OpDPdx>(op);
        break;
      case spv::OpDPdy:
        error = builder.create<OpDPdy>(op);
        break;
      case spv::OpFwidth:
        error = builder.create<OpFwidth>(op);
        break;
      case spv::OpDPdxFine:
        error = builder.create<OpDPdxFine>(op);
        break;
      case spv::OpDPdyFine:
        error = builder.create<OpDPdyFine>(op);
        break;
      case spv::OpFwidthFine:
        error = builder.create<OpFwidthFine>(op);
        break;
      case spv::OpDPdxCoarse:
        error = builder.create<OpDPdxCoarse>(op);
        break;
      case spv::OpDPdyCoarse:
        error = builder.create<OpDPdyCoarse>(op);
        break;
      case spv::OpFwidthCoarse:
        error = builder.create<OpFwidthCoarse>(op);
        break;
      case spv::OpEmitVertex:
        error = builder.create<OpEmitVertex>(op);
        break;
      case spv::OpEndPrimitive:
        error = builder.create<OpEndPrimitive>(op);
        break;
      case spv::OpEmitStreamVertex:
        error = builder.create<OpEmitStreamVertex>(op);
        break;
      case spv::OpEndStreamPrimitive:
        error = builder.create<OpEndStreamPrimitive>(op);
        break;
      case spv::OpControlBarrier:
        error = builder.create<OpControlBarrier>(op);
        break;
      case spv::OpMemoryBarrier:
        error = builder.create<OpMemoryBarrier>(op);
        break;
      case spv::OpAtomicLoad:
        error = builder.create<OpAtomicLoad>(op);
        break;
      case spv::OpAtomicStore:
        error = builder.create<OpAtomicStore>(op);
        break;
      case spv::OpAtomicExchange:
        error = builder.create<OpAtomicExchange>(op);
        break;
      case spv::OpAtomicCompareExchange:
        error = builder.create<OpAtomicCompareExchange>(op);
        break;
      case spv::OpAtomicCompareExchangeWeak:
        error = builder.create<OpAtomicCompareExchangeWeak>(op);
        break;
      case spv::OpAtomicIIncrement:
        error = builder.create<OpAtomicIIncrement>(op);
        break;
      case spv::OpAtomicIDecrement:
        error = builder.create<OpAtomicIDecrement>(op);
        break;
      case spv::OpAtomicIAdd:
        error = builder.create<OpAtomicIAdd>(op);
        break;
      case spv::OpAtomicISub:
        error = builder.create<OpAtomicISub>(op);
        break;
      case spv::OpAtomicSMin:
        error = builder.create<OpAtomicSMin>(op);
        break;
      case spv::OpAtomicUMin:
        error = builder.create<OpAtomicUMin>(op);
        break;
      case spv::OpAtomicSMax:
        error = builder.create<OpAtomicSMax>(op);
        break;
      case spv::OpAtomicUMax:
        error = builder.create<OpAtomicUMax>(op);
        break;
      case spv::OpAtomicFAddEXT:
        error = builder.create<OpAtomicFAddEXT>(op);
        break;
      case spv::OpAtomicFMinEXT:
        error = builder.create<OpAtomicFMinEXT>(op);
        break;
      case spv::OpAtomicFMaxEXT:
        error = builder.create<OpAtomicFMaxEXT>(op);
        break;
      case spv::OpAtomicAnd:
        error = builder.create<OpAtomicAnd>(op);
        break;
      case spv::OpAtomicOr:
        error = builder.create<OpAtomicOr>(op);
        break;
      case spv::OpAtomicXor:
        error = builder.create<OpAtomicXor>(op);
        break;
      case spv::OpPhi:
        error = builder.create<OpPhi>(op);
        Phis.push_back(std::make_pair(op, builder.getIRBuilder().saveIP()));
        break;
      case spv::OpLoopMerge:
        error = builder.create<OpLoopMerge>(op);
        break;
      case spv::OpSelectionMerge:
        error = builder.create<OpSelectionMerge>(op);
        break;
      case spv::OpLabel:
        error = builder.create<OpLabel>(op);
        break;
      case spv::OpBranch:
        error = builder.create<OpBranch>(op);
        break;
      case spv::OpBranchConditional:
        error = builder.create<OpBranchConditional>(op);
        break;
      case spv::OpSwitch:
        error = builder.create<OpSwitch>(op);
        break;
      case spv::OpKill:
        error = builder.create<OpKill>(op);
        break;
      case spv::OpReturn:
        error = builder.create<OpReturn>(op);
        break;
      case spv::OpReturnValue:
        error = builder.create<OpReturnValue>(op);
        break;
      case spv::OpUnreachable:
        error = builder.create<OpUnreachable>(op);
        break;
      case spv::OpLifetimeStart:
        error = builder.create<OpLifetimeStart>(op);
        break;
      case spv::OpLifetimeStop:
        error = builder.create<OpLifetimeStop>(op);
        break;
      case spv::OpGroupAsyncCopy:
        error = builder.create<OpGroupAsyncCopy>(op);
        break;
      case spv::OpGroupWaitEvents:
        error = builder.create<OpGroupWaitEvents>(op);
        break;
      case spv::OpGroupAll:
        error = builder.create<OpGroupAll>(op);
        break;
      case spv::OpGroupAny:
        error = builder.create<OpGroupAny>(op);
        break;
      case spv::OpGroupBroadcast:
        error = builder.create<OpGroupBroadcast>(op);
        break;
      case spv::OpGroupIAdd:
        error = builder.create<OpGroupIAdd>(op);
        break;
      case spv::OpGroupFAdd:
        error = builder.create<OpGroupFAdd>(op);
        break;
      case spv::OpGroupFMin:
        error = builder.create<OpGroupFMin>(op);
        break;
      case spv::OpGroupUMin:
        error = builder.create<OpGroupUMin>(op);
        break;
      case spv::OpGroupSMin:
        error = builder.create<OpGroupSMin>(op);
        break;
      case spv::OpGroupFMax:
        error = builder.create<OpGroupFMax>(op);
        break;
      case spv::OpGroupUMax:
        error = builder.create<OpGroupUMax>(op);
        break;
      case spv::OpGroupSMax:
        error = builder.create<OpGroupSMax>(op);
        break;
      case spv::OpGroupIMulKHR:
        error = builder.create<OpGroupIMulKHR>(op);
        break;
      case spv::OpGroupFMulKHR:
        error = builder.create<OpGroupFMulKHR>(op);
        break;
      case spv::OpGroupBitwiseAndKHR:
        error = builder.create<OpGroupBitwiseAndKHR>(op);
        break;
      case spv::OpGroupBitwiseOrKHR:
        error = builder.create<OpGroupBitwiseOrKHR>(op);
        break;
      case spv::OpGroupBitwiseXorKHR:
        error = builder.create<OpGroupBitwiseXorKHR>(op);
        break;
      case spv::OpGroupLogicalAndKHR:
        error = builder.create<OpGroupLogicalAndKHR>(op);
        break;
      case spv::OpGroupLogicalOrKHR:
        error = builder.create<OpGroupLogicalOrKHR>(op);
        break;
      case spv::OpGroupLogicalXorKHR:
        error = builder.create<OpGroupLogicalXorKHR>(op);
        break;
      case spv::OpSubgroupShuffleINTEL:
        error = builder.create<OpSubgroupShuffle>(op);
        break;
      case spv::OpSubgroupShuffleUpINTEL:
        error = builder.create<OpSubgroupShuffleUp>(op);
        break;
      case spv::OpSubgroupShuffleDownINTEL:
        error = builder.create<OpSubgroupShuffleDown>(op);
        break;
      case spv::OpSubgroupShuffleXorINTEL:
        error = builder.create<OpSubgroupShuffleXor>(op);
        break;
      case spv::OpReadPipe:
        error = builder.create<OpReadPipe>(op);
        break;
      case spv::OpWritePipe:
        error = builder.create<OpWritePipe>(op);
        break;
      case spv::OpReservedReadPipe:
        error = builder.create<OpReservedReadPipe>(op);
        break;
      case spv::OpReservedWritePipe:
        error = builder.create<OpReservedWritePipe>(op);
        break;
      case spv::OpReserveReadPipePackets:
        error = builder.create<OpReserveReadPipePackets>(op);
        break;
      case spv::OpReserveWritePipePackets:
        error = builder.create<OpReserveWritePipePackets>(op);
        break;
      case spv::OpCommitReadPipe:
        error = builder.create<OpCommitReadPipe>(op);
        break;
      case spv::OpCommitWritePipe:
        error = builder.create<OpCommitWritePipe>(op);
        break;
      case spv::OpIsValidReserveId:
        error = builder.create<OpIsValidReserveId>(op);
        break;
      case spv::OpGetNumPipePackets:
        error = builder.create<OpGetNumPipePackets>(op);
        break;
      case spv::OpGetMaxPipePackets:
        error = builder.create<OpGetMaxPipePackets>(op);
        break;
      case spv::OpGroupReserveReadPipePackets:
        error = builder.create<OpGroupReserveReadPipePackets>(op);
        break;
      case spv::OpGroupReserveWritePipePackets:
        error = builder.create<OpGroupReserveWritePipePackets>(op);
        break;
      case spv::OpGroupCommitReadPipe:
        error = builder.create<OpGroupCommitReadPipe>(op);
        break;
      case spv::OpGroupCommitWritePipe:
        error = builder.create<OpGroupCommitWritePipe>(op);
        break;
      case spv::OpEnqueueMarker:
        error = builder.create<OpEnqueueMarker>(op);
        break;
      case spv::OpEnqueueKernel:
        error = builder.create<OpEnqueueKernel>(op);
        break;
      case spv::OpGetKernelNDrangeSubGroupCount:
        error = builder.create<OpGetKernelNDrangeSubGroupCount>(op);
        break;
      case spv::OpGetKernelNDrangeMaxSubGroupSize:
        error = builder.create<OpGetKernelNDrangeMaxSubGroupSize>(op);
        break;
      case spv::OpGetKernelWorkGroupSize:
        error = builder.create<OpGetKernelWorkGroupSize>(op);
        break;
      case spv::OpGetKernelPreferredWorkGroupSizeMultiple:
        error = builder.create<OpGetKernelPreferredWorkGroupSizeMultiple>(op);
        break;
      case spv::OpRetainEvent:
        error = builder.create<OpRetainEvent>(op);
        break;
      case spv::OpReleaseEvent:
        error = builder.create<OpReleaseEvent>(op);
        break;
      case spv::OpCreateUserEvent:
        error = builder.create<OpCreateUserEvent>(op);
        break;
      case spv::OpIsValidEvent:
        error = builder.create<OpIsValidEvent>(op);
        break;
      case spv::OpSetUserEventStatus:
        error = builder.create<OpSetUserEventStatus>(op);
        break;
      case spv::OpCaptureEventProfilingInfo:
        error = builder.create<OpCaptureEventProfilingInfo>(op);
        break;
      case spv::OpGetDefaultQueue:
        error = builder.create<OpGetDefaultQueue>(op);
        break;
      case spv::OpBuildNDRange:
        error = builder.create<OpBuildNDRange>(op);
        break;
      case spv::OpGetKernelLocalSizeForSubgroupCount:
        error = builder.create<OpGetKernelLocalSizeForSubgroupCount>(op);
        break;
      case spv::OpGetKernelMaxNumSubgroups:
        error = builder.create<OpGetKernelMaxNumSubgroups>(op);
        break;
      case spv::OpImageSparseSampleImplicitLod:
        error = builder.create<OpImageSparseSampleImplicitLod>(op);
        break;
      case spv::OpImageSparseSampleExplicitLod:
        error = builder.create<OpImageSparseSampleExplicitLod>(op);
        break;
      case spv::OpImageSparseSampleDrefImplicitLod:
        error = builder.create<OpImageSparseSampleDrefImplicitLod>(op);
        break;
      case spv::OpImageSparseSampleDrefExplicitLod:
        error = builder.create<OpImageSparseSampleDrefExplicitLod>(op);
        break;
      case spv::OpImageSparseSampleProjImplicitLod:
        error = builder.create<OpImageSparseSampleProjImplicitLod>(op);
        break;
      case spv::OpImageSparseSampleProjExplicitLod:
        error = builder.create<OpImageSparseSampleProjExplicitLod>(op);
        break;
      case spv::OpImageSparseSampleProjDrefImplicitLod:
        error = builder.create<OpImageSparseSampleProjDrefImplicitLod>(op);
        break;
      case spv::OpImageSparseSampleProjDrefExplicitLod:
        error = builder.create<OpImageSparseSampleProjDrefExplicitLod>(op);
        break;
      case spv::OpImageSparseFetch:
        error = builder.create<OpImageSparseFetch>(op);
        break;
      case spv::OpImageSparseGather:
        error = builder.create<OpImageSparseGather>(op);
        break;
      case spv::OpImageSparseDrefGather:
        error = builder.create<OpImageSparseDrefGather>(op);
        break;
      case spv::OpImageSparseTexelsResident:
        error = builder.create<OpImageSparseTexelsResident>(op);
        break;
      case spv::OpNoLine:
        error = builder.create<OpNoLine>(op);
        break;
      case spv::OpAtomicFlagTestAndSet:
        error = builder.create<OpAtomicFlagTestAndSet>(op);
        break;
      case spv::OpAtomicFlagClear:
        error = builder.create<OpAtomicFlagClear>(op);
        break;
      case spv::OpImageSparseRead:
        error = builder.create<OpImageSparseRead>(op);
        break;
      case spv::OpAssumeTrueKHR:
        error = builder.create<OpAssumeTrueKHR>(op);
        break;
      case spv::OpExpectKHR:
        error = builder.create<OpExpectKHR>(op);
        break;
    }
    if (error && *error) {
      return cargo::make_unexpected(llvm::toString(std::move(*error)));
    }
  }

  if (auto err = builder.finishModuleProcessing()) {
    return cargo::make_unexpected(llvm::toString(std::move(err)));
  }

  return module;
}
