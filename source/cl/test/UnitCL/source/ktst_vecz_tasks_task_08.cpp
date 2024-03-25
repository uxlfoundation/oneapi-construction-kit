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
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_08_01_User_Fn_Identity) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Task_08_02_User_Fn_SExt) {
  kts::Reference1D<cl_short> refIn = [](size_t x) {
    return static_cast<cl_short>(x * 2);
  };
  kts::Reference1D<cl_int> refOut = [&refIn](size_t x) { return -refIn(x); };
  AddOutputBuffer(kts::N, refOut);
  AddInputBuffer(kts::N, refIn);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_08_03_User_Fn_Two_Contexts) {
  const cl_int alpha = 17;
  auto foo = [](cl_int x, cl_int y) { return x * (y - 1); };
  kts::Reference1D<cl_int> refOut = [=, &foo](size_t x) {
    const cl_int src1 = kts::Ref_A(x);
    const cl_int src2 = kts::Ref_B(x);
    const cl_int res1 = foo(src1, src2);
    const cl_int res2 = foo(alpha, src2);
    return res1 + res2;
  };
  AddOutputBuffer(kts::N, refOut);
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddPrimitive(alpha);
  RunGeneric1D(kts::N);
}
