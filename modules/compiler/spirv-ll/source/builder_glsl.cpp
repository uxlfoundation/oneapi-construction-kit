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

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instructions.h>
#include <multi_llvm/vector_type_helper.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/builder_glsl.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv/unified1/GLSL.std.450.h>

namespace spirv_ll {

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

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Round>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Round>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "round", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450RoundEven>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::RoundEven>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "rint", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Trunc>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Trunc>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "trunc", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FAbs>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FAbs>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "fabs", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SAbs>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SAbs>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "abs", retType, MangleInfo::getSigned(op->IdResultType()), {x}, {});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FSign>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FSign>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "sign", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SSign>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SSign>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  /*
    Computes the sign of a signed integer value using the following formula:
        sign(x) = clamp(x, -1, 1)
    Note that sign(0) in this case is 0, as specified by the GLSL standard.
    Constants for +1 and -1 are generated and then calls are made to the
    builtin clamp() functions.
  */

  // retType is an integer vector or scalar type, but we need to find the
  // element type
  llvm::Type *elemType = retType->getScalarType();

  // we need the values +1 and -1 with same scalar type as retType
  llvm::Value *plus1 =
      builder.getIRBuilder().getIntN(elemType->getScalarSizeInBits(), 1);
  llvm::Value *minus1 =
      builder.getIRBuilder().getIntN(elemType->getScalarSizeInBits(), -1);

  // if retType is vector, pack these numbers into vectors
  if (retType->isVectorTy()) {
    const uint32_t num_elements = multi_llvm::getVectorNumElements(retType);
    plus1 = builder.getIRBuilder().CreateVectorSplat(num_elements, plus1);
    minus1 = builder.getIRBuilder().CreateVectorSplat(num_elements, minus1);
  }

  // create the call:
  llvm::Value *result = builder.createMangledBuiltinCall(
      "clamp", retType, MangleInfo::getSigned(op->IdResultType()),
      {x, minus1, plus1}, {});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Floor>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Floor>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "floor", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Ceil>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Ceil>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "ceil", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Fract>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Fract>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  // The builtin function also returns the whole number part through a
  // pointer passed to the function. This number is stored on the stack and
  // not used.
  llvm::Value *discardable = builder.getIRBuilder().CreateAlloca(retType);

  std::string mangledName =
      builder.applyMangledLength("fract") + builder.getMangledFPName(retType);
  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(discardable->getType());
  if (builder.isSubstitutableArgType(retType)) {
    mangledName += "S_";
  } else {
    mangledName += builder.getMangledFPName(retType);
  }

  llvm::CallInst *result =
      builder.createBuiltinCall(mangledName, retType, {x, discardable});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Radians>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Radians>(opc);

  llvm::Value *degrees = module.getValue(op->degrees());
  SPIRV_LL_ASSERT_PTR(degrees);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "radians", retType, op->IdResultType(), {degrees}, {op->degrees()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Degrees>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Degrees>(opc);

  llvm::Value *radians = module.getValue(op->radians());
  SPIRV_LL_ASSERT_PTR(radians);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "degrees", retType, op->IdResultType(), {radians}, {op->radians()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Sin>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Sin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "sin", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Cos>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Cos>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "cos", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Tan>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Tan>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "tan", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Asin>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Asin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "asin", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Acos>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Acos>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "acos", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Atan>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Atan>(opc);

  llvm::Value *yOverX = module.getValue(op->yOverX());
  SPIRV_LL_ASSERT_PTR(yOverX);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "atan", retType, op->IdResultType(), {yOverX}, {op->yOverX()});
  SPIRV_LL_ASSERT_PTR(result);

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Sinh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Sinh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "sinh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Cosh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Cosh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "cosh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Tanh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Tanh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "tanh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Asinh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Asinh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "asinh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Acosh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Acosh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "acosh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Atanh>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Atanh>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "atanh", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Atan2>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Atan2>(opc);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "atan2", retType, op->IdResultType(), {y, x}, {op->y(), op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Pow>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Pow>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "pow", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Exp>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Exp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "exp", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Log>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Log>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "log", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Exp2>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Exp2>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "exp2", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Log2>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Log2>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "log2", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Sqrt>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Sqrt>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "sqrt", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450InverseSqrt>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::InverseSqrt>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "rsqrt", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Determinant>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Determinant>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450MatrixInverse>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::MatrixInverse>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Modf>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Modf>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *i = module.getValue(op->i());
  SPIRV_LL_ASSERT_PTR(i);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "modf", retType, op->IdResultType(), {x, i}, {op->x(), op->i()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450ModfStruct>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FrexpStruct>(opc);

  auto *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  auto *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::Value *wholeNo = builder.getIRBuilder().CreateAlloca(x->getType());

  std::string mangledName = builder.applyMangledLength("modf") +
                            builder.getMangledFPName(x->getType());
  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(wholeNo->getType());
  // Vectors are substituted: scalars are not.
  if (builder.isSubstitutableArgType(x->getType())) {
    mangledName += "S_";
  } else {
    mangledName += builder.getMangledFPName(x->getType());
  }
  llvm::Value *intermediate =
      builder.createBuiltinCall(mangledName, x->getType(), {x, wholeNo});

  llvm::Value *undefResultStruct = llvm::UndefValue::get(retType);
  llvm::Value *resultIntermediate = builder.getIRBuilder().CreateInsertValue(
      undefResultStruct, intermediate, {0});
  llvm::Value *result = builder.getIRBuilder().CreateInsertValue(
      resultIntermediate,
      builder.getIRBuilder().CreateLoad(x->getType(), wholeNo), {1});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FMin>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FMin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "fmin", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UMin>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UMin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "min", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SMin>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SMin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "min", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FMax>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FMax>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "fmax", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UMax>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UMax>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "max", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SMax>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SMax>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "max", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FClamp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FClamp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "clamp", retType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UClamp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UClamp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "clamp", retType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SClamp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SClamp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "clamp", retType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FMix>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FMix>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result =
      builder.createMangledBuiltinCall("mix", retType, op->IdResultType(),
                                       {x, y, a}, {op->x(), op->y(), op->a()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450IMix>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::IMix>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Step>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Step>(opc);

  llvm::Value *edge = module.getValue(op->edge());
  SPIRV_LL_ASSERT_PTR(edge);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "step", retType, op->IdResultType(), {edge, x}, {op->edge(), op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450SmoothStep>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::SmoothStep>(opc);

  llvm::Value *edge0 = module.getValue(op->edge0());
  SPIRV_LL_ASSERT_PTR(edge0);

  llvm::Value *edge1 = module.getValue(op->edge1());
  SPIRV_LL_ASSERT_PTR(edge1);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "smoothstep", retType, op->IdResultType(), {edge0, edge1, x},
      {op->edge0(), op->edge1(), op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Fma>(const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Fma>(opc);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result =
      builder.createMangledBuiltinCall("fma", retType, op->IdResultType(),
                                       {a, b, c}, {op->a(), op->b(), op->c()});
  SPIRV_LL_ASSERT_PTR(result);

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Frexp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Frexp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *exp = module.getValue(op->exp());
  SPIRV_LL_ASSERT_PTR(exp);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  // We can't automatically mangle frexp with our APIs. For the pointer
  // argument, we need to pass OpType to infer the pointer element type but
  // doing so would take its (un)signedness, when in fact we want to force
  // signed;
  //   gentype(n) frexp(gentype(n) x, int(n) *exp)
  std::string mangledName =
      builder.applyMangledLength("frexp") + builder.getMangledFPName(retType);

  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(exp->getType());
  mangledName += builder.getMangledVecPrefixIfVec(x->getType());
  mangledName += "i";

  llvm::Value *result =
      builder.createBuiltinCall(mangledName, retType, {x, exp});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FrexpStruct>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FrexpStruct>(opc);

  auto *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  auto *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::Type *expTy = builder.getIRBuilder().getInt32Ty();
  if (x->getType()->isVectorTy()) {
    expTy = llvm::FixedVectorType::get(
        expTy, multi_llvm::getVectorNumElements(x->getType()));
  }
  llvm::Value *exp = builder.getIRBuilder().CreateAlloca(expTy);

  // We can't automatically mangle frexp with our APIs. For the pointer
  // argument, we need to pass OpType to infer the pointer element type but
  // doing so would take its (un)signedness, when in fact we want to force
  // signed;
  //   gentype(n) frexp(gentype(n) x, int(n) *exp)
  std::string mangledName = builder.applyMangledLength("frexp") +
                            builder.getMangledFPName(x->getType());

  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(exp->getType());
  mangledName += builder.getMangledVecPrefixIfVec(x->getType());
  mangledName += "i";

  llvm::Value *intermediate =
      builder.createBuiltinCall(mangledName, x->getType(), {x, exp});
  llvm::Value *undefResultStruct = llvm::UndefValue::get(retType);
  llvm::Value *resultIntermediate = builder.getIRBuilder().CreateInsertValue(
      undefResultStruct, intermediate, {0});
  llvm::Value *result = builder.getIRBuilder().CreateInsertValue(
      resultIntermediate, builder.getIRBuilder().CreateLoad(expTy, exp), {1});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Ldexp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Ldexp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *exp = module.getValue(op->exp());
  SPIRV_LL_ASSERT_PTR(exp);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  // Don't pass IDs to force signed int mangling. Since CL ldexp can only take
  // signed int abacus only has an overload for signed int, but this has no
  // correctness implications here since exp values of sufficient magnitude
  // (> 1024, < -1022) yield undefined results according to the spec.
  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "ldexp", retType, op->IdResultType(), {x, exp}, {});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackSnorm4x8>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackSnorm4x8>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "packSnorm4x8", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackUnorm4x8>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackUnorm4x8>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "packUnorm4x8", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackSnorm2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackSnorm2x16>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "packSnorm2x16", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackUnorm2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackUnorm2x16>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "packUnorm2x16", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackHalf2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackHalf2x16>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "packHalf2x16", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450PackDouble2x32>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::PackDouble2x32>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::Value *result = builder.getIRBuilder().CreateBitCast(v, retType);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackSnorm2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackSnorm2x16>(opc);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  auto result = builder.createMangledBuiltinCall(
      "unpackSnorm2x16", retType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackUnorm2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackUnorm2x16>(opc);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  auto result = builder.createMangledBuiltinCall(
      "unpackUnorm2x16", retType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackHalf2x16>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackHalf2x16>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "unpackHalf2x16", retType, op->IdResultType(), {v}, {op->v()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackSnorm4x8>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackSnorm4x8>(opc);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  auto result = builder.createMangledBuiltinCall(
      "unpackSnorm4x8", retType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackUnorm4x8>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackUnorm4x8>(opc);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  auto result = builder.createMangledBuiltinCall(
      "unpackUnorm4x8", retType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450UnpackDouble2x32>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::UnpackDouble2x32>(opc);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::Value *result = builder.getIRBuilder().CreateBitCast(v, retType);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Length>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Length>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "length", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Distance>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Distance>(opc);

  llvm::Value *p0 = module.getValue(op->p0());
  SPIRV_LL_ASSERT_PTR(p0);

  llvm::Value *p1 = module.getValue(op->p1());
  SPIRV_LL_ASSERT_PTR(p1);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "distance", retType, op->IdResultType(), {p0, p1}, {op->p0(), op->p1()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Cross>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Cross>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "cross", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Normalize>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Normalize>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "normalize", retType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FaceForward>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FaceForward>(opc);

  llvm::Value *n = module.getValue(op->n());
  SPIRV_LL_ASSERT_PTR(n);

  llvm::Value *i = module.getValue(op->i());
  SPIRV_LL_ASSERT_PTR(i);

  llvm::Value *nRef = module.getValue(op->nRef());
  SPIRV_LL_ASSERT_PTR(nRef);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "faceforward", retType, op->IdResultType(), {n, i, nRef},
      {op->n(), op->i(), op->nRef()});
  SPIRV_LL_ASSERT_PTR(result);

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Reflect>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Reflect>(opc);

  llvm::Value *i = module.getValue(op->i());
  SPIRV_LL_ASSERT_PTR(i);

  llvm::Value *n = module.getValue(op->n());
  SPIRV_LL_ASSERT_PTR(n);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "reflect", retType, op->IdResultType(), {i, n}, {op->i(), op->n()});
  SPIRV_LL_ASSERT_PTR(result);

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450Refract>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::Refract>(opc);

  llvm::Value *i = module.getValue(op->i());
  SPIRV_LL_ASSERT_PTR(i);

  llvm::Value *n = module.getValue(op->n());
  SPIRV_LL_ASSERT_PTR(n);

  llvm::Value *eta = module.getValue(op->eta());
  SPIRV_LL_ASSERT_PTR(eta);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "refract", retType, op->IdResultType(), {i, n, eta},
      {op->i(), op->n(), op->eta()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FindILsb>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FindILsb>(opc);

  llvm::Value *value = module.getValue(op->value());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "findLSB", retType, op->IdResultType(), {value}, {op->value()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FindSMsb>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FindSMsb>(opc);

  llvm::Value *value = module.getValue(op->value());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "findMSB", retType, MangleInfo::getSigned(op->IdResultType()), {value},
      {});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450FindUMsb>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::FindUMsb>(opc);

  llvm::Value *value = module.getValue(op->value());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "findMSB", retType, op->IdResultType(), {value}, {op->value()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450InterpolateAtCentroid>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::InterpolateAtCentroid>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450InterpolateAtSample>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::InterpolateAtSample>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450InterpolateAtOffset>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::InterpolateAtOffset>(opc);

  // Builtin not yet implemented!
  // Update and rerun generate_glsl_builder_calls once implemented.
  (void)op;
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450NMin>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::NMin>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "fmin", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450NMax>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::NMax>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "fmax", retType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::GLSLBuilder::create<GLSLstd450NClamp>(
    const OpExtInst &opc) {
  auto *op = module.create<GLSLstd450::NClamp>(opc);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Type *retType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retType);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "clamp", retType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

#define CASE(ExtInst) \
  case ExtInst:       \
    return create<ExtInst>(opc);

llvm::Error spirv_ll::GLSLBuilder::create(const OpExtInst &opc) {
  switch (opc.Instruction()) {
    CASE(GLSLstd450Round)
    CASE(GLSLstd450RoundEven)
    CASE(GLSLstd450Trunc)
    CASE(GLSLstd450FAbs)
    CASE(GLSLstd450SAbs)
    CASE(GLSLstd450FSign)
    CASE(GLSLstd450SSign)
    CASE(GLSLstd450Floor)
    CASE(GLSLstd450Ceil)
    CASE(GLSLstd450Fract)
    CASE(GLSLstd450Radians)
    CASE(GLSLstd450Degrees)
    CASE(GLSLstd450Sin)
    CASE(GLSLstd450Cos)
    CASE(GLSLstd450Tan)
    CASE(GLSLstd450Asin)
    CASE(GLSLstd450Acos)
    CASE(GLSLstd450Atan)
    CASE(GLSLstd450Sinh)
    CASE(GLSLstd450Cosh)
    CASE(GLSLstd450Tanh)
    CASE(GLSLstd450Asinh)
    CASE(GLSLstd450Acosh)
    CASE(GLSLstd450Atanh)
    CASE(GLSLstd450Atan2)
    CASE(GLSLstd450Pow)
    CASE(GLSLstd450Exp)
    CASE(GLSLstd450Log)
    CASE(GLSLstd450Exp2)
    CASE(GLSLstd450Log2)
    CASE(GLSLstd450Sqrt)
    CASE(GLSLstd450InverseSqrt)
    CASE(GLSLstd450Determinant)
    CASE(GLSLstd450MatrixInverse)
    CASE(GLSLstd450Modf)
    CASE(GLSLstd450ModfStruct)
    CASE(GLSLstd450FMin)
    CASE(GLSLstd450UMin)
    CASE(GLSLstd450SMin)
    CASE(GLSLstd450FMax)
    CASE(GLSLstd450UMax)
    CASE(GLSLstd450SMax)
    CASE(GLSLstd450FClamp)
    CASE(GLSLstd450UClamp)
    CASE(GLSLstd450SClamp)
    CASE(GLSLstd450FMix)
    CASE(GLSLstd450IMix)
    CASE(GLSLstd450Step)
    CASE(GLSLstd450SmoothStep)
    CASE(GLSLstd450Fma)
    CASE(GLSLstd450Frexp)
    CASE(GLSLstd450FrexpStruct)
    CASE(GLSLstd450Ldexp)
    CASE(GLSLstd450PackSnorm4x8)
    CASE(GLSLstd450PackUnorm4x8)
    CASE(GLSLstd450PackSnorm2x16)
    CASE(GLSLstd450PackUnorm2x16)
    CASE(GLSLstd450PackHalf2x16)
    CASE(GLSLstd450PackDouble2x32)
    CASE(GLSLstd450UnpackSnorm2x16)
    CASE(GLSLstd450UnpackUnorm2x16)
    CASE(GLSLstd450UnpackHalf2x16)
    CASE(GLSLstd450UnpackSnorm4x8)
    CASE(GLSLstd450UnpackUnorm4x8)
    CASE(GLSLstd450UnpackDouble2x32)
    CASE(GLSLstd450Length)
    CASE(GLSLstd450Distance)
    CASE(GLSLstd450Cross)
    CASE(GLSLstd450Normalize)
    CASE(GLSLstd450FaceForward)
    CASE(GLSLstd450Reflect)
    CASE(GLSLstd450Refract)
    CASE(GLSLstd450FindILsb)
    CASE(GLSLstd450FindSMsb)
    CASE(GLSLstd450FindUMsb)
    CASE(GLSLstd450InterpolateAtCentroid)
    CASE(GLSLstd450InterpolateAtSample)
    CASE(GLSLstd450InterpolateAtOffset)
    CASE(GLSLstd450NMin)
    CASE(GLSLstd450NMax)
    CASE(GLSLstd450NClamp)
    default:
      return makeStringError(llvm::Twine("Unrecognized extended instruction ") +
                             std::to_string(opc.Instruction()));
  }
}

#undef CASE

}  // namespace spirv_ll
