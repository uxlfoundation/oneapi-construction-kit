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

const size_t TRIPS = 256;

TEST_P(Execution, Task_05_01_Sum_Static_Trip) {
  kts::Reference1D<cl_int> refOut = [](size_t) {
    cl_int sum = 0;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      const cl_int a = kts::Ref_A(i);
      const cl_int b = kts::Ref_B(i);
      sum += (a * i) + b;
    }
    return sum;
  };
  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS, kts::Ref_A);
  AddInputBuffer(TRIPS, kts::Ref_B);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_05_02_SAXPY_Static_Trip) {
  const cl_float A = 1.5f;
  kts::Reference1D<cl_float> refOut = [A](size_t) {
    cl_float sum = 0.0f;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      const cl_float X = kts::Ref_NegativeOffset(i);
      const cl_float Y = kts::Ref_Float(i);
      sum += (A * X) + Y;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS, kts::Ref_NegativeOffset);
  AddInputBuffer(TRIPS, kts::Ref_Float);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_05_03_Sum_Static_Trip_Uniform) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    const cl_int localID = static_cast<cl_int>(x % kts::localN);
    cl_int sum = 0;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      const cl_int p = localID + i;
      const cl_int a = kts::Ref_A(p);
      const cl_int b = kts::Ref_B(p);
      sum += (a * i) + b;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_A);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_B);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Task_05_04_SAXPY_Static_Trip_Uniform) {
  const cl_float A = 1.5f;
  kts::Reference1D<cl_float> refOut = [A](size_t x) {
    const cl_int localID = static_cast<cl_int>(x % kts::localN);
    cl_float sum = 0.0f;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      const cl_int p = localID + i;
      const cl_float X = kts::Ref_NegativeOffset(p);
      const cl_float Y = kts::Ref_Float(p);
      sum += (A * X) + Y;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_NegativeOffset);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_Float);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(A);
  RunGeneric1D(kts::N, kts::localN);
}
