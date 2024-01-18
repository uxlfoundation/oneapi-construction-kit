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

TEST_F(Execution, Task_02_01_Abs_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Opposite);
    AddOutputBuffer(kts::N, kts::Ref_Identity);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_02_Dot_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Float);
    AddInputBuffer(kts::N, kts::Ref_NegativeOffset);
    AddOutputBuffer(kts::N, kts::Ref_Dot);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_03_Distance_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Float);
    AddInputBuffer(kts::N, kts::Ref_NegativeOffset);
    AddOutputBuffer(kts::N, kts::Ref_Distance);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_04_Fabs_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_NegativeOffset);
    AddOutputBuffer(kts::N, kts::Ref_Abs);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_05_Clz_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Identity);
    AddOutputBuffer(kts::N, kts::Ref_Clz);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_06_Clamp_Builtin) {
  if (clspvSupported_) {
    const float low = 0.0f;
    const float high = 0.0f;
    kts::Reference1D<float> refOut = [low, high](size_t x) {
      const float v = kts::Ref_Float(x);
      return std::min(std::max(v, low), high);
    };
    AddInputBuffer(kts::N, kts::Ref_Float);
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(low);
    AddPrimitive(high);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_07_Length_Builtin) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Float);
    AddOutputBuffer(kts::N, kts::Ref_Length);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_02_08_Barrier_Add) {
  if (clspvSupported_) {
    const cl_int array_size = 16;
    const unsigned groupSize = array_size / 2;
    kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };
    AddInputBuffer(2 * groupSize, kts::Ref_A);
    AddInputBuffer(2 * groupSize, kts::Ref_B);
    AddOutputBuffer(2 * groupSize, refOut);
    RunGeneric1D(2 * groupSize, groupSize);
  }
}
