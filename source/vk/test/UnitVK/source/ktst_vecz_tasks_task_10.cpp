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

// Standard headers
#include <string>
#include <utility>

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Task_10_03_Vector_Loop) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    RunGeneric1D(kts::N);
  }
}

// Set local workgroup size to be the same as global work size, otherwise the
// test is assuming that atomic operations have global scope, which is not
// required by the OpenCL spec.
TEST_F(Execution, Task_10_05_Atomic_CmpXchg_Builtin) {
  if (clspvSupported_) {
    auto streamer(std::make_shared<AtomicStreamer<cl_int>>(-1, kts::localN));
    AddOutputBuffer(kts::BufferDesc(1, streamer));
    AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
    RunGeneric1D(kts::localN, kts::localN);
  }
}

TEST_F(Execution, Task_10_07_Break_Loop) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> Zero = [](size_t) { return 0; };
    AddInputBuffer(kts::N, Zero);
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_10_08_InsertElement_Constant_Index) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int a = kts::Ref_A(4 * x);
      const cl_int b = kts::Ref_A((4 * x) + 1);
      const cl_int c = 42;
      const cl_int d = kts::Ref_A((4 * x) + 3);
      return cl_int4{{a, b, c, d}};
    };
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_10_09_InsertElement_Runtime_Index) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> Indices = [](size_t x) {
      return kts::Ref_Identity(x) % 4;
    };
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int a = (x % 4 == 0) ? 42 : kts::Ref_A(4 * x);
      const cl_int b = (x % 4 == 1) ? 42 : kts::Ref_A((4 * x) + 1);
      const cl_int c = (x % 4 == 2) ? 42 : kts::Ref_A((4 * x) + 2);
      const cl_int d = (x % 4 == 3) ? 42 : kts::Ref_A((4 * x) + 3);
      return cl_int4{{a, b, c, d}};
    };
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    AddInputBuffer(kts::N, Indices);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_10_10_ExtractElement_Constant_Index) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int a = 4;
      const cl_int b = 4;
      const cl_int c = kts::Ref_A(4 * x);
      ;
      const cl_int d = 4;
      return cl_int4{{a, b, c, d}};
    };
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_10_11_ExtractElement_Runtime_Index) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> Indices = [](size_t x) {
      return kts::Ref_Identity(x) % 4;
    };
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int a = 4;
      const cl_int b = 4;
      const cl_int c = kts::Ref_A(4 * x);
      const cl_int d = 4;
      return cl_int4{{a, b, c, d}};
    };
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    AddInputBuffer(kts::N, Indices);
    RunGeneric1D(kts::N);
  }
}
