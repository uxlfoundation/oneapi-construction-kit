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

#ifndef SPIRV_OPCODES_H_INCLUDED
#define SPIRV_OPCODES_H_INCLUDED

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <spirv-ll/assert.h>
#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.hpp>

#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>

namespace spirv_ll {
class iterator;

/// @brief Generic SPIR-V instruction base class.
class OpCode {
 public:
  /// @brief Constructor from iterator.
  OpCode(const spirv_ll::iterator &iter);

  /// @brief Copy constructor, can be used for creating the derived classes.
  OpCode(const OpCode &other, spv::Op code);

  /// @brief Return the instruction's opcode.
  uint16_t opCode() const;

  /// @brief Return the instruction's word count.
  uint16_t wordCount() const;

  /// @brief Return the value at a specified offset from the instruction.
  ///
  /// @param offset The offset in 32-bit words.
  ///
  /// @return Value at offset.
  uint32_t getValueAtOffset(int offset) const;

  /// @brief Return the value at a specified offset from the instruction.
  ///
  /// This overload also takes the number of words the value occupies and can be
  /// used to retrieve values up to 64 bits in length.
  ///
  /// @param offset The offset in 32-bit words.
  /// @param words The number of words the value occupies.
  ///
  /// @return Value at offset.
  uint64_t getValueAtOffset(int offset, int words) const;

  /// @brief Check if this opcode is a type.
  ///
  /// @return Returns true if this opcode is a type, false otherwise.
  bool isType() const;

  /// @brief Check if this opcode has a result ID.
  ///
  /// @return Returns true if this opcode has a result ID, false otherwise.
  bool hasResult() const;

  /// @brief ID for the underlying OpCode. spv::OpMax is used as an invalid ID.
  const spv::Op code;

 protected:
  /// @brief Pointer to the first word that represents this instruction.
  const uint32_t *data;
  /// @brief Flag to specify that `data` points to big endian data.
  const bool endianSwap;
};

template <class Op>
inline bool isa(const OpCode *op) {
  static_assert(std::is_base_of<OpCode, Op>::value, "invalid OpCode cast");
  return Op::ClassCode == op->code;
}

template <>
inline bool isa<OpCode>(const OpCode *) {
  return true;
}

template <class Op>
inline const Op *cast(const OpCode *op) {
  SPIRV_LL_ASSERT(isa<Op>(op), "invalid OpCode cast");
  return static_cast<const Op *>(op);
}

template <class Op>
inline const Op *dyn_cast(const OpCode *op) {
  if (!isa<Op>(op)) {
    return nullptr;
  }
  return static_cast<const Op *>(op);
}

class OpTypeVoid;
class OpTypeBool;
class OpTypeInt;
class OpTypeFloat;
class OpTypeVector;
class OpTypeMatrix;
class OpTypeImage;
class OpTypeSampler;
class OpTypeSampledImage;
class OpTypeArray;
class OpTypeRuntimeArray;
class OpTypeStruct;
class OpTypeOpaque;
class OpTypePointer;
class OpTypeFunction;
class OpTypeEvent;
class OpTypeDeviceEvent;
class OpTypeReserveId;
class OpTypeQueue;
class OpTypePipe;
class OpTypeForwardPointer;

/// @brief Specialization of `OpCode` for instructions which define types.
class OpType : public OpCode {
 public:
  /// @brief Construct from base `OpCode` object.
  OpType(const OpCode &other, spv::Op code) : OpCode(other, code) {}

  /// @brief Return the instruction's result ID. This is the SSA value number
  spv::Id IdResult() const { return getValueAtOffset(1); }

  /// @brief Returns true if this is `OpTypeVoid`.
  bool isVoidType() const { return code == spv::OpTypeVoid; }
  /// @brief Returns true if this is `OpTypeBool`.
  bool isBoolType() const { return code == spv::OpTypeBool; }
  /// @brief Returns true if this is `OpTypeInt`.
  bool isIntType() const { return code == spv::OpTypeInt; }
  /// @brief Returns true if this is `OpTypeFloat`.
  bool isFloatType() const { return code == spv::OpTypeFloat; }
  /// @brief Returns true if this is `OpTypeVector`.
  bool isVectorType() const { return code == spv::OpTypeVector; }
  /// @brief Returns true if this is `OpTypeMatrix`.
  bool isMatrixType() const { return code == spv::OpTypeMatrix; }
  /// @brief Returns true if this is `OpTypeImage`.
  bool isImageType() const { return code == spv::OpTypeImage; }
  /// @brief Returns true if this is `OpTypeSampler`.
  bool isSamplerType() const { return code == spv::OpTypeSampler; }
  /// @brief Returns true if this is `OpTypeSampledImage`.
  bool isSampledImageType() const { return code == spv::OpTypeSampledImage; }
  /// @brief Returns true if this is `OpTypeArray`.
  bool isArrayType() const { return code == spv::OpTypeArray; }
  /// @brief Returns true if this is `OpTypeRuntimeArray`.
  bool isRuntimeArrayType() const { return code == spv::OpTypeRuntimeArray; }
  /// @brief Returns true if this is `OpTypeStruct`.
  bool isStructType() const { return code == spv::OpTypeStruct; }
  /// @brief Returns true if this is `OpTypeOpaque`.
  bool isOpaqueType() const { return code == spv::OpTypeOpaque; }
  /// @brief Returns true if this is `OpTypePointer`.
  bool isPointerType() const { return code == spv::OpTypePointer; }
  /// @brief Returns true if this is `OpTypeFunction`.
  bool isFunctionType() const { return code == spv::OpTypeFunction; }
  /// @brief Returns true if this is `OpTypeEvent`.
  bool isEventType() const { return code == spv::OpTypeEvent; }
  /// @brief Returns true if this is `OpTypeDeviceEvent`.
  bool isDeviceEventType() const { return code == spv::OpTypeDeviceEvent; }
  /// @brief Returns true if this is `OpTypeReserveId`.
  bool isReserveIdType() const { return code == spv::OpTypeReserveId; }
  /// @brief Returns true if this is `OpTypeQueue`.
  bool isQueueType() const { return code == spv::OpTypeQueue; }
  /// @brief Returns true if this is `OpTypePipe`.
  bool isPipeType() const { return code == spv::OpTypePipe; }
  /// @brief Returns true if this is `OpTypeForwardPointer`.
  bool isForwardPointerType() const {
    return code == spv::OpTypeForwardPointer;
  }

  /// @brief Cast this to `OpTypeVoid`.
  const OpTypeVoid *getTypeVoid() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeVoid,
                    "invalid cast to unrelated OpTypeVoid");
    return cast<OpTypeVoid>(this);
  }
  /// @brief Cast this to `OpTypeBool`.
  const OpTypeBool *getTypeBool() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeBool,
                    "invalid cast to unrelated OpTypeBool");
    return cast<OpTypeBool>(this);
  }
  /// @brief Cast this to `OpTypeInt`.
  const OpTypeInt *getTypeInt() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeInt,
                    "invalid cast to unrelated OpTypeInt");
    return cast<OpTypeInt>(this);
  }
  /// @brief Cast this to `OpTypeFloat`.
  const OpTypeFloat *getTypeFloat() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeFloat,
                    "invalid cast to unrelated OpTypeFloat");
    return cast<OpTypeFloat>(this);
  }
  /// @brief Cast this to `OpTypeVector`.
  const OpTypeVector *getTypeVector() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeVector,
                    "invalid cast to unrelated OpTypeVector");
    return cast<OpTypeVector>(this);
  }
  /// @brief Cast this to `OpTypeMatrix`.
  const OpTypeMatrix *getTypeMatrix() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeMatrix,
                    "invalid cast to unrelated OpTypeMatrix");
    return cast<OpTypeMatrix>(this);
  }
  /// @brief Cast this to `OpTypeImage`.
  const OpTypeImage *getTypeImage() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeImage,
                    "invalid cast to unrelated OpTypeImage");
    return cast<OpTypeImage>(this);
  }
  /// @brief Cast this to `OpTypeSampler`.
  const OpTypeSampler *getTypeSampler() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeSampler,
                    "invalid cast to unrelated OpTypeSampler");
    return cast<OpTypeSampler>(this);
  }
  /// @brief Cast this to `OpTypeSampledImage`.
  const OpTypeSampledImage *getTypeSampledImage() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeSampledImage,
                    "invalid cast to unrelated OpTypeSampledImage");
    return cast<OpTypeSampledImage>(this);
  }
  /// @brief Cast this to `OpTypeArray`.
  const OpTypeArray *getTypeArray() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeArray,
                    "invalid cast to unrelated OpTypeArray");
    return cast<OpTypeArray>(this);
  }
  /// @brief Cast this to `OpTypeRuntimeArray`.
  const OpTypeRuntimeArray *getTypeRuntimeArray() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeRuntimeArray,
                    "invalid cast to unrelated OpTypeRuntimeArray");
    return cast<OpTypeRuntimeArray>(this);
  }
  /// @brief Cast this to `OpTypeStruct`.
  const OpTypeStruct *getTypeStruct() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeStruct,
                    "invalid cast to unrelated OpTypeStruct");
    return cast<OpTypeStruct>(this);
  }
  /// @brief Cast this to `OpTypeOpaque`.
  const OpTypeOpaque *getTypeOpaque() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeOpaque,
                    "invalid cast to unrelated OpTypeOpaque");
    return cast<OpTypeOpaque>(this);
  }
  /// @brief Cast this to `OpTypePointer`.
  const OpTypePointer *getTypePointer() const {
    SPIRV_LL_ASSERT(code == spv::OpTypePointer,
                    "invalid cast to unrelated OpTypePointer");
    return cast<OpTypePointer>(this);
  }
  /// @brief Cast this to `OpTypeFunction`.
  const OpTypeFunction *getTypeFunction() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeFunction,
                    "invalid cast to unrelated OpTypeFunction");
    return cast<OpTypeFunction>(this);
  }
  /// @brief Cast this to `OpTypeEvent`.
  const OpTypeEvent *getTypeEvent() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeEvent,
                    "invalid cast to unrelated OpTypeEvent");
    return cast<OpTypeEvent>(this);
  }
  /// @brief Cast this to `OpTypeDeviceEvent`.
  const OpTypeDeviceEvent *getTypeDeviceEvent() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeDeviceEvent,
                    "invalid cast to unrelated OpTypeDeviceEvent");
    return cast<OpTypeDeviceEvent>(this);
  }
  /// @brief Cast this to `OpTypeReserveId`.
  const OpTypeReserveId *getTypeReserveId() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeReserveId,
                    "invalid cast to unrelated OpTypeReserveId");
    return cast<OpTypeReserveId>(this);
  }
  /// @brief Cast this to `OpTypeQueue`.
  const OpTypeQueue *getTypeQueue() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeQueue,
                    "invalid cast to unrelated OpTypeQueue");
    return cast<OpTypeQueue>(this);
  }
  /// @brief Cast this to `OpTypePipe`.
  const OpTypePipe *getTypePipe() const {
    SPIRV_LL_ASSERT(code == spv::OpTypePipe,
                    "invalid cast to unrelated OpTypePipe");
    return cast<OpTypePipe>(this);
  }
  /// @brief Cast this to `OpTypeForwardPointer`.
  const OpTypeForwardPointer *getTypeForwardPointer() const {
    SPIRV_LL_ASSERT(code == spv::OpTypeForwardPointer,
                    "invalid cast to unrelated OpTypeForwardPointer");
    return cast<OpTypeForwardPointer>(this);
  }
};

template <>
inline const OpType *cast(const OpCode *op) {
  SPIRV_LL_ASSERT(op->isType(), "spirv-ll invalid cast to OpType");
  return static_cast<const OpType *>(op);
}

template <>
inline const OpType *dyn_cast(const OpCode *op) {
  if (!op->isType()) {
    return nullptr;
  }
  return cast<OpType>(op);
}

/// @brief Specialization of `OpCode` for instructions have a result ID.
class OpResult : public OpCode {
 public:
  /// @brief Constructor from base `OpCode` object.
  OpResult(OpCode const &other, spv::Op code) : OpCode(other, code) {}

  /// @brief Return the instruction's result type ID.
  spv::Id IdResultType() const;

  /// @brief Return the instruction's result ID.
  spv::Id IdResult() const;
};

template <>
inline const OpResult *cast(const OpCode *op) {
  SPIRV_LL_ASSERT(op->hasResult(), "spirv-ll invalid cast to OpResult");
  return static_cast<const OpResult *>(op);
}

template <>
inline const OpResult *dyn_cast(const OpCode *op) {
  if (!op->hasResult()) {
    return nullptr;
  }
  return cast<OpResult>(op);
}

/// @brief Specialization of `OpCode` for decorate instructions.
///
/// This allows for a unified decoration system that can accommodate both
/// OpDecorate and OpMemberDecorate.
class OpDecorateBase : public OpCode {
 public:
  /// @brief Constructor from base `OpCode` object.
  OpDecorateBase(OpCode const &other, spv::Op code) : OpCode(other, code) {}

  /// @brief Return the instruction's decoration operand.
  spv::Decoration getDecoration() const;
};

template <>
inline const OpDecorateBase *cast(const OpCode *op) {
  SPIRV_LL_ASSERT(
      op->code == spv::OpDecorate || op->code == spv::OpMemberDecorate,
      "spirv-ll invalid cast");
  return static_cast<const OpDecorateBase *>(op);
}

class OpNop : public OpCode {
 public:
  OpNop(OpCode const &other) : OpCode(other, spv::OpNop) {}
  static const spv::Op ClassCode = spv::OpNop;
};

class OpUndef : public OpResult {
 public:
  OpUndef(OpCode const &other) : OpResult(other, spv::OpUndef) {}
  static const spv::Op ClassCode = spv::OpUndef;
};

