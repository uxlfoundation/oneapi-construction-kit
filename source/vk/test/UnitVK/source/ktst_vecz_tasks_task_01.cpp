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

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Task_01_01_Copy) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_02_Add) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddInputBuffer(kts::N, kts::Ref_B);
    AddOutputBuffer(kts::N, kts::Ref_Add);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_03_Mul_FMA) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_PlusOne);
    AddInputBuffer(kts::N, kts::Ref_MinusOne);
    AddInputBuffer(kts::N, kts::Ref_Triple);
    AddOutputBuffer(kts::N, kts::Ref_Mul);
    AddOutputBuffer(kts::N, kts::Ref_FMA);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_04_Ternary) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Odd);
    AddPrimitive(1);
    AddPrimitive(-1);
    AddOutputBuffer(kts::N, kts::Ref_Ternary);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_05_Broadcast) {
  if (clspvSupported_) {
    AddOutputBuffer(kts::N, kts::Ref_Identity);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_06_Broadcast_Uniform) {
  if (clspvSupported_) {
    cl_int foo = 41;
    kts::Reference1D<cl_int> refOut = [&foo](size_t) { return foo + 1; };
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(foo);
    RunGeneric1D(kts::N);
  }
}
