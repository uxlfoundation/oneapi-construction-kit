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

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

// These tests catch a bug identified in CA-3032, where small SIMD width made
// AArch64 backend fail.

TEST_P(Execution, Task_15_01_convert) {
  const size_t local_range = 2;

  kts::Reference1D<cl_long> refIn = [](size_t x) -> cl_long {
    return (cl_long)kts::Ref_B(x);
  };

  kts::Reference1D<cl_float> refOut = [](size_t x) -> cl_float {
    return (cl_float)kts::Ref_B(x);
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N, local_range);
}

TEST_P(Execution, Task_15_02_convert2) {
  const size_t local_range = 2;

  kts::Reference1D<cl_long2> refIn = [](size_t x) -> cl_long2 {
    cl_long2 input;
    for (size_t i = 0; i < 2; ++i) {
      input.s[i] = static_cast<cl_long>(kts::Ref_B(x)) * 2 - i;
    }
    return input;
  };

  kts::Reference1D<cl_float2> refOut = [&refIn](size_t x) -> cl_float2 {
    cl_float2 ref;
    for (size_t i = 0; i < 2; ++i) {
      ref.s[i] = static_cast<cl_float>(refIn(x).s[i]);
    }
    return ref;
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N, local_range);
}

TEST_P(Execution, Task_15_03_convert3) {
  const size_t local_range = 2;

  kts::Reference1D<cl_long> refIn = [](size_t x) -> cl_long {
    return (cl_long)kts::Ref_B(x);
  };

  kts::Reference1D<cl_float> refOut = [](size_t x) -> cl_float {
    return (cl_float)kts::Ref_B(x);
  };

  AddInputBuffer(3 * kts::N, refIn);
  AddOutputBuffer(3 * kts::N, refOut);

  RunGeneric1D(kts::N, local_range);
}

TEST_P(Execution, Task_15_04_convert4) {
  const size_t local_range = 2;

  kts::Reference1D<cl_long4> refIn = [](size_t x) -> cl_long4 {
    cl_long4 input;
    for (size_t i = 0; i < 4; ++i) {
      input.s[i] = static_cast<cl_long>(kts::Ref_B(x)) * 4 - i;
    }
    return input;
  };

  kts::Reference1D<cl_float4> refOut = [&refIn](size_t x) -> cl_float4 {
    cl_float4 ref;
    for (size_t i = 0; i < 4; ++i) {
      ref.s[i] = static_cast<cl_float>(refIn(x).s[i]);
    }
    return ref;
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N, local_range);
}

TEST_P(Execution, Task_15_05_convert3) {
  const size_t local_range = 2;

  kts::Reference1D<ucl::Long3> refIn = [](size_t x) -> ucl::Long3 {
    ucl::Long3 input;
    for (size_t i = 0; i < 3; ++i) {
      input[i] = static_cast<cl_long>(kts::Ref_B(x)) * 3 - i;
    }
    return input;
  };

  kts::Reference1D<ucl::Float3> refOut = [&refIn](size_t x) -> ucl::Float3 {
    ucl::Float3 ref;
    for (size_t i = 0; i < 3; ++i) {
      ref[i] = static_cast<cl_float>(refIn(x)[i]);
    }
    return ref;
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N, local_range);
}
