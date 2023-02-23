// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>

namespace {
template <typename T>
T half_exp(T x) {
  return __abacus_half_exp2(x * ABACUS_LOG2E_F);
}
}  // namespace

abacus_float ABACUS_API __abacus_half_exp(abacus_float x) {
  return half_exp<>(x);
}
abacus_float2 ABACUS_API __abacus_half_exp(abacus_float2 x) {
  return half_exp<>(x);
}
abacus_float3 ABACUS_API __abacus_half_exp(abacus_float3 x) {
  return half_exp<>(x);
}
abacus_float4 ABACUS_API __abacus_half_exp(abacus_float4 x) {
  return half_exp<>(x);
}
abacus_float8 ABACUS_API __abacus_half_exp(abacus_float8 x) {
  return half_exp<>(x);
}
abacus_float16 ABACUS_API __abacus_half_exp(abacus_float16 x) {
  return half_exp<>(x);
}