class OpSourceContinued : public OpCode {
 public:
  OpSourceContinued(OpCode const &other)
      : OpCode(other, spv::OpSourceContinued) {}
  llvm::StringRef ContinuedSource() const;
  static const spv::Op ClassCode = spv::OpSourceContinued;
};

class OpSource : public OpCode {
 public:
  OpSource(OpCode const &other) : OpCode(other, spv::OpSource) {}
  spv::SourceLanguage SourceLanguage() const;
  uint32_t Version() const;
  spv::Id File() const;
  llvm::StringRef Source() const;
  static const spv::Op ClassCode = spv::OpSource;
};

class OpSourceExtension : public OpCode {
 public:
  OpSourceExtension(OpCode const &other)
      : OpCode(other, spv::OpSourceExtension) {}
  llvm::StringRef Extension() const;
  static const spv::Op ClassCode = spv::OpSourceExtension;
};

class OpName : public OpCode {
 public:
  OpName(OpCode const &other) : OpCode(other, spv::OpName) {}
  spv::Id Target() const;
  llvm::StringRef Name() const;
  static const spv::Op ClassCode = spv::OpName;
};

class OpMemberName : public OpCode {
 public:
  OpMemberName(OpCode const &other) : OpCode(other, spv::OpMemberName) {}
  spv::Id Type() const;
  uint32_t Member() const;
  llvm::StringRef Name() const;
  static const spv::Op ClassCode = spv::OpMemberName;
};

class OpString : public OpCode {
 public:
  OpString(OpCode const &other) : OpCode(other, spv::OpString) {}
  spv::Id IdResult() const;
  llvm::StringRef String() const;
  static const spv::Op ClassCode = spv::OpString;
};

class OpLine : public OpCode {
 public:
  OpLine(OpCode const &other) : OpCode(other, spv::OpLine) {}
  spv::Id File() const;
  uint32_t Line() const;
  uint32_t Column() const;
  static const spv::Op ClassCode = spv::OpLine;
};

class OpExtension : public OpCode {
 public:
  OpExtension(OpCode const &other) : OpCode(other, spv::OpExtension) {}
  llvm::StringRef Name() const;
  static const spv::Op ClassCode = spv::OpExtension;
};

class OpExtInstImport : public OpCode {
 public:
  OpExtInstImport(OpCode const &other) : OpCode(other, spv::OpExtInstImport) {}
  spv::Id IdResult() const;
  llvm::StringRef Name() const;
  static const spv::Op ClassCode = spv::OpExtInstImport;
};

class OpExtInst : public OpResult {
 public:
  OpExtInst(OpCode const &other) : OpResult(other, spv::OpExtInst) {}
  spv::Id Set() const;
  uint32_t Instruction() const;
  llvm::SmallVector<spv::Id, 8> Operands() const;
  static const spv::Op ClassCode = spv::OpExtInst;
};

class OpMemoryModel : public OpCode {
 public:
  OpMemoryModel(OpCode const &other) : OpCode(other, spv::OpMemoryModel) {}
  spv::AddressingModel AddressingModel() const;
  spv::MemoryModel MemoryModel() const;
  static const spv::Op ClassCode = spv::OpMemoryModel;
};

class OpEntryPoint : public OpCode {
 public:
  OpEntryPoint(OpCode const &other) : OpCode(other, spv::OpEntryPoint) {}
  spv::ExecutionModel ExecutionModel() const;
  spv::Id EntryPoint() const;
  llvm::StringRef Name() const;
  llvm::SmallVector<spv::Id, 8> Interface() const;
  static const spv::Op ClassCode = spv::OpEntryPoint;
};

class OpExecutionMode : public OpCode {
 public:
  OpExecutionMode(OpCode const &other) : OpCode(other, spv::OpExecutionMode) {}
  spv::Id EntryPoint() const;
  spv::ExecutionMode Mode() const;
  static const spv::Op ClassCode = spv::OpExecutionMode;
};

class OpCapability : public OpCode {
 public:
  OpCapability(OpCode const &other) : OpCode(other, spv::OpCapability) {}
  spv::Capability Capability() const;
  static const spv::Op ClassCode = spv::OpCapability;
};

class OpTypeVoid : public OpType {
 public:
  OpTypeVoid(OpCode const &other) : OpType(other, spv::OpTypeVoid) {}
  static const spv::Op ClassCode = spv::OpTypeVoid;
};

class OpTypeBool : public OpType {
 public:
  OpTypeBool(OpCode const &other) : OpType(other, spv::OpTypeBool) {}
  static const spv::Op ClassCode = spv::OpTypeBool;
};

class OpTypeInt : public OpType {
 public:
  OpTypeInt(OpCode const &other) : OpType(other, spv::OpTypeInt) {}
  uint32_t Width() const;
  uint32_t Signedness() const;
  static const spv::Op ClassCode = spv::OpTypeInt;
};

class OpTypeFloat : public OpType {
 public:
  OpTypeFloat(OpCode const &other) : OpType(other, spv::OpTypeFloat) {}
  uint32_t Width() const;
  static const spv::Op ClassCode = spv::OpTypeFloat;
};

class OpTypeVector : public OpType {
 public:
  OpTypeVector(OpCode const &other) : OpType(other, spv::OpTypeVector) {}
  spv::Id ComponentType() const;
  uint32_t ComponentCount() const;
  static const spv::Op ClassCode = spv::OpTypeVector;
};

class OpTypeMatrix : public OpType {
 public:
  OpTypeMatrix(OpCode const &other) : OpType(other, spv::OpTypeMatrix) {}
  spv::Id ColumnType() const;
  uint32_t ColumnCount() const;
  static const spv::Op ClassCode = spv::OpTypeMatrix;
};

class OpTypeImage : public OpType {
 public:
  OpTypeImage(OpCode const &other) : OpType(other, spv::OpTypeImage) {}
  spv::Id SampledType() const;
  spv::Dim Dim() const;
  uint32_t Depth() const;
  uint32_t Arrayed() const;
  uint32_t MS() const;
  uint32_t Sampled() const;
  spv::ImageFormat ImageFormat() const;
  spv::AccessQualifier AccessQualifier() const;
  static const spv::Op ClassCode = spv::OpTypeImage;
};

class OpTypeSampler : public OpType {
 public:
  OpTypeSampler(OpCode const &other) : OpType(other, spv::OpTypeSampler) {}
  static const spv::Op ClassCode = spv::OpTypeSampler;
};

class OpTypeSampledImage : public OpType {
 public:
  OpTypeSampledImage(OpCode const &other)
      : OpType(other, spv::OpTypeSampledImage) {}
  spv::Id ImageType() const;
  static const spv::Op ClassCode = spv::OpTypeSampledImage;
};

class OpTypeArray : public OpType {
 public:
  OpTypeArray(OpCode const &other) : OpType(other, spv::OpTypeArray) {}
  spv::Id ElementType() const;
  spv::Id Length() const;
  static const spv::Op ClassCode = spv::OpTypeArray;
};

class OpTypeRuntimeArray : public OpType {
 public:
  OpTypeRuntimeArray(OpCode const &other)
      : OpType(other, spv::OpTypeRuntimeArray) {}
  spv::Id ElementType() const;
  static const spv::Op ClassCode = spv::OpTypeRuntimeArray;
};

class OpTypeStruct : public OpType {
 public:
  OpTypeStruct(OpCode const &other) : OpType(other, spv::OpTypeStruct) {}
  llvm::SmallVector<spv::Id, 8> MemberTypes() const;
  static const spv::Op ClassCode = spv::OpTypeStruct;
};

class OpTypeOpaque : public OpType {
 public:
  OpTypeOpaque(OpCode const &other) : OpType(other, spv::OpTypeOpaque) {}
  llvm::StringRef Name() const;
  static const spv::Op ClassCode = spv::OpTypeOpaque;
};

class OpTypePointer : public OpType {
 public:
  OpTypePointer(OpCode const &other) : OpType(other, spv::OpTypePointer) {}
  uint32_t StorageClass() const;
  spv::Id Type() const;
  static const spv::Op ClassCode = spv::OpTypePointer;
};

class OpTypeFunction : public OpType {
 public:
  OpTypeFunction(OpCode const &other) : OpType(other, spv::OpTypeFunction) {}
  spv::Id ReturnType() const;
  llvm::SmallVector<spv::Id, 8> ParameterTypes() const;
  static const spv::Op ClassCode = spv::OpTypeFunction;
};

class OpTypeEvent : public OpType {
 public:
  OpTypeEvent(OpCode const &other) : OpType(other, spv::OpTypeEvent) {}
  static const spv::Op ClassCode = spv::OpTypeEvent;
};

class OpTypeDeviceEvent : public OpType {
 public:
  OpTypeDeviceEvent(OpCode const &other)
      : OpType(other, spv::OpTypeDeviceEvent) {}
  static const spv::Op ClassCode = spv::OpTypeDeviceEvent;
};

class OpTypeReserveId : public OpType {
 public:
  OpTypeReserveId(OpCode const &other) : OpType(other, spv::OpTypeReserveId) {}
  static const spv::Op ClassCode = spv::OpTypeReserveId;
};

class OpTypeQueue : public OpType {
 public:
  OpTypeQueue(OpCode const &other) : OpType(other, spv::OpTypeQueue) {}
  static const spv::Op ClassCode = spv::OpTypeQueue;
};

class OpTypePipe : public OpType {
 public:
  OpTypePipe(OpCode const &other) : OpType(other, spv::OpTypePipe) {}
  spv::AccessQualifier Qualifier() const;
  static const spv::Op ClassCode = spv::OpTypePipe;
};

class OpTypeForwardPointer : public OpType {
 public:
  OpTypeForwardPointer(OpCode const &other)
      : OpType(other, spv::OpTypeForwardPointer) {}
  spv::Id PointerType() const;
  spv::StorageClass StorageClass() const;
  static const spv::Op ClassCode = spv::OpTypeForwardPointer;
};

class OpConstantTrue : public OpResult {
 public:
  OpConstantTrue(OpCode const &other) : OpResult(other, spv::OpConstantTrue) {}
  static const spv::Op ClassCode = spv::OpConstantTrue;
};

class OpConstantFalse : public OpResult {
 public:
  OpConstantFalse(OpCode const &other)
      : OpResult(other, spv::OpConstantFalse) {}
  static const spv::Op ClassCode = spv::OpConstantFalse;
};

class OpConstant : public OpResult {
 public:
  OpConstant(OpCode const &other) : OpResult(other, spv::OpConstant) {}
  uint32_t Value32() const;
  uint64_t Value64() const;
  static const spv::Op ClassCode = spv::OpConstant;
};

class OpConstantComposite : public OpResult {
 public:
  OpConstantComposite(OpCode const &other)
      : OpResult(other, spv::OpConstantComposite) {}
  llvm::SmallVector<spv::Id, 8> Constituents() const;
  static const spv::Op ClassCode = spv::OpConstantComposite;
};

class OpConstantSampler : public OpResult {
 public:
  OpConstantSampler(OpCode const &other)
      : OpResult(other, spv::OpConstantSampler) {}
  spv::SamplerAddressingMode SamplerAddressingMode() const;
  uint32_t Param() const;
  spv::SamplerFilterMode SamplerFilterMode() const;
  static const spv::Op ClassCode = spv::OpConstantSampler;
};

class OpConstantNull : public OpResult {
 public:
  OpConstantNull(OpCode const &other) : OpResult(other, spv::OpConstantNull) {}
  static const spv::Op ClassCode = spv::OpConstantNull;
};

class OpSpecConstantTrue : public OpResult {
 public:
  OpSpecConstantTrue(OpCode const &other)
      : OpResult(other, spv::OpSpecConstantTrue) {}
  static const spv::Op ClassCode = spv::OpSpecConstantTrue;
};

class OpSpecConstantFalse : public OpResult {
 public:
  OpSpecConstantFalse(OpCode const &other)
      : OpResult(other, spv::OpSpecConstantFalse) {}
  static const spv::Op ClassCode = spv::OpSpecConstantFalse;
};

class OpSpecConstant : public OpResult {
 public:
  OpSpecConstant(OpCode const &other) : OpResult(other, spv::OpSpecConstant) {}
  uint32_t Value32() const;
  uint64_t Value64() const;
  static const spv::Op ClassCode = spv::OpSpecConstant;
};

class OpSpecConstantComposite : public OpResult {
 public:
  OpSpecConstantComposite(OpCode const &other)
      : OpResult(other, spv::OpSpecConstantComposite) {}
  llvm::SmallVector<spv::Id, 8> Constituents() const;
  static const spv::Op ClassCode = spv::OpSpecConstantComposite;
};

class OpSpecConstantOp : public OpResult {
 public:
  OpSpecConstantOp(OpCode const &other)
      : OpResult(other, spv::OpSpecConstantOp) {}
  uint32_t Opcode() const;
  static const spv::Op ClassCode = spv::OpSpecConstantOp;
};

class OpFunction : public OpResult {
 public:
  OpFunction(OpCode const &other) : OpResult(other, spv::OpFunction) {}
  uint32_t FunctionControl() const;
  spv::Id FunctionType() const;
  static const spv::Op ClassCode = spv::OpFunction;
};

class OpFunctionParameter : public OpResult {
 public:
  OpFunctionParameter(OpCode const &other)
      : OpResult(other, spv::OpFunctionParameter) {}
  static const spv::Op ClassCode = spv::OpFunctionParameter;
};

class OpFunctionEnd : public OpCode {
 public:
  OpFunctionEnd(OpCode const &other) : OpCode(other, spv::OpFunctionEnd) {}
  static const spv::Op ClassCode = spv::OpFunctionEnd;
};

