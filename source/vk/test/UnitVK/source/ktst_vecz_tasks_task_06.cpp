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

TEST_F(Execution, Task_06_01_Copy_If_Constant) {
  if (clspvSupported_) {
    // Test with the first constant that exercises one path.
    const cl_int C1 = 42;
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    AddPrimitive(C1);
    RunGeneric1D(kts::N);

    // Test with the second constant that exercises the other path.
    const cl_int C2 = 17;
    kts::Reference1D<cl_int> refOut2 = [](size_t) { return 0; };
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut2);
    AddPrimitive(C2);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_06_02_Copy_If_Even_Group) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) {
      const size_t gid = x / kts::localN;
      return ((gid & 1) == 0) ? kts::Ref_A(x) : -1;
    };
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N, kts::localN);
  }
}
