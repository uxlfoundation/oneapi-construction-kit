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

TEST_P(Execution, Task_01_01_Copy) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_02_Add) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddOutputBuffer(kts::N, kts::Ref_Add);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_03_Mul_FMA) {
  AddInputBuffer(kts::N, kts::Ref_PlusOne);
  AddInputBuffer(kts::N, kts::Ref_MinusOne);
  AddInputBuffer(kts::N, kts::Ref_Triple);
  AddOutputBuffer(kts::N, kts::Ref_Mul);
  AddOutputBuffer(kts::N, kts::Ref_FMA);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_04_Ternary) {
  AddInputBuffer(kts::N, kts::Ref_Odd);
  AddPrimitive(1);
  AddPrimitive(-1);
  AddOutputBuffer(kts::N, kts::Ref_Ternary);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_05_Broadcast) {
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_06_Broadcast_Uniform) {
  cl_int foo = 41;
  kts::Reference1D<cl_int> refOut = [&foo](size_t) { return foo + 1; };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(foo);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_01_07_Mulhi) {
  kts::Reference1D<cl_int> refIn1 = [](size_t x) {
    return 0x7fff0000 + kts::Ref_A(x);
  };
  kts::Reference1D<cl_int> refIn2 = [](size_t x) {
    return 0x43210000 + kts::Ref_B(x);
  };
  kts::Reference1D<cl_int> refOut = [&refIn1, &refIn2](size_t x) {
    const cl_int a = refIn1(x);
    const cl_int b = refIn2(x);
    const cl_int c = kts::Ref_Odd(x);
    const int64_t temp = (int64_t)a * (int64_t)b;
    return (cl_int)(temp >> 32) + c;
  };
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddInputBuffer(kts::N, kts::Ref_Odd);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}