class OpFunctionCall : public OpResult {
 public:
  OpFunctionCall(OpCode const &other) : OpResult(other, spv::OpFunctionCall) {}
  spv::Id Function() const;
  llvm::SmallVector<spv::Id, 8> Arguments() const;
  static const spv::Op ClassCode = spv::OpFunctionCall;
};

class OpVariable : public OpResult {
 public:
  OpVariable(OpCode const &other) : OpResult(other, spv::OpVariable) {}
  uint32_t StorageClass() const;
  spv::Id Initializer() const;
  static const spv::Op ClassCode = spv::OpVariable;
};

class OpImageTexelPointer : public OpResult {
 public:
  OpImageTexelPointer(OpCode const &other)
      : OpResult(other, spv::OpImageTexelPointer) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  spv::Id Sample() const;
  static const spv::Op ClassCode = spv::OpImageTexelPointer;
};

class OpLoad : public OpResult {
 public:
  OpLoad(OpCode const &other) : OpResult(other, spv::OpLoad) {}
  spv::Id Pointer() const;
  uint32_t MemoryAccess() const;
  static const spv::Op ClassCode = spv::OpLoad;
};

class OpStore : public OpCode {
 public:
  OpStore(OpCode const &other) : OpCode(other, spv::OpStore) {}
  spv::Id Pointer() const;
  spv::Id Object() const;
  uint32_t MemoryAccess() const;
  static const spv::Op ClassCode = spv::OpStore;
};

class OpCopyMemory : public OpCode {
 public:
  OpCopyMemory(OpCode const &other) : OpCode(other, spv::OpCopyMemory) {}
  spv::Id Target() const;
  spv::Id Source() const;
  uint32_t MemoryAccess() const;
  static const spv::Op ClassCode = spv::OpCopyMemory;
};

class OpCopyMemorySized : public OpCode {
 public:
  OpCopyMemorySized(OpCode const &other)
      : OpCode(other, spv::OpCopyMemorySized) {}
  spv::Id Target() const;
  spv::Id Source() const;
  spv::Id Size() const;
  uint32_t MemoryAccess() const;
  static const spv::Op ClassCode = spv::OpCopyMemorySized;
};

class OpAccessChain : public OpResult {
 public:
  OpAccessChain(OpCode const &other) : OpResult(other, spv::OpAccessChain) {}
  spv::Id Base() const;
  llvm::SmallVector<spv::Id, 8> Indexes() const;
  static const spv::Op ClassCode = spv::OpAccessChain;
};

class OpInBoundsAccessChain : public OpResult {
 public:
  OpInBoundsAccessChain(OpCode const &other)
      : OpResult(other, spv::OpInBoundsAccessChain) {}
  spv::Id Base() const;
  llvm::SmallVector<spv::Id, 8> Indexes() const;
  static const spv::Op ClassCode = spv::OpInBoundsAccessChain;
};

class OpPtrAccessChain : public OpResult {
 public:
  OpPtrAccessChain(OpCode const &other)
      : OpResult(other, spv::OpPtrAccessChain) {}
  spv::Id Base() const;
  spv::Id Element() const;
  llvm::SmallVector<spv::Id, 8> Indexes() const;
  static const spv::Op ClassCode = spv::OpPtrAccessChain;
};

class OpArrayLength : public OpResult {
 public:
  OpArrayLength(OpCode const &other) : OpResult(other, spv::OpArrayLength) {}
  spv::Id Structure() const;
  uint32_t Arraymember() const;
  static const spv::Op ClassCode = spv::OpArrayLength;
};

class OpGenericPtrMemSemantics : public OpResult {
 public:
  OpGenericPtrMemSemantics(OpCode const &other)
      : OpResult(other, spv::OpGenericPtrMemSemantics) {}
  spv::Id Pointer() const;
  static const spv::Op ClassCode = spv::OpGenericPtrMemSemantics;
};

class OpInBoundsPtrAccessChain : public OpResult {
 public:
  OpInBoundsPtrAccessChain(OpCode const &other)
      : OpResult(other, spv::OpInBoundsPtrAccessChain) {}
  spv::Id Base() const;
  spv::Id Element() const;
  llvm::SmallVector<spv::Id, 8> Indexes() const;
  static const spv::Op ClassCode = spv::OpInBoundsPtrAccessChain;
};

class OpDecorate : public OpDecorateBase {
 public:
  OpDecorate(OpCode const &other) : OpDecorateBase(other, spv::OpDecorate) {}
  spv::Id Target() const;
  spv::Decoration Decoration() const;
  const char *getDecorationString() const;
  static const spv::Op ClassCode = spv::OpDecorate;
};

class OpMemberDecorate : public OpDecorateBase {
 public:
  OpMemberDecorate(OpCode const &other)
      : OpDecorateBase(other, spv::OpMemberDecorate) {}
  spv::Id StructureType() const;
  uint32_t Member() const;
  spv::Decoration Decoration() const;
  static const spv::Op ClassCode = spv::OpMemberDecorate;
};

class OpDecorationGroup : public OpCode {
 public:
  OpDecorationGroup(OpCode const &other)
      : OpCode(other, spv::OpDecorationGroup) {}
  spv::Id IdResult() const;
  static const spv::Op ClassCode = spv::OpDecorationGroup;
};

class OpGroupDecorate : public OpCode {
 public:
  OpGroupDecorate(OpCode const &other) : OpCode(other, spv::OpGroupDecorate) {}
  spv::Id DecorationGroup() const;
  llvm::SmallVector<spv::Id, 8> Targets() const;
  static const spv::Op ClassCode = spv::OpGroupDecorate;
};

class OpGroupMemberDecorate : public OpCode {
 public:
  OpGroupMemberDecorate(OpCode const &other)
      : OpCode(other, spv::OpGroupMemberDecorate) {}
  spv::Id DecorationGroup() const;
  struct TargetsT {
    spv::Id Id;
    uint32_t Literal;
  };
  llvm::SmallVector<TargetsT, 4> Targets() const;
  static const spv::Op ClassCode = spv::OpGroupMemberDecorate;
};

class OpVectorExtractDynamic : public OpResult {
 public:
  OpVectorExtractDynamic(OpCode const &other)
      : OpResult(other, spv::OpVectorExtractDynamic) {}
  spv::Id Vector() const;
  spv::Id Index() const;
  static const spv::Op ClassCode = spv::OpVectorExtractDynamic;
};

class OpVectorInsertDynamic : public OpResult {
 public:
  OpVectorInsertDynamic(OpCode const &other)
      : OpResult(other, spv::OpVectorInsertDynamic) {}
  spv::Id Vector() const;
  spv::Id Component() const;
  spv::Id Index() const;
  static const spv::Op ClassCode = spv::OpVectorInsertDynamic;
};

class OpVectorShuffle : public OpResult {
 public:
  OpVectorShuffle(OpCode const &other)
      : OpResult(other, spv::OpVectorShuffle) {}
  spv::Id Vector1() const;
  spv::Id Vector2() const;
  llvm::SmallVector<uint32_t, 16> Components() const;
  static const spv::Op ClassCode = spv::OpVectorShuffle;
};

class OpCompositeConstruct : public OpResult {
 public:
  OpCompositeConstruct(OpCode const &other)
      : OpResult(other, spv::OpCompositeConstruct) {}
  llvm::SmallVector<spv::Id, 8> Constituents() const;
  static const spv::Op ClassCode = spv::OpCompositeConstruct;
};

class OpCompositeExtract : public OpResult {
 public:
  OpCompositeExtract(OpCode const &other)
      : OpResult(other, spv::OpCompositeExtract) {}
  spv::Id Composite() const;
  llvm::SmallVector<uint32_t, 4> Indexes() const;
  static const spv::Op ClassCode = spv::OpCompositeExtract;
};

class OpCompositeInsert : public OpResult {
 public:
  OpCompositeInsert(OpCode const &other)
      : OpResult(other, spv::OpCompositeInsert) {}
  spv::Id Object() const;
  spv::Id Composite() const;
  llvm::SmallVector<uint32_t, 4> Indexes() const;
  static const spv::Op ClassCode = spv::OpCompositeInsert;
};

class OpCopyObject : public OpResult {
 public:
  OpCopyObject(OpCode const &other) : OpResult(other, spv::OpCopyObject) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpCopyObject;
};

class OpTranspose : public OpResult {
 public:
  OpTranspose(OpCode const &other) : OpResult(other, spv::OpTranspose) {}
  spv::Id Matrix() const;
  static const spv::Op ClassCode = spv::OpTranspose;
};

class OpSampledImage : public OpResult {
 public:
  OpSampledImage(OpCode const &other) : OpResult(other, spv::OpSampledImage) {}
  spv::Id Image() const;
  spv::Id Sampler() const;
  static const spv::Op ClassCode = spv::OpSampledImage;
};

class OpImageSampleImplicitLod : public OpResult {
 public:
  OpImageSampleImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleImplicitLod;
};

class OpImageSampleExplicitLod : public OpResult {
 public:
  OpImageSampleExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleExplicitLod;
};

class OpImageSampleDrefImplicitLod : public OpResult {
 public:
  OpImageSampleDrefImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleDrefImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleDrefImplicitLod;
};

class OpImageSampleDrefExplicitLod : public OpResult {
 public:
  OpImageSampleDrefExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleDrefExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleDrefExplicitLod;
};

class OpImageSampleProjImplicitLod : public OpResult {
 public:
  OpImageSampleProjImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleProjImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleProjImplicitLod;
};

class OpImageSampleProjExplicitLod : public OpResult {
 public:
  OpImageSampleProjExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleProjExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleProjExplicitLod;
};

class OpImageSampleProjDrefImplicitLod : public OpResult {
 public:
  OpImageSampleProjDrefImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleProjDrefImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleProjDrefImplicitLod;
};

class OpImageSampleProjDrefExplicitLod : public OpResult {
 public:
  OpImageSampleProjDrefExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSampleProjDrefExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSampleProjDrefExplicitLod;
};

class OpImageFetch : public OpResult {
 public:
  OpImageFetch(OpCode const &other) : OpResult(other, spv::OpImageFetch) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageFetch;
};

class OpImageGather : public OpResult {
 public:
  OpImageGather(OpCode const &other) : OpResult(other, spv::OpImageGather) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Component() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageGather;
};

class OpImageDrefGather : public OpResult {
 public:
  OpImageDrefGather(OpCode const &other)
      : OpResult(other, spv::OpImageDrefGather) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageDrefGather;
};

class OpImageRead : public OpResult {
 public:
  OpImageRead(OpCode const &other) : OpResult(other, spv::OpImageRead) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageRead;
};

class OpImageWrite : public OpCode {
 public:
  OpImageWrite(OpCode const &other) : OpCode(other, spv::OpImageWrite) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  spv::Id Texel() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageWrite;
};

class OpImage : public OpResult {
 public:
  OpImage(OpCode const &other) : OpResult(other, spv::OpImage) {}
  spv::Id SampledImage() const;
  static const spv::Op ClassCode = spv::OpImage;
};

class OpImageQueryFormat : public OpResult {
 public:
  OpImageQueryFormat(OpCode const &other)
      : OpResult(other, spv::OpImageQueryFormat) {}
  spv::Id Image() const;
  static const spv::Op ClassCode = spv::OpImageQueryFormat;
};

class OpImageQueryOrder : public OpResult {
 public:
  OpImageQueryOrder(OpCode const &other)
      : OpResult(other, spv::OpImageQueryOrder) {}
  spv::Id Image() const;
  static const spv::Op ClassCode = spv::OpImageQueryOrder;
};

class OpImageQuerySizeLod : public OpResult {
 public:
  OpImageQuerySizeLod(OpCode const &other)
      : OpResult(other, spv::OpImageQuerySizeLod) {}
  spv::Id Image() const;
  spv::Id LevelofDetail() const;
  static const spv::Op ClassCode = spv::OpImageQuerySizeLod;
};

class OpImageQuerySize : public OpResult {
 public:
  OpImageQuerySize(OpCode const &other)
      : OpResult(other, spv::OpImageQuerySize) {}
  spv::Id Image() const;
  static const spv::Op ClassCode = spv::OpImageQuerySize;
};

