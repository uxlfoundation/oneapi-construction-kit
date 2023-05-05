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

#ifndef KTS_REFERENCE_FUNCTIONS_H_INCLUDED
#define KTS_REFERENCE_FUNCTIONS_H_INCLUDED

#include <cstddef>

namespace kts {

// Returns x.
int Ref_Identity(size_t x);

// Computes a series.
int Ref_A(size_t x);

// Computes another series.
int Ref_B(size_t x);

// Computes x + 1.
int Ref_PlusOne(size_t x);

// Computes x - 1.
int Ref_MinusOne(size_t x);

// Computes x * 3.
int Ref_Triple(size_t x);

// Computes -x.
int Ref_Opposite(size_t x);

// Returns non-zero if x is odd, zero otherwise.
int Ref_Odd(size_t x);

// Computes (A + B).
int Ref_Add(size_t x);

// Computes (PlusOne * MinusOne).
int Ref_Mul(size_t x);

// Computes Mul + Triple.
int Ref_FMA(size_t x);

// Computes Odd ? 1 : -1.
int Ref_Ternary(size_t x);

// Computes MSB(Odd) ? 1 : -1.
int Ref_Ternary_OpenCL(size_t x);

// Computes the number of leading zeros in x.
unsigned Ref_Clz(size_t x);

// Computes x - a.
float Ref_NegativeOffset(size_t x);

// Computes x * 2.
float Ref_Float(size_t x);

// Computes fabs(NegativeOffset).
float Ref_Abs(size_t x);

// Computes dot(NegativeOffset, Double).
float Ref_Dot(size_t x);

// Computes distance(NegativeOffset, Double).
float Ref_Distance(size_t x);

// Computes length(Double).
float Ref_Length(size_t x);
}

#endif  // KTS_REFERENCE_FUNCTIONS_H_INCLUDED
