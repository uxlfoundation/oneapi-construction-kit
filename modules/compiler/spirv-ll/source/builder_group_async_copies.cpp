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

#include <spirv-ll/builder_group_async_copies.h>
#include <spirv-ll/module.h>

namespace spirv_ll {
struct GroupAsyncCopy2D2D : OpExtInst {
  GroupAsyncCopy2D2D(const OpCode &opc) : OpExtInst(opc) {}
  spv::Id Destination() const { return getOpExtInstOperand(0); }
  spv::Id DestinationOffset() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  spv::Id SourceOffset() const { return getOpExtInstOperand(3); }
  spv::Id NumBytesPerElement() const { return getOpExtInstOperand(4); }
  spv::Id NumElementsPerLine() const { return getOpExtInstOperand(5); }
  spv::Id NumLines() const { return getOpExtInstOperand(6); }
  spv::Id SourceLineLength() const { return getOpExtInstOperand(7); }
  spv::Id DestinationLineLength() const { return getOpExtInstOperand(8); }
  spv::Id Event() const { return getOpExtInstOperand(9); }
};

struct GroupAsyncCopy3D3D : OpExtInst {
  GroupAsyncCopy3D3D(const OpCode &opc) : OpExtInst(opc) {}
  spv::Id Destination() const { return getOpExtInstOperand(0); }
  spv::Id DestinationOffset() const { return getOpExtInstOperand(1); }
  spv::Id Source() const { return getOpExtInstOperand(2); }
  spv::Id SourceOffset() const { return getOpExtInstOperand(3); }
  spv::Id NumBytesPerElement() const { return getOpExtInstOperand(4); }
  spv::Id NumElementsPerLine() const { return getOpExtInstOperand(5); }
  spv::Id NumLines() const { return getOpExtInstOperand(6); }
  spv::Id NumPlanes() const { return getOpExtInstOperand(7); }
  spv::Id SourceLineLength() const { return getOpExtInstOperand(8); }
  spv::Id SourcePlaneArea() const { return getOpExtInstOperand(9); }
  spv::Id DestinationLineLength() const { return getOpExtInstOperand(10); }
  spv::Id DestinationPlaneArea() const { return getOpExtInstOperand(11); }
  spv::Id Event() const { return getOpExtInstOperand(12); }
};

template <>
llvm::Error
GroupAsyncCopiesBuilder::create<GroupAsyncCopiesBuilder::GroupAsyncCopy2D2D>(
    const OpExtInst &opc) {
  auto *op = module.create<spirv_ll::GroupAsyncCopy2D2D>(opc);

  llvm::Type *eventTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(eventTy);
  llvm::Value *dst = module.getValue(op->Destination());
  SPIRV_LL_ASSERT_PTR(dst);
  llvm::Value *dstOffset = module.getValue(op->DestinationOffset());
  SPIRV_LL_ASSERT_PTR(dstOffset);
  llvm::Value *src = module.getValue(op->Source());
  SPIRV_LL_ASSERT_PTR(src);
  llvm::Value *srcOffset = module.getValue(op->SourceOffset());
  SPIRV_LL_ASSERT_PTR(srcOffset);
  llvm::Value *numBytesPerElement = module.getValue(op->NumBytesPerElement());
  SPIRV_LL_ASSERT_PTR(numBytesPerElement);
  llvm::Value *numElementsPerLine = module.getValue(op->NumElementsPerLine());
  SPIRV_LL_ASSERT_PTR(numElementsPerLine);
  llvm::Value *numLines = module.getValue(op->NumLines());
  SPIRV_LL_ASSERT_PTR(numLines);
  llvm::Value *srcTotalLineLength = module.getValue(op->SourceLineLength());
  SPIRV_LL_ASSERT_PTR(srcTotalLineLength);
  llvm::Value *dstTotalLineLength =
      module.getValue(op->DestinationLineLength());
  SPIRV_LL_ASSERT_PTR(dstTotalLineLength);
  llvm::Value *event = module.getValue(op->Event());
  SPIRV_LL_ASSERT_PTR(event);

  auto dstAddrSpace = dst->getType()->getPointerAddressSpace();
  auto srcAddrSpace = src->getType()->getPointerAddressSpace();
  std::string mangledName;
  if (dstAddrSpace == 1 && srcAddrSpace == 3) {
    mangledName =
        "_Z26async_work_group_copy_2D2DPU3AS1vmPU3AS3Kvmmmmmm9ocl_event";
  } else if (dstAddrSpace == 3 && srcAddrSpace == 1) {
    mangledName =
        "_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event";
  } else {
    return makeStringError(
        "GroupAsyncCopy2D2D invalid storage class for Source and/or "
        "Destination");
  }

  llvm::Type *i8Ty = llvm::IntegerType::getInt8Ty(*module.context.llvmContext);

  llvm::CallInst *call = builder.createBuiltinCall(
      mangledName, eventTy,
      {
          builder.getIRBuilder().CreateBitCast(
              dst, llvm::PointerType::get(i8Ty, dstAddrSpace)),
          dstOffset,
          builder.getIRBuilder().CreateBitCast(
              src, llvm::PointerType::get(i8Ty, srcAddrSpace)),
          srcOffset,
          numBytesPerElement,
          numElementsPerLine,
          numLines,
          srcTotalLineLength,
          dstTotalLineLength,
          event,
      },
      /* convergent = */ true);
  module.addID(op->IdResult(), op, call);

  return llvm::Error::success();
}

template <>
llvm::Error
GroupAsyncCopiesBuilder::create<GroupAsyncCopiesBuilder::GroupAsyncCopy3D3D>(
    const OpExtInst &opc) {
  auto *op = module.create<spirv_ll::GroupAsyncCopy3D3D>(opc);

  llvm::Type *eventTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(eventTy);
  llvm::Value *dst = module.getValue(op->Destination());
  SPIRV_LL_ASSERT_PTR(dst);
  llvm::Value *dstOffset = module.getValue(op->DestinationOffset());
  SPIRV_LL_ASSERT_PTR(dstOffset);
  llvm::Value *src = module.getValue(op->Source());
  SPIRV_LL_ASSERT_PTR(src);
  llvm::Value *srcOffset = module.getValue(op->SourceOffset());
  SPIRV_LL_ASSERT_PTR(srcOffset);
  llvm::Value *numBytesPerElement = module.getValue(op->NumBytesPerElement());
  SPIRV_LL_ASSERT_PTR(numBytesPerElement);
  llvm::Value *numElementsPerLine = module.getValue(op->NumElementsPerLine());
  SPIRV_LL_ASSERT_PTR(numElementsPerLine);
  llvm::Value *numLines = module.getValue(op->NumLines());
  SPIRV_LL_ASSERT_PTR(numLines);
  llvm::Value *numPlanes = module.getValue(op->NumPlanes());
  SPIRV_LL_ASSERT_PTR(numPlanes);
  llvm::Value *srcTotalLineLength = module.getValue(op->SourceLineLength());
  SPIRV_LL_ASSERT_PTR(srcTotalLineLength);
  llvm::Value *srcTotalPlaneArea = module.getValue(op->SourcePlaneArea());
  SPIRV_LL_ASSERT_PTR(srcTotalPlaneArea);
  llvm::Value *dstTotalLineLength =
      module.getValue(op->DestinationLineLength());
  SPIRV_LL_ASSERT_PTR(dstTotalLineLength);
  llvm::Value *dstTotalPlaneArea = module.getValue(op->DestinationPlaneArea());
  SPIRV_LL_ASSERT_PTR(dstTotalPlaneArea);
  llvm::Value *event = module.getValue(op->Event());
  SPIRV_LL_ASSERT_PTR(event);

  auto dstAddrSpace = dst->getType()->getPointerAddressSpace();
  auto srcAddrSpace = src->getType()->getPointerAddressSpace();
  std::string mangledName;
  if (dstAddrSpace == 1 && srcAddrSpace == 3) {
    mangledName =
        "_Z26async_work_group_copy_3D3DPU3AS1vmPU3AS3Kvmmmmmmmmm9ocl_event";
  } else if (dstAddrSpace == 3 && srcAddrSpace == 1) {
    mangledName =
        "_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event";
  } else {
    return makeStringError(
        "GroupAsyncCopy3D3D invalid storage class for Source and/or "
        "Destination");
  }

  llvm::Type *i8Ty = llvm::IntegerType::getInt8Ty(*module.context.llvmContext);

  llvm::CallInst *call = builder.createBuiltinCall(
      mangledName, eventTy,
      {
          builder.getIRBuilder().CreateBitCast(
              dst, llvm::PointerType::get(i8Ty, dstAddrSpace)),
          dstOffset,
          builder.getIRBuilder().CreateBitCast(
              src, llvm::PointerType::get(i8Ty, srcAddrSpace)),
          srcOffset,
          numBytesPerElement,
          numElementsPerLine,
          numLines,
          numPlanes,
          srcTotalLineLength,
          srcTotalPlaneArea,
          dstTotalLineLength,
          dstTotalPlaneArea,
          event,
      },
      /* convergent = */ true);
  module.addID(op->IdResult(), op, call);

  return llvm::Error::success();
}

llvm::Error GroupAsyncCopiesBuilder::create(const OpExtInst &opc) {
  switch (opc.Instruction()) {
    case GroupAsyncCopy2D2D:
      return create<GroupAsyncCopy2D2D>(opc);
    case GroupAsyncCopy3D3D:
      return create<GroupAsyncCopy3D3D>(opc);
    default:
      return makeStringError(
          "Unrecognized Codeplay.GroupAsyncCopies extended instruction " +
          std::to_string(opc.Instruction()));
  }
}
}  // namespace spirv_ll
