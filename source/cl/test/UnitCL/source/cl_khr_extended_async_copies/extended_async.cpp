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

#include <gtest/gtest.h>

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

namespace {
const size_t local_wg_size = 16;

// Vector addition: C[x] = A[x] + B[x];
kts::Reference1D<cl_int> vaddInA = [](size_t x) {
  return (kts::Ref_Identity(x) * 3) + 27;
};

kts::Reference1D<cl_int> vaddInB = [](size_t x) {
  return (kts::Ref_Identity(x) * 7) + 41;
};

kts::Reference1D<cl_int> vaddOutC = [](size_t x) {
  return vaddInA(x) + vaddInB(x);
};
}  // namespace

TEST_P(Execution, Ext_Async_01_Simple_2D) {
  // The extension isn't supported in SPIRV yet, only test CL C
  if (!isSourceTypeIn({OPENCL_C, OFFLINE})) {
    GTEST_SKIP();
  }
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Ext_Async_02_Simple_3D) {
  // The extension isn't supported in SPIRV yet, only test CL C
  if (!isSourceTypeIn({OPENCL_C, OFFLINE})) {
    GTEST_SKIP();
  }
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}
