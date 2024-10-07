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

// Third-party headers
#include <gtest/gtest.h>

// In-house headers
#include "kts/execution.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Builtins_01_Fmin_Vector_Scalar_NaN) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  AddInputBuffer(kts::N, refIn);
  AddPrimitive(0);
  AddOutputBuffer(kts::N, refIn);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_02_Fmax_Vector_Scalar_NaN) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  AddInputBuffer(kts::N, refIn);
  AddPrimitive(0);
  AddOutputBuffer(kts::N, refIn);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_03_Mad_Conversions) {
  kts::Reference1D<cl_float> refOut = [](size_t) {
    return static_cast<cl_float>((10.0f * 1.2f) + 20.0f);
  };
  AddPrimitive(static_cast<cl_char>(10));
  AddPrimitive(20);
  AddPrimitive(1.2f);
  AddOutputBuffer(1, refOut);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_04_Mad24_Conversions) {
  kts::Reference1D<cl_float> refOut = [](size_t) {
    return static_cast<cl_float>((10 * 1) + 20);
  };
  AddPrimitive(static_cast<cl_char>(10));
  AddPrimitive(20);
  AddPrimitive(1.2f);
  AddOutputBuffer(1, refOut);

  RunGeneric1D(kts::N);
}
