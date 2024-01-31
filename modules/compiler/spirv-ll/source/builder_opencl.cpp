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

#include <cargo/optional.h>
#include <llvm/IR/Attributes.h>
#include <spirv-ll/builder_opencl.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

namespace spirv_ll {

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
  Printf(const OpCode &other) : OpExtInst(other) {}
  spv::Id format() const { return getOpExtInstOperand(0); }
  llvm::SmallVector<spv::Id, 8> AdditionalArguments() const {
    llvm::SmallVector<spv::Id, 8> additionalArguments;
    for (uint16_t i = 1; i < opExtInstOperandCount(); i++) {
      additionalArguments.push_back(getOpExtInstOperand(i));
    }
    return additionalArguments;
  }
};

}  // namespace OpenCLstd

static llvm::Error createPrintf(const OpExtInst &opc, Module &module,
                                Builder &builder) {
  auto *op = module.create<OpenCLstd::Printf>(opc);

  llvm::Type *resultType = module.getLLVMType(op->IdResultType());
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
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X>>(const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, Y>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, Y>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->y()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, Y, Z>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, Y, Z>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->y(), op->z()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, PTR>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, PTR>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->ptr()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, EXP>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, EXP>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->exp()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, K>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, K>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->k()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, SIGNP>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, SIGNP>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->signp()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<Y, X>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<Y, X>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->y(), op->x()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<A, B, C>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<A, B, C>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->a(), op->b(), op->c()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, IPTR>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, IPTR>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->iPtr()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<NANCODE>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<NANCODE>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->nanCode()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, Y, QUO>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, Y, QUO>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->y(), op->quo()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, COSVAL>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, COSVAL>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->cosVal()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, MINVAL, MAXVAL>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, MINVAL, MAXVAL>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->minVal(), op->maxVal()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<V, I>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<V, I>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->v(), op->i()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<HI, LO>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<HI, LO>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->hi(), op->lo()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<DEGREES>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<DEGREES>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->degrees()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<RADIANS>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<RADIANS>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->radians()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, Y, A>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, Y, A>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->y(), op->a()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<P>>(const OpExtInst &opc) {
  auto *op = module.create<ExtInst<P>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<P0, P1>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<P0, P1>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->p0(), op->p1()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<EDGE, X>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<EDGE, X>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->edge(), op->x()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<EDGE0, EDGE1, X>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<EDGE0, EDGE1, X>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->edge0(), op->edge1(), op->x()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, SHUFFLEMASK>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, SHUFFLEMASK>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->shuffleMask()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<X, Y, SHUFFLEMASK>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<X, Y, SHUFFLEMASK>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->x(), op->y(), op->shuffleMask()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<PTR, NUM_ELEMENTS>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<PTR, NUM_ELEMENTS>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->ptr(), op->numElements()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<OFFSET, P>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<OFFSET, P, N>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->offset(), op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<OFFSET, P, N>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<OFFSET, P, N>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->offset(), op->p(), op->n()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<DATA, OFFSET, P>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<DATA, OFFSET, P>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(), {op->data(), op->offset(), op->p()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error spirv_ll::OpenCLBuilder::create<ExtInst<DATA, OFFSET, P, MODE>>(
    const OpExtInst &opc) {
  auto *op = module.create<ExtInst<DATA, OFFSET, P, MODE>>(opc);

  auto *const result = builder.createOCLBuiltinCall(
      static_cast<OpenCLLIB::Entrypoints>(opc.Instruction()),
      op->IdResultType(),
      {op->data(), op->offset(), op->p(), static_cast<spv::Id>(op->mode())});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

#define CASE(Opcode, ExtInst) \
  case Opcode:                \
    return create<ExtInst>(opc);

llvm::Error spirv_ll::OpenCLBuilder::create(const OpExtInst &opc) {
  switch (opc.Instruction()) {
    CASE(OpenCLLIB::Acos, OpenCLstd::Acos)
    CASE(OpenCLLIB::Acosh, OpenCLstd::Acosh)
    CASE(OpenCLLIB::Acospi, OpenCLstd::Acospi)
    CASE(OpenCLLIB::Asin, OpenCLstd::Asin)
    CASE(OpenCLLIB::Asinh, OpenCLstd::Asinh)
    CASE(OpenCLLIB::Asinpi, OpenCLstd::Asinpi)
    CASE(OpenCLLIB::Atan, OpenCLstd::Atan)
    CASE(OpenCLLIB::Atan2, OpenCLstd::Atan2)
    CASE(OpenCLLIB::Atanh, OpenCLstd::Atanh)
    CASE(OpenCLLIB::Atanpi, OpenCLstd::Atanpi)
    CASE(OpenCLLIB::Atan2pi, OpenCLstd::Atan2pi)
    CASE(OpenCLLIB::Cbrt, OpenCLstd::Cbrt)
    CASE(OpenCLLIB::Ceil, OpenCLstd::Ceil)
    CASE(OpenCLLIB::Copysign, OpenCLstd::Copysign)
    CASE(OpenCLLIB::Cos, OpenCLstd::Cos)
    CASE(OpenCLLIB::Cosh, OpenCLstd::Cosh)
    CASE(OpenCLLIB::Cospi, OpenCLstd::Cospi)
    CASE(OpenCLLIB::Erfc, OpenCLstd::Erfc)
    CASE(OpenCLLIB::Erf, OpenCLstd::Erf)
    CASE(OpenCLLIB::Exp, OpenCLstd::Exp)
    CASE(OpenCLLIB::Exp2, OpenCLstd::Exp2)
    CASE(OpenCLLIB::Exp10, OpenCLstd::Exp10)
    CASE(OpenCLLIB::Expm1, OpenCLstd::Expm1)
    CASE(OpenCLLIB::Fabs, OpenCLstd::Fabs)
    CASE(OpenCLLIB::Fdim, OpenCLstd::Fdim)
    CASE(OpenCLLIB::Floor, OpenCLstd::Floor)
    CASE(OpenCLLIB::Fma, OpenCLstd::Fma)
    CASE(OpenCLLIB::Fmax, OpenCLstd::Fmax)
    CASE(OpenCLLIB::Fmin, OpenCLstd::Fmin)
    CASE(OpenCLLIB::Fmod, OpenCLstd::Fmod)
    CASE(OpenCLLIB::Fract, OpenCLstd::Fract)
    CASE(OpenCLLIB::Frexp, OpenCLstd::Frexp)
    CASE(OpenCLLIB::Hypot, OpenCLstd::Hypot)
    CASE(OpenCLLIB::Ilogb, OpenCLstd::Ilogb)
    CASE(OpenCLLIB::Ldexp, OpenCLstd::Ldexp)
    CASE(OpenCLLIB::Lgamma, OpenCLstd::Lgamma)
    CASE(OpenCLLIB::Lgamma_r, OpenCLstd::Lgamma_r)
    CASE(OpenCLLIB::Log, OpenCLstd::Log)
    CASE(OpenCLLIB::Log2, OpenCLstd::Log2)
    CASE(OpenCLLIB::Log10, OpenCLstd::Log10)
    CASE(OpenCLLIB::Log1p, OpenCLstd::Log1p)
    CASE(OpenCLLIB::Logb, OpenCLstd::Logb)
    CASE(OpenCLLIB::Mad, OpenCLstd::Mad)
    CASE(OpenCLLIB::Maxmag, OpenCLstd::Maxmag)
    CASE(OpenCLLIB::Minmag, OpenCLstd::Minmag)
    CASE(OpenCLLIB::Modf, OpenCLstd::Modf)
    CASE(OpenCLLIB::Nan, OpenCLstd::Nan)
    CASE(OpenCLLIB::Nextafter, OpenCLstd::Nextafter)
    CASE(OpenCLLIB::Pow, OpenCLstd::Pow)
    CASE(OpenCLLIB::Pown, OpenCLstd::Pown)
    CASE(OpenCLLIB::Powr, OpenCLstd::Powr)
    CASE(OpenCLLIB::Remainder, OpenCLstd::Remainder)
    CASE(OpenCLLIB::Remquo, OpenCLstd::Remquo)
    CASE(OpenCLLIB::Rint, OpenCLstd::Rint)
    CASE(OpenCLLIB::Rootn, OpenCLstd::Rootn)
    CASE(OpenCLLIB::Round, OpenCLstd::Round)
    CASE(OpenCLLIB::Rsqrt, OpenCLstd::Rsqrt)
    CASE(OpenCLLIB::Sin, OpenCLstd::Sin)
    CASE(OpenCLLIB::Sincos, OpenCLstd::Sincos)
    CASE(OpenCLLIB::Sinh, OpenCLstd::Sinh)
    CASE(OpenCLLIB::Sinpi, OpenCLstd::Sinpi)
    CASE(OpenCLLIB::Sqrt, OpenCLstd::Sqrt)
    CASE(OpenCLLIB::Tan, OpenCLstd::Tan)
    CASE(OpenCLLIB::Tanh, OpenCLstd::Tanh)
    CASE(OpenCLLIB::Tanpi, OpenCLstd::Tanpi)
    CASE(OpenCLLIB::Tgamma, OpenCLstd::Tgamma)
    CASE(OpenCLLIB::Trunc, OpenCLstd::Trunc)
    CASE(OpenCLLIB::Half_cos, OpenCLstd::Half_cos)
    CASE(OpenCLLIB::Half_divide, OpenCLstd::Half_divide)
    CASE(OpenCLLIB::Half_exp, OpenCLstd::Half_exp)
    CASE(OpenCLLIB::Half_exp2, OpenCLstd::Half_exp2)
    CASE(OpenCLLIB::Half_exp10, OpenCLstd::Half_exp10)
    CASE(OpenCLLIB::Half_log, OpenCLstd::Half_log)
    CASE(OpenCLLIB::Half_log2, OpenCLstd::Half_log2)
    CASE(OpenCLLIB::Half_log10, OpenCLstd::Half_log10)
    CASE(OpenCLLIB::Half_powr, OpenCLstd::Half_powr)
    CASE(OpenCLLIB::Half_recip, OpenCLstd::Half_recip)
    CASE(OpenCLLIB::Half_rsqrt, OpenCLstd::Half_rsqrt)
    CASE(OpenCLLIB::Half_sin, OpenCLstd::Half_sin)
    CASE(OpenCLLIB::Half_sqrt, OpenCLstd::Half_sqrt)
    CASE(OpenCLLIB::Half_tan, OpenCLstd::Half_tan)
    CASE(OpenCLLIB::Native_cos, OpenCLstd::Native_cos)
    CASE(OpenCLLIB::Native_divide, OpenCLstd::Native_divide)
    CASE(OpenCLLIB::Native_exp, OpenCLstd::Native_exp)
    CASE(OpenCLLIB::Native_exp2, OpenCLstd::Native_exp2)
    CASE(OpenCLLIB::Native_exp10, OpenCLstd::Native_exp10)
    CASE(OpenCLLIB::Native_log, OpenCLstd::Native_log)
    CASE(OpenCLLIB::Native_log2, OpenCLstd::Native_log2)
    CASE(OpenCLLIB::Native_log10, OpenCLstd::Native_log10)
    CASE(OpenCLLIB::Native_powr, OpenCLstd::Native_powr)
    CASE(OpenCLLIB::Native_recip, OpenCLstd::Native_recip)
    CASE(OpenCLLIB::Native_rsqrt, OpenCLstd::Native_rsqrt)
    CASE(OpenCLLIB::Native_sin, OpenCLstd::Native_sin)
    CASE(OpenCLLIB::Native_sqrt, OpenCLstd::Native_sqrt)
    CASE(OpenCLLIB::Native_tan, OpenCLstd::Native_tan)
    CASE(OpenCLLIB::SAbs, OpenCLstd::S_abs)
    CASE(OpenCLLIB::SAbs_diff, OpenCLstd::S_abs_diff)
    CASE(OpenCLLIB::SAdd_sat, OpenCLstd::S_add_sat)
    CASE(OpenCLLIB::UAdd_sat, OpenCLstd::U_add_sat)
    CASE(OpenCLLIB::SHadd, OpenCLstd::S_hadd)
    CASE(OpenCLLIB::UHadd, OpenCLstd::U_hadd)
    CASE(OpenCLLIB::SRhadd, OpenCLstd::S_rhadd)
    CASE(OpenCLLIB::URhadd, OpenCLstd::U_rhadd)
    CASE(OpenCLLIB::SClamp, OpenCLstd::S_clamp)
    CASE(OpenCLLIB::UClamp, OpenCLstd::U_clamp)
    CASE(OpenCLLIB::Clz, OpenCLstd::Clz)
    CASE(OpenCLLIB::Ctz, OpenCLstd::Ctz)
    CASE(OpenCLLIB::SMad_hi, OpenCLstd::S_mad_hi)
    CASE(OpenCLLIB::UMad_sat, OpenCLstd::U_mad_sat)
    CASE(OpenCLLIB::SMad_sat, OpenCLstd::S_mad_sat)
    CASE(OpenCLLIB::SMax, OpenCLstd::S_max)
    CASE(OpenCLLIB::UMax, OpenCLstd::U_max)
    CASE(OpenCLLIB::SMin, OpenCLstd::S_min)
    CASE(OpenCLLIB::UMin, OpenCLstd::U_min)
    CASE(OpenCLLIB::SMul_hi, OpenCLstd::S_mul_hi)
    CASE(OpenCLLIB::Rotate, OpenCLstd::Rotate)
    CASE(OpenCLLIB::SSub_sat, OpenCLstd::S_sub_sat)
    CASE(OpenCLLIB::USub_sat, OpenCLstd::U_sub_sat)
    CASE(OpenCLLIB::U_Upsample, OpenCLstd::U_upsample)
    CASE(OpenCLLIB::S_Upsample, OpenCLstd::S_upsample)
    CASE(OpenCLLIB::Popcount, OpenCLstd::Popcount)
    CASE(OpenCLLIB::SMad24, OpenCLstd::S_mad24)
    CASE(OpenCLLIB::UMad24, OpenCLstd::U_mad24)
    CASE(OpenCLLIB::SMul24, OpenCLstd::S_mul24)
    CASE(OpenCLLIB::UMul24, OpenCLstd::U_mul24)
    CASE(OpenCLLIB::UAbs, OpenCLstd::U_abs)
    CASE(OpenCLLIB::UAbs_diff, OpenCLstd::U_abs_diff)
    CASE(OpenCLLIB::UMul_hi, OpenCLstd::U_mul_hi)
    CASE(OpenCLLIB::UMad_hi, OpenCLstd::U_mad_hi)
    CASE(OpenCLLIB::FClamp, OpenCLstd::Fclamp)
    CASE(OpenCLLIB::Degrees, OpenCLstd::Degrees)
    CASE(OpenCLLIB::FMax_common, OpenCLstd::Fmax_common)
    CASE(OpenCLLIB::FMin_common, OpenCLstd::Fmin_common)
    CASE(OpenCLLIB::Mix, OpenCLstd::Mix)
    CASE(OpenCLLIB::Radians, OpenCLstd::Radians)
    CASE(OpenCLLIB::Step, OpenCLstd::Step)
    CASE(OpenCLLIB::Smoothstep, OpenCLstd::Smoothstep)
    CASE(OpenCLLIB::Sign, OpenCLstd::Sign)
    CASE(OpenCLLIB::Cross, OpenCLstd::Cross)
    CASE(OpenCLLIB::Distance, OpenCLstd::Distance)
    CASE(OpenCLLIB::Length, OpenCLstd::Length)
    CASE(OpenCLLIB::Normalize, OpenCLstd::Normalize)
    CASE(OpenCLLIB::Fast_distance, OpenCLstd::Fast_distance)
    CASE(OpenCLLIB::Fast_length, OpenCLstd::Fast_length)
    CASE(OpenCLLIB::Fast_normalize, OpenCLstd::Fast_normalize)
    CASE(OpenCLLIB::Bitselect, OpenCLstd::Bitselect)
    CASE(OpenCLLIB::Select, OpenCLstd::Select)
    CASE(OpenCLLIB::Shuffle, OpenCLstd::Shuffle)
    CASE(OpenCLLIB::Shuffle2, OpenCLstd::Shuffle2)
    CASE(OpenCLLIB::Prefetch, OpenCLstd::Prefetch)
    CASE(OpenCLLIB::Vloadn, OpenCLstd::Vloadn)
    CASE(OpenCLLIB::Vload_half, OpenCLstd::Vload_half)
    CASE(OpenCLLIB::Vload_halfn, OpenCLstd::Vload_halfn)
    CASE(OpenCLLIB::Vloada_halfn, OpenCLstd::Vloada_halfn)
    CASE(OpenCLLIB::Vstoren, OpenCLstd::Vstoren)
    CASE(OpenCLLIB::Vstore_half, OpenCLstd::Vstore_half)
    CASE(OpenCLLIB::Vstore_halfn, OpenCLstd::Vstore_halfn)
    CASE(OpenCLLIB::Vstorea_halfn, OpenCLstd::Vstorea_halfn)
    CASE(OpenCLLIB::Vstore_half_r, OpenCLstd::Vstore_half_r)
    CASE(OpenCLLIB::Vstore_halfn_r, OpenCLstd::Vstore_halfn_r)
    CASE(OpenCLLIB::Vstorea_halfn_r, OpenCLstd::Vstorea_halfn_r)

    case OpenCLLIB::Entrypoints::Printf:
      return createPrintf(opc, module, builder);

    default:
      return makeStringError("Unrecognized extended instruction " +
                             std::to_string(opc.Instruction()));
  }
}

#undef CASE

}  // namespace spirv_ll
