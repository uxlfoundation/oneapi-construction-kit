// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/optional.h>
#include <llvm/IR/Attributes.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

namespace spirv_ll {

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Acos>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Acos>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "acos", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Acosh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Acosh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "acosh", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Acospi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Acospi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "acospi", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Asin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Asin>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "asin", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Asinh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Asinh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "asinh", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Asinpi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Asinpi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "asinpi", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Atan>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Atan>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "atan", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Atan2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Atan2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *argTwo = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argTwo);

  llvm::Value *result =
      builder.createMangledBuiltinCall("atan2", resultType, op->IdResultType(),
                                       {argOne, argTwo}, {op->y(), op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Atanh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Atanh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "atanh", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Atanpi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Atanpi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "atanpi", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Atan2pi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Atan2pi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *argTwo = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argTwo);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "atan2pi", resultType, op->IdResultType(), {argOne, argTwo},
      {op->y(), op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Cbrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Cbrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "cbrt", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Ceil>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Ceil>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "ceil", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Copysign>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Copysign>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *argTwo = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(argTwo);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "copysign", resultType, op->IdResultType(), {argOne, argTwo},
      {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Cos>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Cos>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "cos", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Cosh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Cosh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "cosh", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Cospi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Cospi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "cospi", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Erfc>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Erfc>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "erfc", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Erf>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Erf>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "erf", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Exp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Exp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "exp", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Exp2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Exp2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "exp2", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Exp10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Exp10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "exp10", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Expm1>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Expm1>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *argOne = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(argOne);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "expm1", resultType, op->IdResultType(), {argOne}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fabs>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fabs>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fabs", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fdim>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fdim>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fdim", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Floor>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Floor>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "floor", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fma>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fma>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result =
      builder.createMangledBuiltinCall("fma", resultType, op->IdResultType(),
                                       {a, b, c}, {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fmax>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fmax>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fmax", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fmin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fmin>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fmin", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fmod>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fmod>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fmod", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fract>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fract>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *ptr = module.getValue(op->ptr());
  SPIRV_LL_ASSERT_PTR(ptr);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fract", resultType, op->IdResultType(), {x, ptr}, {op->x(), op->ptr()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Frexp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Frexp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *exp = module.getValue(op->exp());
  SPIRV_LL_ASSERT_PTR(exp);

  // We can't automatically mangle frexp with our APIs. For the pointer
  // argument, we need to pass OpType to infer the pointer element type but
  // doing so would take its (un)signedness, when in fact we want to force
  // signed;
  //   gentype(n) frexp(gentype(n) x, int(n) *exp)
  std::string mangledName = builder.applyMangledLength("frexp") +
                            builder.getMangledFPName(resultType);

  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(exp->getType());
  mangledName += builder.getMangledVecPrefixIfVec(resultType);
  mangledName += "i";

  llvm::Value *result =
      builder.createBuiltinCall(mangledName, resultType, {x, exp});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Hypot>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Hypot>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "hypot", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Ilogb>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Ilogb>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "ilogb", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Ldexp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Ldexp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *k = module.getValue(op->k());
  SPIRV_LL_ASSERT_PTR(k);

  // Don't pass the IDs to force the mangler to assume signed int, in the CL
  // spec ldexp only operates on (gentype x, intn k).
  llvm::Value *result = builder.createMangledBuiltinCall(
      "ldexp", resultType, op->IdResultType(), {x, k}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Lgamma>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Lgamma>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "lgamma", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Lgamma_r>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Lgamma_r>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *signp = module.getValue(op->signp());
  SPIRV_LL_ASSERT_PTR(signp);

  // We can't automatically mangle lgamma_r with our APIs. For the pointer
  // argument, we need to pass OpType to infer the pointer element type but
  // doing so would take its (un)signedness, when in fact we want to force
  // signed;
  //   gentype(n) lgamma_r(gentype(n) x, int(n) *signp)
  std::string mangledName = builder.applyMangledLength("lgamma_r") +
                            builder.getMangledFPName(resultType);

  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(signp->getType());
  mangledName += builder.getMangledVecPrefixIfVec(x->getType());
  mangledName += "i";

  llvm::Value *result =
      builder.createBuiltinCall(mangledName, resultType, {x, signp});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Log>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Log>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "log", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Log2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Log2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "log2", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Log10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Log10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "log10", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Log1p>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Log1p>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "log1p", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Logb>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Logb>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "logb", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Mad>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Mad>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result =
      builder.createMangledBuiltinCall("mad", resultType, op->IdResultType(),
                                       {a, b, c}, {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Maxmag>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Maxmag>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "maxmag", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Minmag>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Minmag>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "minmag", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Modf>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Modf>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *iPtr = module.getValue(op->iPtr());
  SPIRV_LL_ASSERT_PTR(iPtr);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "modf", resultType, op->IdResultType(), {x, iPtr}, {op->x(), op->iPtr()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Nan>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Nan>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *nanCode = module.getValue(op->nanCode());
  SPIRV_LL_ASSERT_PTR(nanCode);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "nan", resultType, op->IdResultType(), {nanCode}, {op->nanCode()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Nextafter>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Nextafter>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "nextafter", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Pow>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Pow>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "pow", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Pown>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Pown>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  // Don't pass the IDs to force the mangler to assume signed int, in the CL
  // spec pown only operates on (gentype x, intn y).
  llvm::Value *result = builder.createMangledBuiltinCall(
      "pown", resultType, op->IdResultType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Powr>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Powr>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "powr", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Remainder>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Remainder>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "remainder", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Remquo>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Remquo>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *quo = module.getValue(op->quo());
  SPIRV_LL_ASSERT_PTR(quo);

  // We can't automatically mangle remquo with our APIs. For the pointer
  // argument, we need to pass OpType to infer the pointer element type but
  // doing so would take its (un)signedness, when in fact we want to force
  // signed;
  //   gentype(n) remquo(gentype(n) x, gentype(n) y, int(n) *quo)
  std::string mangledName = builder.applyMangledLength("remquo") +
                            builder.getMangledFPName(resultType);
  if (builder.isSubstitutableArgType(resultType)) {
    mangledName += "S_";
  } else {
    mangledName += builder.getMangledFPName(resultType);
  }
  // Mangle the pointer argument.
  mangledName += builder.getMangledPointerPrefix(quo->getType());
  mangledName += builder.getMangledVecPrefixIfVec(x->getType());
  mangledName += "i";

  llvm::Value *result =
      builder.createBuiltinCall(mangledName, resultType, {x, y, quo});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Rint>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Rint>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "rint", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Rootn>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Rootn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  // Don't pass the IDs to force the mangler to assume signed int, in the CL
  // spec rootn only operates on (gentype x, intn y).
  llvm::Value *result = builder.createMangledBuiltinCall(
      "rootn", resultType, op->IdResultType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Round>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Round>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "round", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Rsqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Rsqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "rsqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sin>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sin", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sincos>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sincos>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *cosVal = module.getValue(op->cosVal());
  SPIRV_LL_ASSERT_PTR(cosVal);

  llvm::Value *result =
      builder.createMangledBuiltinCall("sincos", resultType, op->IdResultType(),
                                       {x, cosVal}, {op->x(), op->cosVal()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sinh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sinh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sinh", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sinpi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sinpi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sinpi", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Tan>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Tan>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "tan", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Tanh>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Tanh>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "tanh", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Tanpi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Tanpi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "tanpi", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Tgamma>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Tgamma>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "tgamma", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Trunc>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Trunc>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "trunc", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_cos>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_cos>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_cos", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_divide>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_divide>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_divide", resultType, op->IdResultType(), {x, y},
      {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_exp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_exp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_exp", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_exp2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_exp2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_exp2", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_exp10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_exp10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_exp10", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_log>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_log>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_log", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_log2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_log2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_log2", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_log10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_log10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_log10", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_powr>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_powr>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_powr", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_recip>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_recip>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_recip", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_rsqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_rsqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_rsqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_sin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_sin>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_sin", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_sqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_sqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_sqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Half_tan>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Half_tan>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "half_tan", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_cos>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_cos>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_cos", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_divide>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_divide>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_divide", resultType, op->IdResultType(), {x, y},
      {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_exp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_exp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_exp", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_exp2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_exp2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_exp2", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_exp10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_exp10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_exp10", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_log>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_log>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_log", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_log2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_log2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_log2", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_log10>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_log10>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_log10", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_powr>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_powr>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_powr", resultType, op->IdResultType(), {x, y},
      {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_recip>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_recip>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_recip", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_rsqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_rsqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_rsqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_sin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_sin>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_sin", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_sqrt>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_sqrt>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_sqrt", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Native_tan>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Native_tan>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "native_tan", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SAbs>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_abs>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "abs", resultType, llvm::NoneType(), {x}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SAbs_diff>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_abs_diff>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "abs_diff", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SAdd_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_add_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::CallInst *result = builder.createMangledBuiltinCall(
      "add_sat", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UAdd_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_add_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "add_sat", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SHadd>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_hadd>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "hadd", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UHadd>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_hadd>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "hadd", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SRhadd>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_rhadd>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "rhadd", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::URhadd>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_rhadd>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "rhadd", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SClamp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_clamp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "clamp", resultType, llvm::NoneType(), {x, minVal, maxVal}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UClamp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_clamp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "clamp", resultType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Clz>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Clz>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "clz", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Ctz>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Ctz>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "ctz", resultType, op->IdResultType(), x, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMad_hi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_mad_hi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mad_hi", resultType, llvm::NoneType(), {a, b, c}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMad_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_mad_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *z = module.getValue(op->z());
  SPIRV_LL_ASSERT_PTR(z);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mad_sat", resultType, op->IdResultType(), {x, y, z},
      {op->x(), op->y(), op->z()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMad_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_mad_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *z = module.getValue(op->z());
  SPIRV_LL_ASSERT_PTR(z);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mad_sat", resultType, llvm::NoneType(), {x, y, z}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMax>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_max>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "max", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMax>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_max>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "max", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_min>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "min", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMin>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_min>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "min", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMul_hi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_mul_hi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mul_hi", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Rotate>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Rotate>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *v = module.getValue(op->v());
  SPIRV_LL_ASSERT_PTR(v);

  llvm::Value *i = module.getValue(op->i());
  SPIRV_LL_ASSERT_PTR(i);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "rotate", resultType, op->IdResultType(), {v, i}, {op->v(), op->i()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SSub_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_sub_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sub_sat", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::USub_sat>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_sub_sat>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sub_sat", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::U_Upsample>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_upsample>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *hi = module.getValue(op->hi());
  SPIRV_LL_ASSERT_PTR(hi);

  llvm::Value *lo = module.getValue(op->lo());
  SPIRV_LL_ASSERT_PTR(lo);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "upsample", resultType, op->IdResultType(), {hi, lo},
      {op->hi(), op->lo()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::S_Upsample>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_upsample>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *hi = module.getValue(op->hi());
  SPIRV_LL_ASSERT_PTR(hi);

  llvm::Value *lo = module.getValue(op->lo());
  SPIRV_LL_ASSERT_PTR(lo);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "upsample", resultType, llvm::NoneType(), {hi, lo}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Popcount>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Popcount>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "popcount", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMad24>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_mad24>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *z = module.getValue(op->z());
  SPIRV_LL_ASSERT_PTR(z);

  llvm::Value *result =
      builder.createMangledBuiltinCall("mad24", resultType, op->IdResultType(),
                                       {x, y, z}, {op->x(), op->y(), op->z()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMad24>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_mad24>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *z = module.getValue(op->z());
  SPIRV_LL_ASSERT_PTR(z);

  llvm::Value *result =
      builder.createMangledBuiltinCall("mad24", resultType, op->IdResultType(),
                                       {x, y, z}, {op->x(), op->y(), op->z()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::SMul24>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::S_mul24>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mul24", resultType, llvm::NoneType(), {x, y}, {});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMul24>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_mul24>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mul24", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UAbs>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_abs>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "abs", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UAbs_diff>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_abs_diff>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "abs_diff", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMul_hi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_mul_hi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "mul_hi", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::UMad_hi>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::U_mad_hi>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result =
      builder.createMangledBuiltinCall("mad_hi", resultType, op->IdResultType(),
                                       {a, b, c}, {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::FClamp>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fclamp>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *minVal = module.getValue(op->minVal());
  SPIRV_LL_ASSERT_PTR(minVal);

  llvm::Value *maxVal = module.getValue(op->maxVal());
  SPIRV_LL_ASSERT_PTR(maxVal);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "clamp", resultType, op->IdResultType(), {x, minVal, maxVal},
      {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Degrees>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Degrees>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *radians = module.getValue(op->radians());
  SPIRV_LL_ASSERT_PTR(radians);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "degrees", resultType, op->IdResultType(), {radians}, {op->radians()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::FMax_common>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fmax_common>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "max", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::FMin_common>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fmin_common>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "min", resultType, op->IdResultType(), {x, y}, {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Mix>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Mix>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *result =
      builder.createMangledBuiltinCall("mix", resultType, op->IdResultType(),
                                       {x, y, a}, {op->x(), op->y(), op->a()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Radians>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Radians>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *degrees = module.getValue(op->degrees());
  SPIRV_LL_ASSERT_PTR(degrees);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "radians", resultType, op->IdResultType(), {degrees}, {op->degrees()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Step>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Step>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *edge = module.getValue(op->edge());
  SPIRV_LL_ASSERT_PTR(edge);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "step", resultType, op->IdResultType(), {edge, x}, {op->edge(), op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Smoothstep>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Smoothstep>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *edge0 = module.getValue(op->edge0());
  SPIRV_LL_ASSERT_PTR(edge0);

  llvm::Value *edge1 = module.getValue(op->edge1());
  SPIRV_LL_ASSERT_PTR(edge1);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "smoothstep", resultType, op->IdResultType(), {edge0, edge1, x},
      {op->edge0(), op->edge1(), op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Sign>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Sign>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "sign", resultType, op->IdResultType(), {x}, {op->x()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Cross>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Cross>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p0 = module.getValue(op->p0());
  SPIRV_LL_ASSERT_PTR(p0);

  llvm::Value *p1 = module.getValue(op->p1());
  SPIRV_LL_ASSERT_PTR(p1);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "cross", resultType, op->IdResultType(), {p0, p1}, {op->p0(), op->p1()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Distance>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Distance>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p0 = module.getValue(op->p0());
  SPIRV_LL_ASSERT_PTR(p0);

  llvm::Value *p1 = module.getValue(op->p1());
  SPIRV_LL_ASSERT_PTR(p1);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "distance", resultType, op->IdResultType(), {p0, p1},
      {op->p0(), op->p1()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Length>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Length>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "length", resultType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Normalize>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Normalize>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "normalize", resultType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fast_distance>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fast_distance>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p0 = module.getValue(op->p0());
  SPIRV_LL_ASSERT_PTR(p0);

  llvm::Value *p1 = module.getValue(op->p1());
  SPIRV_LL_ASSERT_PTR(p1);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fast_distance", resultType, op->IdResultType(), {p0, p1},
      {op->p0(), op->p1()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fast_length>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fast_length>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fast_length", resultType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Fast_normalize>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Fast_normalize>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "fast_normalize", resultType, op->IdResultType(), {p}, {op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Bitselect>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Bitselect>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "bitselect", resultType, op->IdResultType(), {a, b, c},
      {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Select>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Select>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *a = module.getValue(op->a());
  SPIRV_LL_ASSERT_PTR(a);

  llvm::Value *b = module.getValue(op->b());
  SPIRV_LL_ASSERT_PTR(b);

  llvm::Value *c = module.getValue(op->c());
  SPIRV_LL_ASSERT_PTR(c);

  llvm::Value *result =
      builder.createMangledBuiltinCall("select", resultType, op->IdResultType(),
                                       {a, b, c}, {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vloadn>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vloadn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  auto result = builder.createVectorDataBuiltinCall(
      "vload", resultType, resultType, op->IdResultType(), {offset, p},
      {op->offset(), op->p()}, llvm::NoneType(), {NONE, CONST});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstoren>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstoren>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstore", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vload_half>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vload_half>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vload_half", resultType, resultType, op->IdResultType(), {offset, p},
      {op->offset(), op->p()}, llvm::NoneType(), {NONE, CONST});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vload_halfn>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vload_halfn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vload_half", resultType, resultType, op->IdResultType(), {offset, p},
      {op->offset(), op->p()}, llvm::NoneType(), {NONE, CONST});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstore_half>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstore_half>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstore_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()});
  SPIRV_LL_ASSERT_PTR(result);

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstore_half_r>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstore_half_r>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstore_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()}, op->mode());

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstore_halfn>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstore_halfn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstore_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstore_halfn_r>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstore_halfn_r>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstore_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()}, op->mode());

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vloada_halfn>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vloada_halfn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vloada_half", resultType, resultType, op->IdResultType(), {offset, p},
      {op->offset(), op->p()}, llvm::NoneType(), {NONE, CONST});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstorea_halfn>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstorea_halfn>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstorea_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Vstorea_halfn_r>(
    OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Vstorea_halfn_r>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *data = module.getValue(op->data());
  SPIRV_LL_ASSERT_PTR(data);

  llvm::Value *offset = module.getValue(op->offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *p = module.getValue(op->p());
  SPIRV_LL_ASSERT_PTR(p);

  llvm::Value *result = builder.createVectorDataBuiltinCall(
      "vstorea_half", data->getType(), resultType, op->IdResultType(),
      {data, offset, p}, {op->data(), op->offset(), op->p()}, op->mode());

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Shuffle>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Shuffle>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *shuffleMask = module.getValue(op->shuffleMask());
  SPIRV_LL_ASSERT_PTR(shuffleMask);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "shuffle", resultType, op->IdResultType(), {x, shuffleMask},
      {op->x(), op->shuffleMask()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Shuffle2>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Shuffle2>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Value *shuffleMask = module.getValue(op->shuffleMask());
  SPIRV_LL_ASSERT_PTR(shuffleMask);

  llvm::Value *result = builder.createMangledBuiltinCall(
      "shuffle2", resultType, op->IdResultType(), {x, y, shuffleMask},
      {op->x(), op->y(), op->shuffleMask()});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Printf>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Printf>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *format = module.getValue(op->format());
  SPIRV_LL_ASSERT_PTR(format);

  llvm::Function *printf = module.llvmModule->getFunction("printf");
  if (!printf) {
    llvm::FunctionType *printfTy = llvm::FunctionType::get(
        resultType, {format->getType()}, /* isVarArg */ true);
    SPIRV_LL_ASSERT_PTR(printfTy);

    printf = llvm::Function::Create(
        printfTy, llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage,
        "printf", module.llvmModule.get());
    SPIRV_LL_ASSERT_PTR(printf);
    printf->setCallingConv(llvm::CallingConv::SPIR_FUNC);
    printf->addParamAttr(0, llvm::Attribute::NoCapture);
    printf->addParamAttr(0, llvm::Attribute::ReadOnly);
    printf->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Local);
  }

  llvm::SmallVector<llvm::Value *, 8> args;
  args.push_back(format);
  for (auto argId : op->AdditionalArguments()) {
    args.push_back(module.getValue(argId));
  }

  llvm::CallInst *call = builder.getIRBuilder().CreateCall(printf, args);
  SPIRV_LL_ASSERT_PTR(call);
  call->setName(module.getName(op->IdResult()));
  call->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  call->setTailCallKind(llvm::CallInst::TCK_Tail);

  module.addID(op->IdResult(), op, call);
  return cargo::nullopt;
}

template <>
cargo::optional<spirv_ll::Error>
spirv_ll::OpenCLBuilder::create<OpenCLLIB::Prefetch>(OpExtInst const &opc) {
  auto *op = module.create<OpenCLstd::Prefetch>(opc);

  llvm::Type *resultType = module.getType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *ptr = module.getValue(op->ptr());
  SPIRV_LL_ASSERT_PTR(ptr);

  llvm::Value *numElements = module.getValue(op->numElements());
  SPIRV_LL_ASSERT_PTR(numElements);

  auto result = builder.createMangledBuiltinCall(
      "prefetch", resultType, op->IdResultType(), {ptr, numElements},
      {op->ptr(), op->numElements()}, {CONST, NONE});

  module.addID(op->IdResult(), op, result);
  return cargo::nullopt;
}

#define CASE(ExtInst) \
  case ExtInst:       \
    return create<ExtInst>(opc);

cargo::optional<spirv_ll::Error> spirv_ll::OpenCLBuilder::create(
    OpExtInst const &opc) {
  switch (opc.Instruction()) {
    CASE(OpenCLLIB::Entrypoints::Acos)
    CASE(OpenCLLIB::Entrypoints::Acosh)
    CASE(OpenCLLIB::Entrypoints::Acospi)
    CASE(OpenCLLIB::Entrypoints::Asin)
    CASE(OpenCLLIB::Entrypoints::Asinh)
    CASE(OpenCLLIB::Entrypoints::Asinpi)
    CASE(OpenCLLIB::Entrypoints::Atan)
    CASE(OpenCLLIB::Entrypoints::Atan2)
    CASE(OpenCLLIB::Entrypoints::Atanh)
    CASE(OpenCLLIB::Entrypoints::Atanpi)
    CASE(OpenCLLIB::Entrypoints::Atan2pi)
    CASE(OpenCLLIB::Entrypoints::Cbrt)
    CASE(OpenCLLIB::Entrypoints::Ceil)
    CASE(OpenCLLIB::Entrypoints::Copysign)
    CASE(OpenCLLIB::Entrypoints::Cos)
    CASE(OpenCLLIB::Entrypoints::Cosh)
    CASE(OpenCLLIB::Entrypoints::Cospi)
    CASE(OpenCLLIB::Entrypoints::Erfc)
    CASE(OpenCLLIB::Entrypoints::Erf)
    CASE(OpenCLLIB::Entrypoints::Exp)
    CASE(OpenCLLIB::Entrypoints::Exp2)
    CASE(OpenCLLIB::Entrypoints::Exp10)
    CASE(OpenCLLIB::Entrypoints::Expm1)
    CASE(OpenCLLIB::Entrypoints::Fabs)
    CASE(OpenCLLIB::Entrypoints::Fdim)
    CASE(OpenCLLIB::Entrypoints::Floor)
    CASE(OpenCLLIB::Entrypoints::Fma)
    CASE(OpenCLLIB::Entrypoints::Fmax)
    CASE(OpenCLLIB::Entrypoints::Fmin)
    CASE(OpenCLLIB::Entrypoints::Fmod)
    CASE(OpenCLLIB::Entrypoints::Fract)
    CASE(OpenCLLIB::Entrypoints::Frexp)
    CASE(OpenCLLIB::Entrypoints::Hypot)
    CASE(OpenCLLIB::Entrypoints::Ilogb)
    CASE(OpenCLLIB::Entrypoints::Ldexp)
    CASE(OpenCLLIB::Entrypoints::Lgamma)
    CASE(OpenCLLIB::Entrypoints::Lgamma_r)
    CASE(OpenCLLIB::Entrypoints::Log)
    CASE(OpenCLLIB::Entrypoints::Log2)
    CASE(OpenCLLIB::Entrypoints::Log10)
    CASE(OpenCLLIB::Entrypoints::Log1p)
    CASE(OpenCLLIB::Entrypoints::Logb)
    CASE(OpenCLLIB::Entrypoints::Mad)
    CASE(OpenCLLIB::Entrypoints::Maxmag)
    CASE(OpenCLLIB::Entrypoints::Minmag)
    CASE(OpenCLLIB::Entrypoints::Modf)
    CASE(OpenCLLIB::Entrypoints::Nan)
    CASE(OpenCLLIB::Entrypoints::Nextafter)
    CASE(OpenCLLIB::Entrypoints::Pow)
    CASE(OpenCLLIB::Entrypoints::Pown)
    CASE(OpenCLLIB::Entrypoints::Powr)
    CASE(OpenCLLIB::Entrypoints::Remainder)
    CASE(OpenCLLIB::Entrypoints::Remquo)
    CASE(OpenCLLIB::Entrypoints::Rint)
    CASE(OpenCLLIB::Entrypoints::Rootn)
    CASE(OpenCLLIB::Entrypoints::Round)
    CASE(OpenCLLIB::Entrypoints::Rsqrt)
    CASE(OpenCLLIB::Entrypoints::Sin)
    CASE(OpenCLLIB::Entrypoints::Sincos)
    CASE(OpenCLLIB::Entrypoints::Sinh)
    CASE(OpenCLLIB::Entrypoints::Sinpi)
    CASE(OpenCLLIB::Entrypoints::Sqrt)
    CASE(OpenCLLIB::Entrypoints::Tan)
    CASE(OpenCLLIB::Entrypoints::Tanh)
    CASE(OpenCLLIB::Entrypoints::Tanpi)
    CASE(OpenCLLIB::Entrypoints::Tgamma)
    CASE(OpenCLLIB::Entrypoints::Trunc)
    CASE(OpenCLLIB::Entrypoints::Half_cos)
    CASE(OpenCLLIB::Entrypoints::Half_divide)
    CASE(OpenCLLIB::Entrypoints::Half_exp)
    CASE(OpenCLLIB::Entrypoints::Half_exp2)
    CASE(OpenCLLIB::Entrypoints::Half_exp10)
    CASE(OpenCLLIB::Entrypoints::Half_log)
    CASE(OpenCLLIB::Entrypoints::Half_log2)
    CASE(OpenCLLIB::Entrypoints::Half_log10)
    CASE(OpenCLLIB::Entrypoints::Half_powr)
    CASE(OpenCLLIB::Entrypoints::Half_recip)
    CASE(OpenCLLIB::Entrypoints::Half_rsqrt)
    CASE(OpenCLLIB::Entrypoints::Half_sin)
    CASE(OpenCLLIB::Entrypoints::Half_sqrt)
    CASE(OpenCLLIB::Entrypoints::Half_tan)
    CASE(OpenCLLIB::Entrypoints::Native_cos)
    CASE(OpenCLLIB::Entrypoints::Native_divide)
    CASE(OpenCLLIB::Entrypoints::Native_exp)
    CASE(OpenCLLIB::Entrypoints::Native_exp2)
    CASE(OpenCLLIB::Entrypoints::Native_exp10)
    CASE(OpenCLLIB::Entrypoints::Native_log)
    CASE(OpenCLLIB::Entrypoints::Native_log2)
    CASE(OpenCLLIB::Entrypoints::Native_log10)
    CASE(OpenCLLIB::Entrypoints::Native_powr)
    CASE(OpenCLLIB::Entrypoints::Native_recip)
    CASE(OpenCLLIB::Entrypoints::Native_rsqrt)
    CASE(OpenCLLIB::Entrypoints::Native_sin)
    CASE(OpenCLLIB::Entrypoints::Native_sqrt)
    CASE(OpenCLLIB::Entrypoints::Native_tan)
    CASE(OpenCLLIB::Entrypoints::SAbs)
    CASE(OpenCLLIB::Entrypoints::SAbs_diff)
    CASE(OpenCLLIB::Entrypoints::SAdd_sat)
    CASE(OpenCLLIB::Entrypoints::UAdd_sat)
    CASE(OpenCLLIB::Entrypoints::SHadd)
    CASE(OpenCLLIB::Entrypoints::UHadd)
    CASE(OpenCLLIB::Entrypoints::SRhadd)
    CASE(OpenCLLIB::Entrypoints::URhadd)
    CASE(OpenCLLIB::Entrypoints::SClamp)
    CASE(OpenCLLIB::Entrypoints::UClamp)
    CASE(OpenCLLIB::Entrypoints::Clz)
    CASE(OpenCLLIB::Entrypoints::Ctz)
    CASE(OpenCLLIB::Entrypoints::SMad_hi)
    CASE(OpenCLLIB::Entrypoints::UMad_sat)
    CASE(OpenCLLIB::Entrypoints::SMad_sat)
    CASE(OpenCLLIB::Entrypoints::SMax)
    CASE(OpenCLLIB::Entrypoints::UMax)
    CASE(OpenCLLIB::Entrypoints::SMin)
    CASE(OpenCLLIB::Entrypoints::UMin)
    CASE(OpenCLLIB::Entrypoints::SMul_hi)
    CASE(OpenCLLIB::Entrypoints::Rotate)
    CASE(OpenCLLIB::Entrypoints::SSub_sat)
    CASE(OpenCLLIB::Entrypoints::USub_sat)
    CASE(OpenCLLIB::Entrypoints::U_Upsample)
    CASE(OpenCLLIB::Entrypoints::S_Upsample)
    CASE(OpenCLLIB::Entrypoints::Popcount)
    CASE(OpenCLLIB::Entrypoints::SMad24)
    CASE(OpenCLLIB::Entrypoints::UMad24)
    CASE(OpenCLLIB::Entrypoints::SMul24)
    CASE(OpenCLLIB::Entrypoints::UMul24)
    CASE(OpenCLLIB::Entrypoints::UAbs)
    CASE(OpenCLLIB::Entrypoints::UAbs_diff)
    CASE(OpenCLLIB::Entrypoints::UMul_hi)
    CASE(OpenCLLIB::Entrypoints::UMad_hi)
    CASE(OpenCLLIB::Entrypoints::FClamp)
    CASE(OpenCLLIB::Entrypoints::Degrees)
    CASE(OpenCLLIB::Entrypoints::FMax_common)
    CASE(OpenCLLIB::Entrypoints::FMin_common)
    CASE(OpenCLLIB::Entrypoints::Mix)
    CASE(OpenCLLIB::Entrypoints::Radians)
    CASE(OpenCLLIB::Entrypoints::Step)
    CASE(OpenCLLIB::Entrypoints::Smoothstep)
    CASE(OpenCLLIB::Entrypoints::Sign)
    CASE(OpenCLLIB::Entrypoints::Cross)
    CASE(OpenCLLIB::Entrypoints::Distance)
    CASE(OpenCLLIB::Entrypoints::Length)
    CASE(OpenCLLIB::Entrypoints::Normalize)
    CASE(OpenCLLIB::Entrypoints::Fast_distance)
    CASE(OpenCLLIB::Entrypoints::Fast_length)
    CASE(OpenCLLIB::Entrypoints::Fast_normalize)
    CASE(OpenCLLIB::Entrypoints::Bitselect)
    CASE(OpenCLLIB::Entrypoints::Select)
    CASE(OpenCLLIB::Entrypoints::Vloadn)
    CASE(OpenCLLIB::Entrypoints::Vstoren)
    CASE(OpenCLLIB::Entrypoints::Vload_half)
    CASE(OpenCLLIB::Entrypoints::Vload_halfn)
    CASE(OpenCLLIB::Entrypoints::Vstore_half)
    CASE(OpenCLLIB::Entrypoints::Vstore_half_r)
    CASE(OpenCLLIB::Entrypoints::Vstore_halfn)
    CASE(OpenCLLIB::Entrypoints::Vstore_halfn_r)
    CASE(OpenCLLIB::Entrypoints::Vloada_halfn)
    CASE(OpenCLLIB::Entrypoints::Vstorea_halfn)
    CASE(OpenCLLIB::Entrypoints::Vstorea_halfn_r)
    CASE(OpenCLLIB::Entrypoints::Shuffle)
    CASE(OpenCLLIB::Entrypoints::Shuffle2)
    CASE(OpenCLLIB::Entrypoints::Printf)
    CASE(OpenCLLIB::Entrypoints::Prefetch)
    default:
      return Error("Unrecognized extended instruction " +
                   std::to_string(opc.Instruction()));
  }
}

#undef CASE

}  // namespace spirv_ll
