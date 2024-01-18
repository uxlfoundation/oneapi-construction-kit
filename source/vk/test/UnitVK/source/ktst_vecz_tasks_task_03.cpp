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

// System headers
#include <cmath>
#include <cstdio>
#include <cstring>

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Task_03_01_Copy4) {
  if (clspvSupported_) {
    auto ref = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    AddInputBuffer(kts::N, ref);
    AddOutputBuffer(kts::N, ref);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_02_Add4) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    auto refIn2 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_B);
    auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Add);
    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_03_Abs4_Builtin) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
    auto refOut = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Abs);
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_04_Dot4_Builtin) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
    auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
    kts::Reference1D<float> refOut = [refIn1, refIn2](size_t x) {
      const cl_float4 v1 = refIn1(x);
      const cl_float4 v2 = refIn2(x);
      return (v1.data[0] * v2.data[0]) + (v1.data[1] * v2.data[1]) +
             (v1.data[2] * v2.data[2]) + (v1.data[3] * v2.data[3]);
    };

    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_05_Distance4_Builtin) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
    auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
    kts::Reference1D<float> refOut = [refIn1, refIn2](size_t x) {
      const cl_float4 v1 = refIn1(x);
      const cl_float4 v2 = refIn2(x);
      const float d0 = (v1.data[0] - v2.data[0]);
      const float d1 = (v1.data[1] - v2.data[1]);
      const float d2 = (v1.data[2] - v2.data[2]);
      const float d3 = (v1.data[3] - v2.data[3]);
      return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3);
    };

    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_06_Ternary4) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Odd);
    const cl_int4 one = {{1, 1, 1, 1}};
    const cl_int4 minusOne = {{-1, -1, -1, -1}};
    auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Ternary_OpenCL);
    AddInputBuffer(kts::N, refIn1);
    AddPrimitive(one);
    AddPrimitive(minusOne);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_07_Transpose4) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int ix = kts::Ref_Identity(x);
      const cl_int chunkID = ix % 4;
      const cl_int base = (ix - chunkID) * 4 + chunkID;
      cl_int4 v;
      v.data[0] = kts::Ref_A(base + 0);
      v.data[1] = kts::Ref_A(base + 4);
      v.data[2] = kts::Ref_A(base + 8);
      v.data[3] = kts::Ref_A(base + 12);
      return v;
    };
    AddInputBuffer(kts::N * 4, refIn);
    AddOutputBuffer(kts::N * 4, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_08_Clz4_Builtin) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Identity);
    auto refOut = kts::BuildVec4Reference1D<cl_uint4>(kts::Ref_Clz);
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_09_Clamp4_Builtin) {
  if (clspvSupported_) {
    const float low = 0.0f;
    const float high = 0.0f;
    kts::Reference1D<cl_float4> refOut = [low, high](size_t x) {
      const float v0 = kts::Ref_Float((x * 4) + 0);
      const float v1 = kts::Ref_Float((x * 4) + 1);
      const float v2 = kts::Ref_Float((x * 4) + 2);
      const float v3 = kts::Ref_Float((x * 4) + 3);
      cl_float4 v;
      v.data[0] = std::min(std::max(v0, low), high);
      v.data[1] = std::min(std::max(v1, low), high);
      v.data[2] = std::min(std::max(v2, low), high);
      v.data[3] = std::min(std::max(v3, low), high);
      return v;
    };
    auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(low);
    AddPrimitive(high);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_10_S2V_Int) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int4> refOut = [](size_t x) {
      const cl_int y = kts::Ref_A(x);
      cl_int4 v;
      v.data[0] = y;
      v.data[1] = y;
      v.data[2] = y;
      v.data[3] = y;
      return v;
    };

    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, DISABLED_Task_03_11_Sum_Reduce4) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) {
      const cl_int i0 = kts::Ref_A((x * 4) + 0);
      const cl_int i1 = kts::Ref_A((x * 4) + 1);
      const cl_int i2 = kts::Ref_A((x * 4) + 2);
      const cl_int i3 = kts::Ref_A((x * 4) + 3);
      return i0 + i1 + i2 + i3;
    };

    AddInputBuffer(kts::N * 4, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, DISABLED_Task_03_12_V2S2V2S) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) {
      const cl_int i0 = kts::Ref_A((x * 4) + 0);
      const cl_int i1 = kts::Ref_A((x * 4) + 1);
      const cl_int i2 = kts::Ref_A((x * 4) + 2);
      const cl_int i3 = kts::Ref_A((x * 4) + 3);
      const unsigned sum = i0 + i1 + i2 + i3;
      const unsigned j0 = sum + 1;
      const unsigned j1 = sum + 2;
      const unsigned j2 = sum + 3;
      const unsigned j3 = sum + 4;
      const unsigned sum2 = j0 * j1 * j2 * j3;
      return sum2;
    };

    AddInputBuffer(kts::N * 4, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_13_Copy2) {
  if (clspvSupported_) {
    auto ref = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);
    AddInputBuffer(kts::N, ref);
    AddOutputBuffer(kts::N, ref);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_14_Add2) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);
    auto refIn2 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_B);
    auto refOut = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_Add);
    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_17_Length4_Builtin) {
  if (clspvSupported_) {
    auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
    kts::Reference1D<cl_float> refOut = [&refIn](size_t x) {
      const cl_float4 v = refIn(x);
      return std::sqrt((v.data[0] * v.data[0]) + (v.data[1] * v.data[1]) +
                       (v.data[2] * v.data[2]) + (v.data[3] * v.data[3]));
    };
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_03_19_Add4_I32_Tid) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
    auto refIn2 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_B);
    auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Add);
    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

// Set local workgroup size to be the same as global work size, otherwise the
// test is assuming that atomic operations have global scope, which is not
// required by the OpenCL spec.
TEST_F(Execution, Task_03_27_Atomic_Inc_Builtin) {
  if (clspvSupported_) {
    const cl_int base_value = 42;
    auto streamer(
        std::make_shared<AtomicStreamer<cl_int>>(base_value, kts::localN));
    AddOutputBuffer(kts::BufferDesc(1, streamer));
    AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
    RunGeneric1D(kts::localN, kts::localN);
  }
}