class OpImageQueryLod : public OpResult {
 public:
  OpImageQueryLod(OpCode const &other)
      : OpResult(other, spv::OpImageQueryLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  static const spv::Op ClassCode = spv::OpImageQueryLod;
};

class OpImageQueryLevels : public OpResult {
 public:
  OpImageQueryLevels(OpCode const &other)
      : OpResult(other, spv::OpImageQueryLevels) {}
  spv::Id Image() const;
  static const spv::Op ClassCode = spv::OpImageQueryLevels;
};

class OpImageQuerySamples : public OpResult {
 public:
  OpImageQuerySamples(OpCode const &other)
      : OpResult(other, spv::OpImageQuerySamples) {}
  spv::Id Image() const;
  static const spv::Op ClassCode = spv::OpImageQuerySamples;
};

class OpConvertFToU : public OpResult {
 public:
  OpConvertFToU(OpCode const &other) : OpResult(other, spv::OpConvertFToU) {}
  spv::Id FloatValue() const;
  static const spv::Op ClassCode = spv::OpConvertFToU;
};

class OpConvertFToS : public OpResult {
 public:
  OpConvertFToS(OpCode const &other) : OpResult(other, spv::OpConvertFToS) {}
  spv::Id FloatValue() const;
  static const spv::Op ClassCode = spv::OpConvertFToS;
};

class OpConvertSToF : public OpResult {
 public:
  OpConvertSToF(OpCode const &other) : OpResult(other, spv::OpConvertSToF) {}
  spv::Id SignedValue() const;
  static const spv::Op ClassCode = spv::OpConvertSToF;
};

class OpConvertUToF : public OpResult {
 public:
  OpConvertUToF(OpCode const &other) : OpResult(other, spv::OpConvertUToF) {}
  spv::Id UnsignedValue() const;
  static const spv::Op ClassCode = spv::OpConvertUToF;
};

class OpUConvert : public OpResult {
 public:
  OpUConvert(OpCode const &other) : OpResult(other, spv::OpUConvert) {}
  spv::Id UnsignedValue() const;
  static const spv::Op ClassCode = spv::OpUConvert;
};

class OpSConvert : public OpResult {
 public:
  OpSConvert(OpCode const &other) : OpResult(other, spv::OpSConvert) {}
  spv::Id SignedValue() const;
  static const spv::Op ClassCode = spv::OpSConvert;
};

class OpFConvert : public OpResult {
 public:
  OpFConvert(OpCode const &other) : OpResult(other, spv::OpFConvert) {}
  spv::Id FloatValue() const;
  static const spv::Op ClassCode = spv::OpFConvert;
};

class OpQuantizeToF16 : public OpResult {
 public:
  OpQuantizeToF16(OpCode const &other)
      : OpResult(other, spv::OpQuantizeToF16) {}
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpQuantizeToF16;
};

class OpConvertPtrToU : public OpResult {
 public:
  OpConvertPtrToU(OpCode const &other)
      : OpResult(other, spv::OpConvertPtrToU) {}
  spv::Id Pointer() const;
  static const spv::Op ClassCode = spv::OpConvertPtrToU;
};

class OpSatConvertSToU : public OpResult {
 public:
  OpSatConvertSToU(OpCode const &other)
      : OpResult(other, spv::OpSatConvertSToU) {}
  spv::Id SignedValue() const;
  static const spv::Op ClassCode = spv::OpSatConvertSToU;
};

class OpSatConvertUToS : public OpResult {
 public:
  OpSatConvertUToS(OpCode const &other)
      : OpResult(other, spv::OpSatConvertUToS) {}
  spv::Id UnsignedValue() const;
  static const spv::Op ClassCode = spv::OpSatConvertUToS;
};

class OpConvertUToPtr : public OpResult {
 public:
  OpConvertUToPtr(OpCode const &other)
      : OpResult(other, spv::OpConvertUToPtr) {}
  spv::Id IntegerValue() const;
  static const spv::Op ClassCode = spv::OpConvertUToPtr;
};

class OpPtrCastToGeneric : public OpResult {
 public:
  OpPtrCastToGeneric(OpCode const &other)
      : OpResult(other, spv::OpPtrCastToGeneric) {}
  spv::Id Pointer() const;
  static const spv::Op ClassCode = spv::OpPtrCastToGeneric;
};

class OpGenericCastToPtr : public OpResult {
 public:
  OpGenericCastToPtr(OpCode const &other)
      : OpResult(other, spv::OpGenericCastToPtr) {}
  spv::Id Pointer() const;
  static const spv::Op ClassCode = spv::OpGenericCastToPtr;
};

class OpGenericCastToPtrExplicit : public OpResult {
 public:
  OpGenericCastToPtrExplicit(OpCode const &other)
      : OpResult(other, spv::OpGenericCastToPtrExplicit) {}
  spv::Id Pointer() const;
  spv::StorageClass Storage() const;
  static const spv::Op ClassCode = spv::OpGenericCastToPtrExplicit;
};

class OpBitcast : public OpResult {
 public:
  OpBitcast(OpCode const &other) : OpResult(other, spv::OpBitcast) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpBitcast;
};

class OpSNegate : public OpResult {
 public:
  OpSNegate(OpCode const &other) : OpResult(other, spv::OpSNegate) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpSNegate;
};

class OpFNegate : public OpResult {
 public:
  OpFNegate(OpCode const &other) : OpResult(other, spv::OpFNegate) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpFNegate;
};

class OpIAdd : public OpResult {
 public:
  OpIAdd(OpCode const &other) : OpResult(other, spv::OpIAdd) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpIAdd;
};

class OpFAdd : public OpResult {
 public:
  OpFAdd(OpCode const &other) : OpResult(other, spv::OpFAdd) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFAdd;
};

class OpISub : public OpResult {
 public:
  OpISub(OpCode const &other) : OpResult(other, spv::OpISub) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpISub;
};

class OpFSub : public OpResult {
 public:
  OpFSub(OpCode const &other) : OpResult(other, spv::OpFSub) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFSub;
};

class OpIMul : public OpResult {
 public:
  OpIMul(OpCode const &other) : OpResult(other, spv::OpIMul) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpIMul;
};

class OpFMul : public OpResult {
 public:
  OpFMul(OpCode const &other) : OpResult(other, spv::OpFMul) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFMul;
};

class OpUDiv : public OpResult {
 public:
  OpUDiv(OpCode const &other) : OpResult(other, spv::OpUDiv) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpUDiv;
};

class OpSDiv : public OpResult {
 public:
  OpSDiv(OpCode const &other) : OpResult(other, spv::OpSDiv) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSDiv;
};

class OpFDiv : public OpResult {
 public:
  OpFDiv(OpCode const &other) : OpResult(other, spv::OpFDiv) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFDiv;
};

class OpUMod : public OpResult {
 public:
  OpUMod(OpCode const &other) : OpResult(other, spv::OpUMod) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpUMod;
};

class OpSRem : public OpResult {
 public:
  OpSRem(OpCode const &other) : OpResult(other, spv::OpSRem) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSRem;
};

class OpSMod : public OpResult {
 public:
  OpSMod(OpCode const &other) : OpResult(other, spv::OpSMod) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSMod;
};

class OpFRem : public OpResult {
 public:
  OpFRem(OpCode const &other) : OpResult(other, spv::OpFRem) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFRem;
};

class OpFMod : public OpResult {
 public:
  OpFMod(OpCode const &other) : OpResult(other, spv::OpFMod) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFMod;
};

class OpVectorTimesScalar : public OpResult {
 public:
  OpVectorTimesScalar(OpCode const &other)
      : OpResult(other, spv::OpVectorTimesScalar) {}
  spv::Id Vector() const;
  spv::Id Scalar() const;
  static const spv::Op ClassCode = spv::OpVectorTimesScalar;
};

class OpMatrixTimesScalar : public OpResult {
 public:
  OpMatrixTimesScalar(OpCode const &other)
      : OpResult(other, spv::OpMatrixTimesScalar) {}
  spv::Id Matrix() const;
  spv::Id Scalar() const;
  static const spv::Op ClassCode = spv::OpMatrixTimesScalar;
};

class OpVectorTimesMatrix : public OpResult {
 public:
  OpVectorTimesMatrix(OpCode const &other)
      : OpResult(other, spv::OpVectorTimesMatrix) {}
  spv::Id Vector() const;
  spv::Id Matrix() const;
  static const spv::Op ClassCode = spv::OpVectorTimesMatrix;
};

class OpMatrixTimesVector : public OpResult {
 public:
  OpMatrixTimesVector(OpCode const &other)
      : OpResult(other, spv::OpMatrixTimesVector) {}
  spv::Id Matrix() const;
  spv::Id Vector() const;
  static const spv::Op ClassCode = spv::OpMatrixTimesVector;
};

class OpMatrixTimesMatrix : public OpResult {
 public:
  OpMatrixTimesMatrix(OpCode const &other)
      : OpResult(other, spv::OpMatrixTimesMatrix) {}
  spv::Id LeftMatrix() const;
  spv::Id RightMatrix() const;
  static const spv::Op ClassCode = spv::OpMatrixTimesMatrix;
};

class OpOuterProduct : public OpResult {
 public:
  OpOuterProduct(OpCode const &other) : OpResult(other, spv::OpOuterProduct) {}
  spv::Id Vector1() const;
  spv::Id Vector2() const;
  static const spv::Op ClassCode = spv::OpOuterProduct;
};

class OpDot : public OpResult {
 public:
  OpDot(OpCode const &other) : OpResult(other, spv::OpDot) {}
  spv::Id Vector1() const;
  spv::Id Vector2() const;
  static const spv::Op ClassCode = spv::OpDot;
};

class OpIAddCarry : public OpResult {
 public:
  OpIAddCarry(OpCode const &other) : OpResult(other, spv::OpIAddCarry) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpIAddCarry;
};

class OpISubBorrow : public OpResult {
 public:
  OpISubBorrow(OpCode const &other) : OpResult(other, spv::OpISubBorrow) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpISubBorrow;
};

class OpUMulExtended : public OpResult {
 public:
  OpUMulExtended(OpCode const &other) : OpResult(other, spv::OpUMulExtended) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpUMulExtended;
};

class OpSMulExtended : public OpResult {
 public:
  OpSMulExtended(OpCode const &other) : OpResult(other, spv::OpSMulExtended) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSMulExtended;
};

class OpAny : public OpResult {
 public:
  OpAny(OpCode const &other) : OpResult(other, spv::OpAny) {}
  spv::Id Vector() const;
  static const spv::Op ClassCode = spv::OpAny;
};

class OpAll : public OpResult {
 public:
  OpAll(OpCode const &other) : OpResult(other, spv::OpAll) {}
  spv::Id Vector() const;
  static const spv::Op ClassCode = spv::OpAll;
};

class OpIsNan : public OpResult {
 public:
  OpIsNan(OpCode const &other) : OpResult(other, spv::OpIsNan) {}
  spv::Id x() const;
  static const spv::Op ClassCode = spv::OpIsNan;
};

class OpIsInf : public OpResult {
 public:
  OpIsInf(OpCode const &other) : OpResult(other, spv::OpIsInf) {}
  spv::Id x() const;
  static const spv::Op ClassCode = spv::OpIsInf;
};

class OpIsFinite : public OpResult {
 public:
  OpIsFinite(OpCode const &other) : OpResult(other, spv::OpIsFinite) {}
  spv::Id x() const;
  static const spv::Op ClassCode = spv::OpIsFinite;
};

class OpIsNormal : public OpResult {
 public:
  OpIsNormal(OpCode const &other) : OpResult(other, spv::OpIsNormal) {}
  spv::Id x() const;
  static const spv::Op ClassCode = spv::OpIsNormal;
};

class OpSignBitSet : public OpResult {
 public:
  OpSignBitSet(OpCode const &other) : OpResult(other, spv::OpSignBitSet) {}
  spv::Id x() const;
  static const spv::Op ClassCode = spv::OpSignBitSet;
};

class OpLessOrGreater : public OpResult {
 public:
  OpLessOrGreater(OpCode const &other)
      : OpResult(other, spv::OpLessOrGreater) {}
  spv::Id x() const;
  spv::Id y() const;
  static const spv::Op ClassCode = spv::OpLessOrGreater;
};

class OpOrdered : public OpResult {
 public:
  OpOrdered(OpCode const &other) : OpResult(other, spv::OpOrdered) {}
  spv::Id x() const;
  spv::Id y() const;
  static const spv::Op ClassCode = spv::OpOrdered;
};

class OpUnordered : public OpResult {
 public:
  OpUnordered(OpCode const &other) : OpResult(other, spv::OpUnordered) {}
  spv::Id x() const;
  spv::Id y() const;
  static const spv::Op ClassCode = spv::OpUnordered;
};

class OpLogicalEqual : public OpResult {
 public:
  OpLogicalEqual(OpCode const &other) : OpResult(other, spv::OpLogicalEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpLogicalEqual;
};

class OpLogicalNotEqual : public OpResult {
 public:
  OpLogicalNotEqual(OpCode const &other)
      : OpResult(other, spv::OpLogicalNotEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpLogicalNotEqual;
};

class OpLogicalOr : public OpResult {
 public:
  OpLogicalOr(OpCode const &other) : OpResult(other, spv::OpLogicalOr) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpLogicalOr;
};

class OpLogicalAnd : public OpResult {
 public:
  OpLogicalAnd(OpCode const &other) : OpResult(other, spv::OpLogicalAnd) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpLogicalAnd;
};

class OpLogicalNot : public OpResult {
 public:
  OpLogicalNot(OpCode const &other) : OpResult(other, spv::OpLogicalNot) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpLogicalNot;
};

class OpSelect : public OpResult {
 public:
  OpSelect(OpCode const &other) : OpResult(other, spv::OpSelect) {}
  spv::Id Condition() const;
  spv::Id Object1() const;
  spv::Id Object2() const;
  static const spv::Op ClassCode = spv::OpSelect;
};

class OpIEqual : public OpResult {
 public:
  OpIEqual(OpCode const &other) : OpResult(other, spv::OpIEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpIEqual;
};

class OpINotEqual : public OpResult {
 public:
  OpINotEqual(OpCode const &other) : OpResult(other, spv::OpINotEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpINotEqual;
};

class OpUGreaterThan : public OpResult {
 public:
  OpUGreaterThan(OpCode const &other) : OpResult(other, spv::OpUGreaterThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpUGreaterThan;
};

class OpSGreaterThan : public OpResult {
 public:
  OpSGreaterThan(OpCode const &other) : OpResult(other, spv::OpSGreaterThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSGreaterThan;
};

class OpUGreaterThanEqual : public OpResult {
 public:
  OpUGreaterThanEqual(OpCode const &other)
      : OpResult(other, spv::OpUGreaterThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpUGreaterThanEqual;
};

class OpSGreaterThanEqual : public OpResult {
 public:
  OpSGreaterThanEqual(OpCode const &other)
      : OpResult(other, spv::OpSGreaterThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSGreaterThanEqual;
};

class OpULessThan : public OpResult {
 public:
  OpULessThan(OpCode const &other) : OpResult(other, spv::OpULessThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpULessThan;
};

class OpSLessThan : public OpResult {
 public:
  OpSLessThan(OpCode const &other) : OpResult(other, spv::OpSLessThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSLessThan;
};

class OpULessThanEqual : public OpResult {
 public:
  OpULessThanEqual(OpCode const &other)
      : OpResult(other, spv::OpULessThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpULessThanEqual;
};

class OpSLessThanEqual : public OpResult {
 public:
  OpSLessThanEqual(OpCode const &other)
      : OpResult(other, spv::OpSLessThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpSLessThanEqual;
};

class OpFOrdEqual : public OpResult {
 public:
  OpFOrdEqual(OpCode const &other) : OpResult(other, spv::OpFOrdEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdEqual;
};

class OpFUnordEqual : public OpResult {
 public:
  OpFUnordEqual(OpCode const &other) : OpResult(other, spv::OpFUnordEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordEqual;
};

class OpFOrdNotEqual : public OpResult {
 public:
  OpFOrdNotEqual(OpCode const &other) : OpResult(other, spv::OpFOrdNotEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdNotEqual;
};

class OpFUnordNotEqual : public OpResult {
 public:
  OpFUnordNotEqual(OpCode const &other)
      : OpResult(other, spv::OpFUnordNotEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordNotEqual;
};

class OpFOrdLessThan : public OpResult {
 public:
  OpFOrdLessThan(OpCode const &other) : OpResult(other, spv::OpFOrdLessThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdLessThan;
};

class OpFUnordLessThan : public OpResult {
 public:
  OpFUnordLessThan(OpCode const &other)
      : OpResult(other, spv::OpFUnordLessThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordLessThan;
};

class OpFOrdGreaterThan : public OpResult {
 public:
  OpFOrdGreaterThan(OpCode const &other)
      : OpResult(other, spv::OpFOrdGreaterThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdGreaterThan;
};

class OpFUnordGreaterThan : public OpResult {
 public:
  OpFUnordGreaterThan(OpCode const &other)
      : OpResult(other, spv::OpFUnordGreaterThan) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordGreaterThan;
};

class OpFOrdLessThanEqual : public OpResult {
 public:
  OpFOrdLessThanEqual(OpCode const &other)
      : OpResult(other, spv::OpFOrdLessThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdLessThanEqual;
};

class OpFUnordLessThanEqual : public OpResult {
 public:
  OpFUnordLessThanEqual(OpCode const &other)
      : OpResult(other, spv::OpFUnordLessThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordLessThanEqual;
};

class OpFOrdGreaterThanEqual : public OpResult {
 public:
  OpFOrdGreaterThanEqual(OpCode const &other)
      : OpResult(other, spv::OpFOrdGreaterThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFOrdGreaterThanEqual;
};

class OpFUnordGreaterThanEqual : public OpResult {
 public:
  OpFUnordGreaterThanEqual(OpCode const &other)
      : OpResult(other, spv::OpFUnordGreaterThanEqual) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpFUnordGreaterThanEqual;
};

class OpShiftRightLogical : public OpResult {
 public:
  OpShiftRightLogical(OpCode const &other)
      : OpResult(other, spv::OpShiftRightLogical) {}
  spv::Id Base() const;
  spv::Id Shift() const;
  static const spv::Op ClassCode = spv::OpShiftRightLogical;
};

class OpShiftRightArithmetic : public OpResult {
 public:
  OpShiftRightArithmetic(OpCode const &other)
      : OpResult(other, spv::OpShiftRightArithmetic) {}
  spv::Id Base() const;
  spv::Id Shift() const;
  static const spv::Op ClassCode = spv::OpShiftRightArithmetic;
};

class OpShiftLeftLogical : public OpResult {
 public:
  OpShiftLeftLogical(OpCode const &other)
      : OpResult(other, spv::OpShiftLeftLogical) {}
  spv::Id Base() const;
  spv::Id Shift() const;
  static const spv::Op ClassCode = spv::OpShiftLeftLogical;
};

class OpBitwiseOr : public OpResult {
 public:
  OpBitwiseOr(OpCode const &other) : OpResult(other, spv::OpBitwiseOr) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpBitwiseOr;
};

class OpBitwiseXor : public OpResult {
 public:
  OpBitwiseXor(OpCode const &other) : OpResult(other, spv::OpBitwiseXor) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpBitwiseXor;
};

class OpBitwiseAnd : public OpResult {
 public:
  OpBitwiseAnd(OpCode const &other) : OpResult(other, spv::OpBitwiseAnd) {}
  spv::Id Operand1() const;
  spv::Id Operand2() const;
  static const spv::Op ClassCode = spv::OpBitwiseAnd;
};

class OpNot : public OpResult {
 public:
  OpNot(OpCode const &other) : OpResult(other, spv::OpNot) {}
  spv::Id Operand() const;
  static const spv::Op ClassCode = spv::OpNot;
};

class OpBitFieldInsert : public OpResult {
 public:
  OpBitFieldInsert(OpCode const &other)
      : OpResult(other, spv::OpBitFieldInsert) {}
  spv::Id Base() const;
  spv::Id Insert() const;
  spv::Id Offset() const;
  spv::Id Count() const;
  static const spv::Op ClassCode = spv::OpBitFieldInsert;
};

class OpBitFieldSExtract : public OpResult {
 public:
  OpBitFieldSExtract(OpCode const &other)
      : OpResult(other, spv::OpBitFieldSExtract) {}
  spv::Id Base() const;
  spv::Id Offset() const;
  spv::Id Count() const;
  static const spv::Op ClassCode = spv::OpBitFieldSExtract;
};

class OpBitFieldUExtract : public OpResult {
 public:
  OpBitFieldUExtract(OpCode const &other)
      : OpResult(other, spv::OpBitFieldUExtract) {}
  spv::Id Base() const;
  spv::Id Offset() const;
  spv::Id Count() const;
  static const spv::Op ClassCode = spv::OpBitFieldUExtract;
};

class OpBitReverse : public OpResult {
 public:
  OpBitReverse(OpCode const &other) : OpResult(other, spv::OpBitReverse) {}
  spv::Id Base() const;
  static const spv::Op ClassCode = spv::OpBitReverse;
};

class OpBitCount : public OpResult {
 public:
  OpBitCount(OpCode const &other) : OpResult(other, spv::OpBitCount) {}
  spv::Id Base() const;
  static const spv::Op ClassCode = spv::OpBitCount;
};

class OpDPdx : public OpResult {
 public:
  OpDPdx(OpCode const &other) : OpResult(other, spv::OpDPdx) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdx;
};

class OpDPdy : public OpResult {
 public:
  OpDPdy(OpCode const &other) : OpResult(other, spv::OpDPdy) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdy;
};

class OpFwidth : public OpResult {
 public:
  OpFwidth(OpCode const &other) : OpResult(other, spv::OpFwidth) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpFwidth;
};

class OpDPdxFine : public OpResult {
 public:
  OpDPdxFine(OpCode const &other) : OpResult(other, spv::OpDPdxFine) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdxFine;
};

class OpDPdyFine : public OpResult {
 public:
  OpDPdyFine(OpCode const &other) : OpResult(other, spv::OpDPdyFine) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdyFine;
};

class OpFwidthFine : public OpResult {
 public:
  OpFwidthFine(OpCode const &other) : OpResult(other, spv::OpFwidthFine) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpFwidthFine;
};

class OpDPdxCoarse : public OpResult {
 public:
  OpDPdxCoarse(OpCode const &other) : OpResult(other, spv::OpDPdxCoarse) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdxCoarse;
};

class OpDPdyCoarse : public OpResult {
 public:
  OpDPdyCoarse(OpCode const &other) : OpResult(other, spv::OpDPdyCoarse) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpDPdyCoarse;
};

class OpFwidthCoarse : public OpResult {
 public:
  OpFwidthCoarse(OpCode const &other) : OpResult(other, spv::OpFwidthCoarse) {}
  spv::Id P() const;
  static const spv::Op ClassCode = spv::OpFwidthCoarse;
};

class OpEmitVertex : public OpCode {
 public:
  OpEmitVertex(OpCode const &other) : OpCode(other, spv::OpEmitVertex) {}
  static const spv::Op ClassCode = spv::OpEmitVertex;
};

class OpEndPrimitive : public OpCode {
 public:
  OpEndPrimitive(OpCode const &other) : OpCode(other, spv::OpEndPrimitive) {}
  static const spv::Op ClassCode = spv::OpEndPrimitive;
};

class OpEmitStreamVertex : public OpCode {
 public:
  OpEmitStreamVertex(OpCode const &other)
      : OpCode(other, spv::OpEmitStreamVertex) {}
  spv::Id Stream() const;
  static const spv::Op ClassCode = spv::OpEmitStreamVertex;
};

class OpEndStreamPrimitive : public OpCode {
 public:
  OpEndStreamPrimitive(OpCode const &other)
      : OpCode(other, spv::OpEndStreamPrimitive) {}
  spv::Id Stream() const;
  static const spv::Op ClassCode = spv::OpEndStreamPrimitive;
};

class OpControlBarrier : public OpCode {
 public:
  OpControlBarrier(OpCode const &other)
      : OpCode(other, spv::OpControlBarrier) {}
  spv::Id Execution() const;
  spv::Id Memory() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpControlBarrier;
};

class OpMemoryBarrier : public OpCode {
 public:
  OpMemoryBarrier(OpCode const &other) : OpCode(other, spv::OpMemoryBarrier) {}
  spv::Id Memory() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpMemoryBarrier;
};

class OpAtomicLoad : public OpResult {
 public:
  OpAtomicLoad(OpCode const &other) : OpResult(other, spv::OpAtomicLoad) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpAtomicLoad;
};

class OpAtomicStore : public OpCode {
 public:
  OpAtomicStore(OpCode const &other) : OpCode(other, spv::OpAtomicStore) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicStore;
};

class OpAtomicExchange : public OpResult {
 public:
  OpAtomicExchange(OpCode const &other)
      : OpResult(other, spv::OpAtomicExchange) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicExchange;
};

class OpAtomicCompareExchange : public OpResult {
 public:
  OpAtomicCompareExchange(OpCode const &other)
      : OpResult(other, spv::OpAtomicCompareExchange) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Equal() const;
  spv::Id Unequal() const;
  spv::Id Value() const;
  spv::Id Comparator() const;
  static const spv::Op ClassCode = spv::OpAtomicCompareExchange;
};

class OpAtomicCompareExchangeWeak : public OpResult {
 public:
  OpAtomicCompareExchangeWeak(OpCode const &other)
      : OpResult(other, spv::OpAtomicCompareExchangeWeak) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Equal() const;
  spv::Id Unequal() const;
  spv::Id Value() const;
  spv::Id Comparator() const;
  static const spv::Op ClassCode = spv::OpAtomicCompareExchangeWeak;
};

class OpAtomicIIncrement : public OpResult {
 public:
  OpAtomicIIncrement(OpCode const &other)
      : OpResult(other, spv::OpAtomicIIncrement) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpAtomicIIncrement;
};

class OpAtomicIDecrement : public OpResult {
 public:
  OpAtomicIDecrement(OpCode const &other)
      : OpResult(other, spv::OpAtomicIDecrement) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpAtomicIDecrement;
};

class OpAtomicIAdd : public OpResult {
 public:
  OpAtomicIAdd(OpCode const &other) : OpResult(other, spv::OpAtomicIAdd) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicIAdd;
};

class OpAtomicISub : public OpResult {
 public:
  OpAtomicISub(OpCode const &other) : OpResult(other, spv::OpAtomicISub) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicISub;
};

class OpAtomicSMin : public OpResult {
 public:
  OpAtomicSMin(OpCode const &other) : OpResult(other, spv::OpAtomicSMin) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicSMin;
};

class OpAtomicUMin : public OpResult {
 public:
  OpAtomicUMin(OpCode const &other) : OpResult(other, spv::OpAtomicUMin) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicUMin;
};

class OpAtomicSMax : public OpResult {
 public:
  OpAtomicSMax(OpCode const &other) : OpResult(other, spv::OpAtomicSMax) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicSMax;
};

class OpAtomicUMax : public OpResult {
 public:
  OpAtomicUMax(OpCode const &other) : OpResult(other, spv::OpAtomicUMax) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicUMax;
};

class OpAtomicFAddEXT : public OpResult {
 public:
  OpAtomicFAddEXT(OpCode const &other)
      : OpResult(other, spv::OpAtomicFAddEXT) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicFAddEXT;
};

class OpAtomicFMinEXT : public OpResult {
 public:
  OpAtomicFMinEXT(OpCode const &other)
      : OpResult(other, spv::OpAtomicFMinEXT) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicFMinEXT;
};

class OpAtomicFMaxEXT : public OpResult {
 public:
  OpAtomicFMaxEXT(OpCode const &other)
      : OpResult(other, spv::OpAtomicFMaxEXT) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicFMaxEXT;
};

class OpAtomicAnd : public OpResult {
 public:
  OpAtomicAnd(OpCode const &other) : OpResult(other, spv::OpAtomicAnd) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicAnd;
};

class OpAtomicOr : public OpResult {
 public:
  OpAtomicOr(OpCode const &other) : OpResult(other, spv::OpAtomicOr) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicOr;
};

class OpAtomicXor : public OpResult {
 public:
  OpAtomicXor(OpCode const &other) : OpResult(other, spv::OpAtomicXor) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpAtomicXor;
};

class OpPhi : public OpResult {
 public:
  OpPhi(OpCode const &other) : OpResult(other, spv::OpPhi) {}
  struct VariableParentT {
    spv::Id Variable;
    spv::Id Parent;
  };
  llvm::SmallVector<VariableParentT, 4> VariableParent() const;
  static const spv::Op ClassCode = spv::OpPhi;
};

class OpLoopMerge : public OpCode {
 public:
  OpLoopMerge(OpCode const &other) : OpCode(other, spv::OpLoopMerge) {}
  spv::Id MergeBlock() const;
  spv::Id ContinueTarget() const;
  uint32_t LoopControl() const;
  static const spv::Op ClassCode = spv::OpLoopMerge;
};

class OpSelectionMerge : public OpCode {
 public:
  OpSelectionMerge(OpCode const &other)
      : OpCode(other, spv::OpSelectionMerge) {}
  spv::Id MergeBlock() const;
  uint32_t SelectionControl() const;
  static const spv::Op ClassCode = spv::OpSelectionMerge;
};

class OpLabel : public OpCode {
 public:
  OpLabel(OpCode const &other) : OpCode(other, spv::OpLabel) {}
  spv::Id IdResult() const;
  static const spv::Op ClassCode = spv::OpLabel;
};

class OpBranch : public OpCode {
 public:
  OpBranch(OpCode const &other) : OpCode(other, spv::OpBranch) {}
  spv::Id TargetLabel() const;
  static const spv::Op ClassCode = spv::OpBranch;
};

class OpBranchConditional : public OpCode {
 public:
  OpBranchConditional(OpCode const &other)
      : OpCode(other, spv::OpBranchConditional) {}
  spv::Id Condition() const;
  spv::Id TrueLabel() const;
  spv::Id FalseLabel() const;
  llvm::SmallVector<uint32_t, 2> BranchWeights() const;
  static const spv::Op ClassCode = spv::OpBranchConditional;
};

class OpSwitch : public OpCode {
 public:
  OpSwitch(OpCode const &other) : OpCode(other, spv::OpSwitch) {}
  spv::Id Selector() const;
  spv::Id Default() const;
  struct TargetT {
    uint64_t Literal;
    spv::Id Label;
  };
  llvm::SmallVector<TargetT, 4> Target(uint16_t literalWords) const;
  static const spv::Op ClassCode = spv::OpSwitch;
};

class OpKill : public OpCode {
 public:
  OpKill(OpCode const &other) : OpCode(other, spv::OpKill) {}
  static const spv::Op ClassCode = spv::OpKill;
};

class OpReturn : public OpCode {
 public:
  OpReturn(OpCode const &other) : OpCode(other, spv::OpReturn) {}
  static const spv::Op ClassCode = spv::OpReturn;
};

class OpReturnValue : public OpCode {
 public:
  OpReturnValue(OpCode const &other) : OpCode(other, spv::OpReturnValue) {}
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpReturnValue;
};

class OpUnreachable : public OpCode {
 public:
  OpUnreachable(OpCode const &other) : OpCode(other, spv::OpUnreachable) {}
  static const spv::Op ClassCode = spv::OpUnreachable;
};

class OpLifetimeStart : public OpCode {
 public:
  OpLifetimeStart(OpCode const &other) : OpCode(other, spv::OpLifetimeStart) {}
  spv::Id Pointer() const;
  uint32_t Size() const;
  static const spv::Op ClassCode = spv::OpLifetimeStart;
};

class OpLifetimeStop : public OpCode {
 public:
  OpLifetimeStop(OpCode const &other) : OpCode(other, spv::OpLifetimeStop) {}
  spv::Id Pointer() const;
  uint32_t Size() const;
  static const spv::Op ClassCode = spv::OpLifetimeStop;
};

class OpGroupAsyncCopy : public OpResult {
 public:
  OpGroupAsyncCopy(OpCode const &other)
      : OpResult(other, spv::OpGroupAsyncCopy) {}
  spv::Id Execution() const;
  spv::Id Destination() const;
  spv::Id Source() const;
  spv::Id NumElements() const;
  spv::Id Stride() const;
  spv::Id Event() const;
  static const spv::Op ClassCode = spv::OpGroupAsyncCopy;
};

class OpGroupWaitEvents : public OpCode {
 public:
  OpGroupWaitEvents(OpCode const &other)
      : OpCode(other, spv::OpGroupWaitEvents) {}
  spv::Id Execution() const;
  spv::Id NumEvents() const;
  spv::Id EventsList() const;
  static const spv::Op ClassCode = spv::OpGroupWaitEvents;
};

class OpGroupAll : public OpResult {
 public:
  OpGroupAll(OpCode const &other) : OpResult(other, spv::OpGroupAll) {}
  spv::Id Execution() const;
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpGroupAll;
};

class OpGroupAny : public OpResult {
 public:
  OpGroupAny(OpCode const &other) : OpResult(other, spv::OpGroupAny) {}
  spv::Id Execution() const;
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpGroupAny;
};

class OpGroupBroadcast : public OpResult {
 public:
  OpGroupBroadcast(OpCode const &other)
      : OpResult(other, spv::OpGroupBroadcast) {}
  spv::Id Execution() const;
  spv::Id Value() const;
  spv::Id LocalId() const;
  static const spv::Op ClassCode = spv::OpGroupBroadcast;
};

template <enum spv::Op opcode>
class OpGroupOperation : public OpResult {
 public:
  OpGroupOperation(OpCode const &other) : OpResult(other, opcode) {}
  spv::Id Execution() const { return getValueAtOffset(3); }
  spv::GroupOperation Operation() const {
    return static_cast<spv::GroupOperation>(getValueAtOffset(4));
  }
  spv::Id X() const { return getValueAtOffset(5); }
  static const spv::Op ClassCode = opcode;
};

class OpGroupIAdd : public OpGroupOperation<spv::OpGroupIAdd> {
 public:
  OpGroupIAdd(OpCode const &other) : OpGroupOperation(other) {}
};
class OpGroupFAdd : public OpGroupOperation<spv::OpGroupFAdd> {
 public:
  OpGroupFAdd(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupFMin : public OpGroupOperation<spv::OpGroupFMin> {
 public:
  OpGroupFMin(OpCode const &other) : OpGroupOperation(other) {}
};
class OpGroupUMin : public OpGroupOperation<spv::OpGroupUMin> {
 public:
  OpGroupUMin(OpCode const &other) : OpGroupOperation(other) {}
};
class OpGroupSMin : public OpGroupOperation<spv::OpGroupSMin> {
 public:
  OpGroupSMin(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupFMax : public OpGroupOperation<spv::OpGroupFMax> {
 public:
  OpGroupFMax(OpCode const &other) : OpGroupOperation(other) {}
};
class OpGroupUMax : public OpGroupOperation<spv::OpGroupUMax> {
 public:
  OpGroupUMax(OpCode const &other) : OpGroupOperation(other) {}
};
class OpGroupSMax : public OpGroupOperation<spv::OpGroupSMax> {
 public:
  OpGroupSMax(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupIMulKHR : public OpGroupOperation<spv::OpGroupIMulKHR> {
 public:
  OpGroupIMulKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupFMulKHR : public OpGroupOperation<spv::OpGroupFMulKHR> {
 public:
  OpGroupFMulKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupBitwiseAndKHR
    : public OpGroupOperation<spv::OpGroupBitwiseAndKHR> {
 public:
  OpGroupBitwiseAndKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupBitwiseOrKHR : public OpGroupOperation<spv::OpGroupBitwiseOrKHR> {
 public:
  OpGroupBitwiseOrKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupBitwiseXorKHR
    : public OpGroupOperation<spv::OpGroupBitwiseXorKHR> {
 public:
  OpGroupBitwiseXorKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupLogicalAndKHR
    : public OpGroupOperation<spv::OpGroupLogicalAndKHR> {
 public:
  OpGroupLogicalAndKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupLogicalOrKHR : public OpGroupOperation<spv::OpGroupLogicalOrKHR> {
 public:
  OpGroupLogicalOrKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpGroupLogicalXorKHR
    : public OpGroupOperation<spv::OpGroupLogicalXorKHR> {
 public:
  OpGroupLogicalXorKHR(OpCode const &other) : OpGroupOperation(other) {}
};

class OpSubgroupShuffle : public OpResult {
 public:
  OpSubgroupShuffle(OpCode const &other)
      : OpResult(other, spv::OpSubgroupShuffleINTEL) {}
  spv::Id Data() const { return getValueAtOffset(3); }
  spv::Id InvocationId() const { return getValueAtOffset(4); }
  static const spv::Op ClassCode = spv::OpSubgroupShuffleINTEL;
};

class OpSubgroupShuffleUp : public OpResult {
 public:
  OpSubgroupShuffleUp(OpCode const &other)
      : OpResult(other, spv::OpSubgroupShuffleUpINTEL) {}
  spv::Id Previous() const { return getValueAtOffset(3); }
  spv::Id Current() const { return getValueAtOffset(4); }
  spv::Id Delta() const { return getValueAtOffset(5); }
  static const spv::Op ClassCode = spv::OpSubgroupShuffleUpINTEL;
};

class OpSubgroupShuffleDown : public OpResult {
 public:
  OpSubgroupShuffleDown(OpCode const &other)
      : OpResult(other, spv::OpSubgroupShuffleDownINTEL) {}
  spv::Id Current() const { return getValueAtOffset(3); }
  spv::Id Next() const { return getValueAtOffset(4); }
  spv::Id Delta() const { return getValueAtOffset(5); }
  static const spv::Op ClassCode = spv::OpSubgroupShuffleDownINTEL;
};

class OpSubgroupShuffleXor : public OpResult {
 public:
  OpSubgroupShuffleXor(OpCode const &other)
      : OpResult(other, spv::OpSubgroupShuffleXorINTEL) {}
  spv::Id Data() const { return getValueAtOffset(3); }
  spv::Id Value() const { return getValueAtOffset(4); }
  static const spv::Op ClassCode = spv::OpSubgroupShuffleXorINTEL;
};

class OpReadPipe : public OpResult {
 public:
  OpReadPipe(OpCode const &other) : OpResult(other, spv::OpReadPipe) {}
  spv::Id Pipe() const;
  spv::Id Pointer() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpReadPipe;
};

class OpWritePipe : public OpResult {
 public:
  OpWritePipe(OpCode const &other) : OpResult(other, spv::OpWritePipe) {}
  spv::Id Pipe() const;
  spv::Id Pointer() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpWritePipe;
};

class OpReservedReadPipe : public OpResult {
 public:
  OpReservedReadPipe(OpCode const &other)
      : OpResult(other, spv::OpReservedReadPipe) {}
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id Index() const;
  spv::Id Pointer() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpReservedReadPipe;
};

class OpReservedWritePipe : public OpResult {
 public:
  OpReservedWritePipe(OpCode const &other)
      : OpResult(other, spv::OpReservedWritePipe) {}
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id Index() const;
  spv::Id Pointer() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpReservedWritePipe;
};

class OpReserveReadPipePackets : public OpResult {
 public:
  OpReserveReadPipePackets(OpCode const &other)
      : OpResult(other, spv::OpReserveReadPipePackets) {}
  spv::Id Pipe() const;
  spv::Id NumPackets() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpReserveReadPipePackets;
};

class OpReserveWritePipePackets : public OpResult {
 public:
  OpReserveWritePipePackets(OpCode const &other)
      : OpResult(other, spv::OpReserveWritePipePackets) {}
  spv::Id Pipe() const;
  spv::Id NumPackets() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpReserveWritePipePackets;
};

class OpCommitReadPipe : public OpCode {
 public:
  OpCommitReadPipe(OpCode const &other)
      : OpCode(other, spv::OpCommitReadPipe) {}
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpCommitReadPipe;
};

class OpCommitWritePipe : public OpCode {
 public:
  OpCommitWritePipe(OpCode const &other)
      : OpCode(other, spv::OpCommitWritePipe) {}
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpCommitWritePipe;
};

class OpIsValidReserveId : public OpResult {
 public:
  OpIsValidReserveId(OpCode const &other)
      : OpResult(other, spv::OpIsValidReserveId) {}
  spv::Id ReserveId() const;
  static const spv::Op ClassCode = spv::OpIsValidReserveId;
};

class OpGetNumPipePackets : public OpResult {
 public:
  OpGetNumPipePackets(OpCode const &other)
      : OpResult(other, spv::OpGetNumPipePackets) {}
  spv::Id Pipe() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGetNumPipePackets;
};

class OpGetMaxPipePackets : public OpResult {
 public:
  OpGetMaxPipePackets(OpCode const &other)
      : OpResult(other, spv::OpGetMaxPipePackets) {}
  spv::Id Pipe() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGetMaxPipePackets;
};

class OpGroupReserveReadPipePackets : public OpResult {
 public:
  OpGroupReserveReadPipePackets(OpCode const &other)
      : OpResult(other, spv::OpGroupReserveReadPipePackets) {}
  spv::Id Execution() const;
  spv::Id Pipe() const;
  spv::Id NumPackets() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGroupReserveReadPipePackets;
};

class OpGroupReserveWritePipePackets : public OpResult {
 public:
  OpGroupReserveWritePipePackets(OpCode const &other)
      : OpResult(other, spv::OpGroupReserveWritePipePackets) {}
  spv::Id Execution() const;
  spv::Id Pipe() const;
  spv::Id NumPackets() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGroupReserveWritePipePackets;
};

class OpGroupCommitReadPipe : public OpCode {
 public:
  OpGroupCommitReadPipe(OpCode const &other)
      : OpCode(other, spv::OpGroupCommitReadPipe) {}
  spv::Id Execution() const;
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGroupCommitReadPipe;
};

class OpGroupCommitWritePipe : public OpCode {
 public:
  OpGroupCommitWritePipe(OpCode const &other)
      : OpCode(other, spv::OpGroupCommitWritePipe) {}
  spv::Id Execution() const;
  spv::Id Pipe() const;
  spv::Id ReserveId() const;
  spv::Id PacketSize() const;
  spv::Id PacketAlignment() const;
  static const spv::Op ClassCode = spv::OpGroupCommitWritePipe;
};

class OpEnqueueMarker : public OpResult {
 public:
  OpEnqueueMarker(OpCode const &other)
      : OpResult(other, spv::OpEnqueueMarker) {}
  spv::Id Queue() const;
  spv::Id NumEvents() const;
  spv::Id WaitEvents() const;
  spv::Id RetEvent() const;
  static const spv::Op ClassCode = spv::OpEnqueueMarker;
};

class OpEnqueueKernel : public OpResult {
 public:
  OpEnqueueKernel(OpCode const &other)
      : OpResult(other, spv::OpEnqueueKernel) {}
  spv::Id Queue() const;
  spv::Id Flags() const;
  spv::Id NDRange() const;
  spv::Id NumEvents() const;
  spv::Id WaitEvents() const;
  spv::Id RetEvent() const;
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  llvm::SmallVector<spv::Id, 3> LocalSize() const;
  static const spv::Op ClassCode = spv::OpEnqueueKernel;
};

class OpGetKernelNDrangeSubGroupCount : public OpResult {
 public:
  OpGetKernelNDrangeSubGroupCount(OpCode const &other)
      : OpResult(other, spv::OpGetKernelNDrangeSubGroupCount) {}
  spv::Id NDRange() const;
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode = spv::OpGetKernelNDrangeSubGroupCount;
};

class OpGetKernelNDrangeMaxSubGroupSize : public OpResult {
 public:
  OpGetKernelNDrangeMaxSubGroupSize(OpCode const &other)
      : OpResult(other, spv::OpGetKernelNDrangeMaxSubGroupSize) {}
  spv::Id NDRange() const;
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode = spv::OpGetKernelNDrangeMaxSubGroupSize;
};

class OpGetKernelWorkGroupSize : public OpResult {
 public:
  OpGetKernelWorkGroupSize(OpCode const &other)
      : OpResult(other, spv::OpGetKernelWorkGroupSize) {}
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode = spv::OpGetKernelWorkGroupSize;
};

class OpGetKernelPreferredWorkGroupSizeMultiple : public OpResult {
 public:
  OpGetKernelPreferredWorkGroupSizeMultiple(OpCode const &other)
      : OpResult(other, spv::OpGetKernelPreferredWorkGroupSizeMultiple) {}
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode =
      spv::OpGetKernelPreferredWorkGroupSizeMultiple;
};

class OpRetainEvent : public OpCode {
 public:
  OpRetainEvent(OpCode const &other) : OpCode(other, spv::OpRetainEvent) {}
  spv::Id Event() const;
  static const spv::Op ClassCode = spv::OpRetainEvent;
};

class OpReleaseEvent : public OpCode {
 public:
  OpReleaseEvent(OpCode const &other) : OpCode(other, spv::OpReleaseEvent) {}
  spv::Id Event() const;
  static const spv::Op ClassCode = spv::OpReleaseEvent;
};

class OpCreateUserEvent : public OpResult {
 public:
  OpCreateUserEvent(OpCode const &other)
      : OpResult(other, spv::OpCreateUserEvent) {}
  static const spv::Op ClassCode = spv::OpCreateUserEvent;
};

class OpIsValidEvent : public OpResult {
 public:
  OpIsValidEvent(OpCode const &other) : OpResult(other, spv::OpIsValidEvent) {}
  spv::Id Event() const;
  static const spv::Op ClassCode = spv::OpIsValidEvent;
};

class OpSetUserEventStatus : public OpCode {
 public:
  OpSetUserEventStatus(OpCode const &other)
      : OpCode(other, spv::OpSetUserEventStatus) {}
  spv::Id Event() const;
  spv::Id Status() const;
  static const spv::Op ClassCode = spv::OpSetUserEventStatus;
};

class OpCaptureEventProfilingInfo : public OpCode {
 public:
  OpCaptureEventProfilingInfo(OpCode const &other)
      : OpCode(other, spv::OpCaptureEventProfilingInfo) {}
  spv::Id Event() const;
  spv::Id ProfilingInfo() const;
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpCaptureEventProfilingInfo;
};

class OpGetDefaultQueue : public OpResult {
 public:
  OpGetDefaultQueue(OpCode const &other)
      : OpResult(other, spv::OpGetDefaultQueue) {}
  static const spv::Op ClassCode = spv::OpGetDefaultQueue;
};

class OpBuildNDRange : public OpResult {
 public:
  OpBuildNDRange(OpCode const &other) : OpResult(other, spv::OpBuildNDRange) {}
  spv::Id GlobalWorkSize() const;
  spv::Id LocalWorkSize() const;
  spv::Id GlobalWorkOffset() const;
  static const spv::Op ClassCode = spv::OpBuildNDRange;
};

class OpGetKernelLocalSizeForSubgroupCount : public OpResult {
 public:
  OpGetKernelLocalSizeForSubgroupCount(OpCode const &other)
      : OpResult(other, spv::OpGetKernelLocalSizeForSubgroupCount) {}
  spv::Id SubgroupCount() const;
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode = spv::OpGetKernelLocalSizeForSubgroupCount;
};

class OpGetKernelMaxNumSubgroups : public OpResult {
 public:
  OpGetKernelMaxNumSubgroups(OpCode const &other)
      : OpResult(other, spv::OpGetKernelMaxNumSubgroups) {}
  spv::Id Invoke() const;
  spv::Id Param() const;
  spv::Id ParamSize() const;
  spv::Id ParamAlign() const;
  static const spv::Op ClassCode = spv::OpGetKernelMaxNumSubgroups;
};

class OpImageSparseSampleImplicitLod : public OpResult {
 public:
  OpImageSparseSampleImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleImplicitLod;
};

class OpImageSparseSampleExplicitLod : public OpResult {
 public:
  OpImageSparseSampleExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleExplicitLod;
};

class OpImageSparseSampleDrefImplicitLod : public OpResult {
 public:
  OpImageSparseSampleDrefImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleDrefImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleDrefImplicitLod;
};

class OpImageSparseSampleDrefExplicitLod : public OpResult {
 public:
  OpImageSparseSampleDrefExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleDrefExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleDrefExplicitLod;
};

class OpImageSparseSampleProjImplicitLod : public OpResult {
 public:
  OpImageSparseSampleProjImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleProjImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleProjImplicitLod;
};

class OpImageSparseSampleProjExplicitLod : public OpResult {
 public:
  OpImageSparseSampleProjExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleProjExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleProjExplicitLod;
};

class OpImageSparseSampleProjDrefImplicitLod : public OpResult {
 public:
  OpImageSparseSampleProjDrefImplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleProjDrefImplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleProjDrefImplicitLod;
};

class OpImageSparseSampleProjDrefExplicitLod : public OpResult {
 public:
  OpImageSparseSampleProjDrefExplicitLod(OpCode const &other)
      : OpResult(other, spv::OpImageSparseSampleProjDrefExplicitLod) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseSampleProjDrefExplicitLod;
};

class OpImageSparseFetch : public OpResult {
 public:
  OpImageSparseFetch(OpCode const &other)
      : OpResult(other, spv::OpImageSparseFetch) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseFetch;
};

class OpImageSparseGather : public OpResult {
 public:
  OpImageSparseGather(OpCode const &other)
      : OpResult(other, spv::OpImageSparseGather) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Component() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseGather;
};

class OpImageSparseDrefGather : public OpResult {
 public:
  OpImageSparseDrefGather(OpCode const &other)
      : OpResult(other, spv::OpImageSparseDrefGather) {}
  spv::Id SampledImage() const;
  spv::Id Coordinate() const;
  spv::Id Dref() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseDrefGather;
};

class OpImageSparseTexelsResident : public OpResult {
 public:
  OpImageSparseTexelsResident(OpCode const &other)
      : OpResult(other, spv::OpImageSparseTexelsResident) {}
  spv::Id ResidentCode() const;
  static const spv::Op ClassCode = spv::OpImageSparseTexelsResident;
};

class OpNoLine : public OpCode {
 public:
  OpNoLine(OpCode const &other) : OpCode(other, spv::OpNoLine) {}
  static const spv::Op ClassCode = spv::OpNoLine;
};

class OpAtomicFlagTestAndSet : public OpResult {
 public:
  OpAtomicFlagTestAndSet(OpCode const &other)
      : OpResult(other, spv::OpAtomicFlagTestAndSet) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpAtomicFlagTestAndSet;
};

class OpAtomicFlagClear : public OpCode {
 public:
  OpAtomicFlagClear(OpCode const &other)
      : OpCode(other, spv::OpAtomicFlagClear) {}
  spv::Id Pointer() const;
  spv::Id Scope() const;
  spv::Id Semantics() const;
  static const spv::Op ClassCode = spv::OpAtomicFlagClear;
};

class OpImageSparseRead : public OpResult {
 public:
  OpImageSparseRead(OpCode const &other)
      : OpResult(other, spv::OpImageSparseRead) {}
  spv::Id Image() const;
  spv::Id Coordinate() const;
  uint32_t ImageOperands() const;
  static const spv::Op ClassCode = spv::OpImageSparseRead;
};

class OpSubgroupBallotKHR : public OpResult {
 public:
  OpSubgroupBallotKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupBallotKHR) {}
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpSubgroupBallotKHR;
};

class OpSubgroupFirstInvocationKHR : public OpResult {
 public:
  OpSubgroupFirstInvocationKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupFirstInvocationKHR) {}
  spv::Id Value() const;
  static const spv::Op ClassCode = spv::OpSubgroupFirstInvocationKHR;
};

class OpSubgroupAllKHR : public OpResult {
 public:
  OpSubgroupAllKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupAllKHR) {}
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpSubgroupAllKHR;
};

class OpSubgroupAnyKHR : public OpResult {
 public:
  OpSubgroupAnyKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupAnyKHR) {}
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpSubgroupAnyKHR;
};

class OpSubgroupAllEqualKHR : public OpResult {
 public:
  OpSubgroupAllEqualKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupAllEqualKHR) {}
  spv::Id Predicate() const;
  static const spv::Op ClassCode = spv::OpSubgroupAllEqualKHR;
};

class OpSubgroupReadInvocationKHR : public OpResult {
 public:
  OpSubgroupReadInvocationKHR(OpCode const &other)
      : OpResult(other, spv::OpSubgroupReadInvocationKHR) {}
  spv::Id Value() const;
  spv::Id Index() const;
  static const spv::Op ClassCode = spv::OpSubgroupReadInvocationKHR;
};

/// @brief extended instruction operand names.
enum ExtInstArg {
  A,
  B,
  C,
  COSVAL,
  DATA,
  DEGREES,
  EDGE,
  EDGE0,
  EDGE1,
  ETA,
  EXP,
  HI,
  I,
  INTERPOLANT,
  IPTR,
  K,
  LO,
  MAXVAL,
  MINVAL,
  MODE,
  N,
  NANCODE,
  NREF,
  NUM_ELEMENTS,
  OFFSET,
  P,
  P0,
  P1,
  PTR,
  QUO,
  RADIANS,
  SAMPLER,
  SHUFFLEMASK,
  SIGNP,
  V,
  VALUE,
  X,
  Y,
  Y_OVER_X,
  Z
};

/// @brief Primary extended instruction class template.
///
/// @tparam Operands `ExtInstArg`s representing operands of the instruction.
template <ExtInstArg... Operands>
class ExtInst;

template <>
class ExtInst<DEGREES> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id degrees() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<INTERPOLANT> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id interpolant() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<NANCODE> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id nanCode() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<P> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id p() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<RADIANS> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id radians() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<V> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id v() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<VALUE> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id value() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<X> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<Y_OVER_X> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id yOverX() const { return getValueAtOffset(5); }
};

template <>
class ExtInst<EDGE, X> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id edge() const { return getValueAtOffset(5); }
  spv::Id x() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<HI, LO> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id hi() const { return getValueAtOffset(5); }
  spv::Id lo() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<I, N> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id i() const { return getValueAtOffset(5); }
  spv::Id n() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<INTERPOLANT, OFFSET> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id interpolant() const { return getValueAtOffset(5); }
  spv::Id offset() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<INTERPOLANT, SAMPLER> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id interpolant() const { return getValueAtOffset(5); }
  spv::Id sampler() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<OFFSET, P> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id offset() const { return getValueAtOffset(5); }
  spv::Id p() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<P0, P1> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id p0() const { return getValueAtOffset(5); }
  spv::Id p1() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<PTR, NUM_ELEMENTS> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id ptr() const { return getValueAtOffset(5); }
  spv::Id numElements() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<V, I> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id v() const { return getValueAtOffset(5); }
  spv::Id i() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, COSVAL> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id cosVal() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, EXP> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id exp() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, I> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id i() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, IPTR> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id iPtr() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, K> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id k() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, PTR> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id ptr() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, SHUFFLEMASK> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id shuffleMask() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, SIGNP> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id signp() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<X, Y> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id y() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<Y, X> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id y() const { return getValueAtOffset(5); }
  spv::Id x() const { return getValueAtOffset(6); }
};

template <>
class ExtInst<A, B, C> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id a() const { return getValueAtOffset(5); }
  spv::Id b() const { return getValueAtOffset(6); }
  spv::Id c() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<DATA, OFFSET, P> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id data() const { return getValueAtOffset(5); }
  spv::Id offset() const { return getValueAtOffset(6); }
  spv::Id p() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<EDGE0, EDGE1, X> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id edge0() const { return getValueAtOffset(5); }
  spv::Id edge1() const { return getValueAtOffset(6); }
  spv::Id x() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<I, N, ETA> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id i() const { return getValueAtOffset(5); }
  spv::Id n() const { return getValueAtOffset(6); }
  spv::Id eta() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<N, I, NREF> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id n() const { return getValueAtOffset(5); }
  spv::Id i() const { return getValueAtOffset(6); }
  spv::Id nRef() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<OFFSET, P, N> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id offset() const { return getValueAtOffset(5); }
  spv::Id p() const { return getValueAtOffset(6); }
  spv::Id n() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<X, MINVAL, MAXVAL> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id minVal() const { return getValueAtOffset(6); }
  spv::Id maxVal() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<X, Y, A> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id y() const { return getValueAtOffset(6); }
  spv::Id a() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<X, Y, QUO> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id y() const { return getValueAtOffset(6); }
  spv::Id quo() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<X, Y, SHUFFLEMASK> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id y() const { return getValueAtOffset(6); }
  spv::Id shuffleMask() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<X, Y, Z> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id x() const { return getValueAtOffset(5); }
  spv::Id y() const { return getValueAtOffset(6); }
  spv::Id z() const { return getValueAtOffset(7); }
};

template <>
class ExtInst<DATA, OFFSET, P, MODE> : public OpExtInst {
 public:
  ExtInst(OpCode const &other) : OpExtInst(other) {}
  spv::Id data() const { return getValueAtOffset(5); }
  spv::Id offset() const { return getValueAtOffset(6); }
  spv::Id p() const { return getValueAtOffset(7); }
  spv::FPRoundingMode mode() const {
    return static_cast<spv::FPRoundingMode>(getValueAtOffset(8));
  }
};
class OpAssumeTrueKHR : public OpCode {
 public:
  OpAssumeTrueKHR(OpCode const &other) : OpCode(other, spv::OpAssumeTrueKHR) {}
  spv::Id Condition() const;
  static const spv::Op ClassCode = spv::OpAssumeTrueKHR;
};
class OpExpectKHR : public OpResult {
 public:
  OpExpectKHR(OpCode const &other) : OpResult(other, spv::OpExpectKHR) {}
  spv::Id Value() const;
  spv::Id ExpectedValue() const;
  static const spv::Op ClassCode = spv::OpExpectKHR;
};

namespace OpenCLstd {
using Acos = ExtInst<X>;
using Acosh = ExtInst<X>;
using Acospi = ExtInst<X>;
using Asin = ExtInst<X>;
using Asinh = ExtInst<X>;
using Asinpi = ExtInst<X>;
using Atan = ExtInst<X>;
using Atan2 = ExtInst<Y, X>;
using Atanh = ExtInst<X>;
using Atanpi = ExtInst<X>;
using Atan2pi = ExtInst<Y, X>;
using Cbrt = ExtInst<X>;
using Ceil = ExtInst<X>;
using Copysign = ExtInst<X, Y>;
using Cos = ExtInst<X>;
using Cosh = ExtInst<X>;
using Cospi = ExtInst<X>;
using Erfc = ExtInst<X>;
using Erf = ExtInst<X>;
using Exp = ExtInst<X>;
using Exp2 = ExtInst<X>;
using Exp10 = ExtInst<X>;
using Expm1 = ExtInst<X>;
using Fabs = ExtInst<X>;
using Fdim = ExtInst<X, Y>;
using Floor = ExtInst<X>;
using Fma = ExtInst<A, B, C>;
using Fmax = ExtInst<X, Y>;
using Fmin = ExtInst<X, Y>;
using Fmod = ExtInst<X, Y>;
using Fract = ExtInst<X, PTR>;
using Frexp = ExtInst<X, EXP>;
using Hypot = ExtInst<X, Y>;
using Ilogb = ExtInst<X>;
using Ldexp = ExtInst<X, K>;
using Lgamma = ExtInst<X>;
using Lgamma_r = ExtInst<X, SIGNP>;
using Log = ExtInst<X>;
using Log2 = ExtInst<X>;
using Log10 = ExtInst<X>;
using Log1p = ExtInst<X>;
using Logb = ExtInst<X>;
using Mad = ExtInst<A, B, C>;
using Maxmag = ExtInst<X, Y>;
using Minmag = ExtInst<X, Y>;
using Modf = ExtInst<X, IPTR>;
using Nan = ExtInst<NANCODE>;
using Nextafter = ExtInst<X, Y>;
using Pow = ExtInst<X, Y>;
using Pown = ExtInst<X, Y>;
using Powr = ExtInst<X, Y>;
using Remainder = ExtInst<X, Y>;
using Remquo = ExtInst<X, Y, QUO>;
using Rint = ExtInst<X>;
using Rootn = ExtInst<X, Y>;
using Round = ExtInst<X>;
using Rsqrt = ExtInst<X>;
using Sin = ExtInst<X>;
using Sincos = ExtInst<X, COSVAL>;
using Sinh = ExtInst<X>;
using Sinpi = ExtInst<X>;
using Sqrt = ExtInst<X>;
using Tan = ExtInst<X>;
using Tanh = ExtInst<X>;
using Tanpi = ExtInst<X>;
using Tgamma = ExtInst<X>;
using Trunc = ExtInst<X>;
using Half_cos = ExtInst<X>;
using Half_divide = ExtInst<X, Y>;
using Half_exp = ExtInst<X>;
using Half_exp2 = ExtInst<X>;
using Half_exp10 = ExtInst<X>;
using Half_log = ExtInst<X>;
using Half_log2 = ExtInst<X>;
using Half_log10 = ExtInst<X>;
using Half_powr = ExtInst<X, Y>;
using Half_recip = ExtInst<X>;
using Half_rsqrt = ExtInst<X>;
using Half_sin = ExtInst<X>;
using Half_sqrt = ExtInst<X>;
using Half_tan = ExtInst<X>;
using Native_cos = ExtInst<X>;
using Native_divide = ExtInst<X, Y>;
using Native_exp = ExtInst<X>;
using Native_exp2 = ExtInst<X>;
using Native_exp10 = ExtInst<X>;
using Native_log = ExtInst<X>;
using Native_log2 = ExtInst<X>;
using Native_log10 = ExtInst<X>;
using Native_powr = ExtInst<X, Y>;
using Native_recip = ExtInst<X>;
using Native_rsqrt = ExtInst<X>;
using Native_sin = ExtInst<X>;
using Native_sqrt = ExtInst<X>;
using Native_tan = ExtInst<X>;
using S_abs = ExtInst<X>;
using S_abs_diff = ExtInst<X, Y>;
using S_add_sat = ExtInst<X, Y>;
using U_add_sat = ExtInst<X, Y>;
using S_hadd = ExtInst<X, Y>;
using U_hadd = ExtInst<X, Y>;
using S_rhadd = ExtInst<X, Y>;
using U_rhadd = ExtInst<X, Y>;
using S_clamp = ExtInst<X, MINVAL, MAXVAL>;
using U_clamp = ExtInst<X, MINVAL, MAXVAL>;
using Clz = ExtInst<X>;
using Ctz = ExtInst<X>;
using S_mad_hi = ExtInst<A, B, C>;
using U_mad_sat = ExtInst<X, Y, Z>;
using S_mad_sat = ExtInst<X, Y, Z>;
using S_max = ExtInst<X, Y>;
using U_max = ExtInst<X, Y>;
using S_min = ExtInst<X, Y>;
using U_min = ExtInst<X, Y>;
using S_mul_hi = ExtInst<X, Y>;
using Rotate = ExtInst<V, I>;
using S_sub_sat = ExtInst<X, Y>;
using U_sub_sat = ExtInst<X, Y>;
using U_upsample = ExtInst<HI, LO>;
using S_upsample = ExtInst<HI, LO>;
using Popcount = ExtInst<X>;
using S_mad24 = ExtInst<X, Y, Z>;
using U_mad24 = ExtInst<X, Y, Z>;
using S_mul24 = ExtInst<X, Y>;
using U_mul24 = ExtInst<X, Y>;
using U_abs = ExtInst<X>;
using U_abs_diff = ExtInst<X, Y>;
using U_mul_hi = ExtInst<X, Y>;
using U_mad_hi = ExtInst<A, B, C>;
using Fclamp = ExtInst<X, MINVAL, MAXVAL>;
using Degrees = ExtInst<RADIANS>;
using Fmax_common = ExtInst<X, Y>;
using Fmin_common = ExtInst<X, Y>;
using Mix = ExtInst<X, Y, A>;
using Radians = ExtInst<DEGREES>;
using Step = ExtInst<EDGE, X>;
using Smoothstep = ExtInst<EDGE0, EDGE1, X>;
using Sign = ExtInst<X>;
using Cross = ExtInst<P0, P1>;
using Distance = ExtInst<P0, P1>;
using Length = ExtInst<P>;
using Normalize = ExtInst<P>;
using Fast_distance = ExtInst<P0, P1>;
using Fast_length = ExtInst<P>;
using Fast_normalize = ExtInst<P>;
using Bitselect = ExtInst<A, B, C>;
using Select = ExtInst<A, B, C>;
using Vloadn = ExtInst<OFFSET, P, N>;
using Vstoren = ExtInst<DATA, OFFSET, P>;
using Vload_half = ExtInst<OFFSET, P>;
using Vload_halfn = ExtInst<OFFSET, P, N>;
using Vstore_half = ExtInst<DATA, OFFSET, P>;
using Vstore_half_r = ExtInst<DATA, OFFSET, P, MODE>;
using Vstore_halfn = ExtInst<DATA, OFFSET, P>;
using Vstore_halfn_r = ExtInst<DATA, OFFSET, P, MODE>;
using Vloada_halfn = ExtInst<OFFSET, P, N>;
using Vstorea_halfn = ExtInst<DATA, OFFSET, P>;
using Vstorea_halfn_r = ExtInst<DATA, OFFSET, P, MODE>;
using Shuffle = ExtInst<X, SHUFFLEMASK>;
using Shuffle2 = ExtInst<X, Y, SHUFFLEMASK>;
using Prefetch = ExtInst<PTR, NUM_ELEMENTS>;

class Printf : public OpExtInst {
 public:
  Printf(OpCode const &other) : OpExtInst(other) {}
  spv::Id format() const;
  llvm::SmallVector<spv::Id, 8> AdditionalArguments() const;
};
}  // namespace OpenCLstd

namespace GLSLstd450 {
using Round = ExtInst<X>;
using RoundEven = ExtInst<X>;
using Trunc = ExtInst<X>;
using FAbs = ExtInst<X>;
using SAbs = ExtInst<X>;
using FSign = ExtInst<X>;
using SSign = ExtInst<X>;
using Floor = ExtInst<X>;
using Ceil = ExtInst<X>;
using Fract = ExtInst<X>;
using Radians = ExtInst<DEGREES>;
using Degrees = ExtInst<RADIANS>;
using Sin = ExtInst<X>;
using Cos = ExtInst<X>;
using Tan = ExtInst<X>;
using Asin = ExtInst<X>;
using Acos = ExtInst<X>;
using Atan = ExtInst<Y_OVER_X>;
using Sinh = ExtInst<X>;
using Cosh = ExtInst<X>;
using Tanh = ExtInst<X>;
using Asinh = ExtInst<X>;
using Acosh = ExtInst<X>;
using Atanh = ExtInst<X>;
using Atan2 = ExtInst<Y, X>;
using Pow = ExtInst<X, Y>;
using Exp = ExtInst<X>;
using Log = ExtInst<X>;
using Exp2 = ExtInst<X>;
using Log2 = ExtInst<X>;
using Sqrt = ExtInst<X>;
using InverseSqrt = ExtInst<X>;
using Determinant = ExtInst<X>;
using MatrixInverse = ExtInst<X>;
using Modf = ExtInst<X, I>;
using ModfStruct = ExtInst<X>;
using FMin = ExtInst<X, Y>;
using UMin = ExtInst<X, Y>;
using SMin = ExtInst<X, Y>;
using FMax = ExtInst<X, Y>;
using UMax = ExtInst<X, Y>;
using SMax = ExtInst<X, Y>;
using FClamp = ExtInst<X, MINVAL, MAXVAL>;
using UClamp = ExtInst<X, MINVAL, MAXVAL>;
using SClamp = ExtInst<X, MINVAL, MAXVAL>;
using FMix = ExtInst<X, Y, A>;
using IMix = ExtInst<X, Y, A>;
using Step = ExtInst<EDGE, X>;
using SmoothStep = ExtInst<EDGE0, EDGE1, X>;
using Fma = ExtInst<A, B, C>;
using Frexp = ExtInst<X, EXP>;
using FrexpStruct = ExtInst<X>;
using Ldexp = ExtInst<X, EXP>;
using PackSnorm4x8 = ExtInst<V>;
using PackUnorm4x8 = ExtInst<V>;
using PackSnorm2x16 = ExtInst<V>;
using PackUnorm2x16 = ExtInst<V>;
using PackHalf2x16 = ExtInst<V>;
using PackDouble2x32 = ExtInst<V>;
using UnpackSnorm2x16 = ExtInst<P>;
using UnpackUnorm2x16 = ExtInst<P>;
using UnpackHalf2x16 = ExtInst<V>;
using UnpackSnorm4x8 = ExtInst<P>;
using UnpackUnorm4x8 = ExtInst<P>;
using UnpackDouble2x32 = ExtInst<V>;
using Length = ExtInst<X>;
using Distance = ExtInst<P0, P1>;
using Cross = ExtInst<X, Y>;
using Normalize = ExtInst<X>;
using FaceForward = ExtInst<N, I, NREF>;
using Reflect = ExtInst<I, N>;
using Refract = ExtInst<I, N, ETA>;
using FindILsb = ExtInst<VALUE>;
using FindSMsb = ExtInst<VALUE>;
using FindUMsb = ExtInst<VALUE>;
using InterpolateAtCentroid = ExtInst<INTERPOLANT>;
using InterpolateAtSample = ExtInst<INTERPOLANT, SAMPLER>;
using InterpolateAtOffset = ExtInst<INTERPOLANT, OFFSET>;
using NMin = ExtInst<X, Y>;
using NMax = ExtInst<X, Y>;
using NClamp = ExtInst<X, MINVAL, MAXVAL>;
};  // namespace GLSLstd450

std::string getCapabilityName(spv::Capability cap);

std::optional<spv::Capability> getCapabilityFromString(const std::string &cap);

}  // namespace spirv_ll

#endif  // SPIRV_OPCODES_H_INCLUDED
