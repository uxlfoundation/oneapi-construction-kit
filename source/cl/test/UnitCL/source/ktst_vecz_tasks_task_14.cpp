// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// This file contains tests the processing of FNeg instructions.

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

TEST_P(Execution, Task_14_01_balance) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};

  kts::Reference1D<cl_float4> refIn = [](size_t x) -> cl_float4 {
    cl_float4 input;
    for (size_t i = 0; i < 4; ++i) {
      input.s[i] = kts::Ref_Float(x) * 4 - i;
    }
    return input;
  };

  kts::Reference1D<cl_float4> refOut = [](size_t) -> cl_float4 {
    cl_float4 zero = {{0.0f, 0.0f, 0.0f, 0.0f}};
    return zero;
  };

  const size_t N = global_range[0];
  const cl_float k = 0.5f;
  AddPrimitive(k);
  AddInOutBuffer(N, refIn, refOut);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Task_14_02_negate) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};

  kts::Reference1D<cl_float4> refIn = [](size_t x) -> cl_float4 {
    cl_float4 input;
    for (size_t i = 0; i < 4; ++i) {
      input.s[i] = kts::Ref_Float(x) * 4 - i;
    }
    return input;
  };

  kts::Reference1D<cl_float4> refOut = [&refIn](size_t x) -> cl_float4 {
    cl_float4 neg = refIn(x);
    for (size_t i = 0; i < 4; ++i) {
      neg.s[i] = -neg.s[i];
    }
    return neg;
  };

  const size_t N = global_range[0];
  AddInputBuffer(N, refIn);
  AddOutputBuffer(N, refOut);

  RunGenericND(2, global_range, local_range);
}
