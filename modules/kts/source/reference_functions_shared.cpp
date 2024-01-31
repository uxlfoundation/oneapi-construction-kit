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

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "kts/reference_functions.h"

float kts::Ref_NegativeOffset(size_t x) { return x - 3.0f; }

float kts::Ref_Float(size_t x) { return x * 2.0f; }

float kts::Ref_Abs(size_t x) { return std::fabs(Ref_NegativeOffset(x)); }

float kts::Ref_Dot(size_t x) {
  const float a = Ref_NegativeOffset(x);
  const float b = Ref_Float(x);
  return a * b;
}

float kts::Ref_Distance(size_t x) {
  const float a = Ref_NegativeOffset(x);
  const float b = Ref_Float(x);
  return sqrtf((a - b) * (a - b));
}

float kts::Ref_Length(size_t x) {
  const float a = Ref_Float(x);
  return sqrtf(a * a);
}

int32_t kts::Ref_Identity(size_t x) { return static_cast<int32_t>(x); }

int32_t kts::Ref_A(size_t x) { return (kts::Ref_Identity(x) * 3) - 42; }

int32_t kts::Ref_B(size_t x) { return (kts::Ref_Identity(x) * 5) + 42; }

int32_t kts::Ref_PlusOne(size_t x) { return kts::Ref_Identity(x) + 1; }

int32_t kts::Ref_MinusOne(size_t x) { return kts::Ref_Identity(x) - 1; }

int32_t kts::Ref_Triple(size_t x) { return kts::Ref_Identity(x) * 3; }

int32_t kts::Ref_Opposite(size_t x) { return -kts::Ref_Identity(x); }

int32_t kts::Ref_Odd(size_t x) { return kts::Ref_Identity(x) & 1; }

int32_t kts::Ref_Add(size_t x) { return kts::Ref_A(x) + kts::Ref_B(x); }

int32_t kts::Ref_Mul(size_t x) {
  return kts::Ref_PlusOne(x) * kts::Ref_MinusOne(x);
}

int32_t kts::Ref_FMA(size_t x) { return kts::Ref_Mul(x) + kts::Ref_Triple(x); }

int32_t kts::Ref_Ternary(size_t x) { return kts::Ref_Odd(x) ? 1 : -1; }

int32_t kts::Ref_Ternary_OpenCL(size_t x) {
  return (kts::Ref_Odd(x) & 0x80000000) ? 1 : -1;
}

uint32_t kts::Ref_Clz(size_t x) {
  uint32_t i = 0;
  while (((uint32_t)x & (1 << (31 - i))) == 0) {
    i++;
    if (i == 32) break;
  }
  return i;
}
