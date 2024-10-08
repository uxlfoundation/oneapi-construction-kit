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

#include <cmath>
#include <cstdio>
#include <cstring>

#include "Common.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_03_01_Copy4) {
  auto ref = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  AddInputBuffer(kts::N, ref);
  AddOutputBuffer(kts::N, ref);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_02_Add4) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  auto refIn2 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_B);
  auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_03_Abs4_Builtin) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
  auto refOut = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Abs);
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_04_Dot4_Builtin) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
  auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  kts::Reference1D<cl_float> refOut = [refIn1, refIn2](size_t x) {
    const cl_float4 v1 = refIn1(x);
    const cl_float4 v2 = refIn2(x);
    return (v1.s[0] * v2.s[0]) + (v1.s[1] * v2.s[1]) + (v1.s[2] * v2.s[2]) +
           (v1.s[3] * v2.s[3]);
  };

  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_05_Distance4_Builtin) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_NegativeOffset);
  auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  kts::Reference1D<cl_float> refOut = [refIn1, refIn2](size_t x) {
    const cl_float4 v1 = refIn1(x);
    const cl_float4 v2 = refIn2(x);
    const cl_float d0 = (v1.s[0] - v2.s[0]);
    const cl_float d1 = (v1.s[1] - v2.s[1]);
    const cl_float d2 = (v1.s[2] - v2.s[2]);
    const cl_float d3 = (v1.s[3] - v2.s[3]);
    return std::sqrt((d0 * d0) + (d1 * d1) + (d2 * d2) + (d3 * d3));
  };

  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_06_Ternary4) {
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

TEST_P(Execution, Task_03_07_Transpose4) {
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    const cl_int chunkID = ix % 4;
    const cl_int base = ((ix - chunkID) * 4) + chunkID;
    cl_int4 v;
    v.s[0] = kts::Ref_A(base + 0);
    v.s[1] = kts::Ref_A(base + 4);
    v.s[2] = kts::Ref_A(base + 8);
    v.s[3] = kts::Ref_A(base + 12);
    return v;
  };
  AddInputBuffer(kts::N * 4, refIn);
  AddOutputBuffer(kts::N * 4, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_08_Clz4_Builtin) {
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Identity);
  auto refOut = kts::BuildVec4Reference1D<cl_uint4>(kts::Ref_Clz);
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_09_Clamp4_Builtin) {
  const float low = 0.0f;
  const float high = 0.0f;
  kts::Reference1D<cl_float4> refOut = [low, high](size_t x) {
    const float v0 = kts::Ref_Float((x * 4) + 0);
    const float v1 = kts::Ref_Float((x * 4) + 1);
    const float v2 = kts::Ref_Float((x * 4) + 2);
    const float v3 = kts::Ref_Float((x * 4) + 3);
    cl_float4 v;
    v.s[0] = std::min(std::max(v0, low), high);
    v.s[1] = std::min(std::max(v1, low), high);
    v.s[2] = std::min(std::max(v2, low), high);
    v.s[3] = std::min(std::max(v3, low), high);
    return v;
  };
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(low);
  AddPrimitive(high);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_10_S2V_Int) {
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    const cl_int y = kts::Ref_A(x);
    cl_int4 v;
    v.s[0] = y;
    v.s[1] = y;
    v.s[2] = y;
    v.s[3] = y;
    return v;
  };

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_11_Sum_Reduce4) {
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

TEST_P(Execution, DISABLED_Task_03_12_V2S2V2S) {
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

TEST_P(Execution, Task_03_13_Copy2) {
  auto ref = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);
  AddInputBuffer(kts::N, ref);
  AddOutputBuffer(kts::N, ref);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_14_Add2) {
  auto refIn1 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);
  auto refIn2 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_B);
  auto refOut = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_15_Copy3) {
  auto ref = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_A);
  AddInputBuffer(kts::N, ref);
  AddOutputBuffer(kts::N, ref);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_16_Add3) {
  auto refIn1 = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_A);
  auto refIn2 = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_B);
  auto refOut = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_17_Length4_Builtin) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  kts::Reference1D<cl_float> refOut = [&refIn](size_t x) {
    const cl_float4 v = refIn(x);
    return std::sqrt((v.s[0] * v.s[0]) + (v.s[1] * v.s[1]) + (v.s[2] * v.s[2]) +
                     (v.s[3] * v.s[3]));
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_19_Add4_I32_Tid) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  auto refIn2 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_B);
  auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_20_All3) {
  kts::Reference1D<ucl::PackedInt3> refIn = [](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    ucl::PackedInt3 v;
    v[0] = -(ix & 1);
    v[1] = -(ix & 2);
    v[2] = -ix;
    return v;
  };
  kts::Reference1D<cl_int> refOut = [&refIn](size_t x) {
    ucl::PackedInt3 input = refIn(x);
    const unsigned reduced =
        (unsigned)input[0] & (unsigned)input[1] & (unsigned)input[2];
    return (reduced >> 31);
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_21_Any3) {
  kts::Reference1D<ucl::PackedInt3> refIn = [](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    ucl::PackedInt3 v;
    v[0] = -(ix & 1);
    v[1] = -(ix & 2);
    v[2] = 0;
    return v;
  };
  kts::Reference1D<cl_int> refOut = [&refIn](size_t x) {
    ucl::PackedInt3 input = refIn(x);
    const unsigned reduced =
        (unsigned)input[0] | (unsigned)input[1] | (unsigned)input[2];
    return (reduced >> 31);
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_22_As_UChar4_UInt) {
  // This is really a copy test. No need to make testing complicated.
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_23_As_UInt_UChar4) {
  // This is really a copy test. No need to make testing complicated.
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_24_As_UInt4_Float4) {
  // This is really a copy test. No need to make testing complicated.
  AddInputBuffer(kts::N * 4, kts::Ref_A);
  AddOutputBuffer(kts::N * 4, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_03_25_As_UShort2_UChar4) {
  // This is really a copy test. No need to make testing complicated.
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

// Set local workgroup size to be the same as global work size, otherwise the
// test is assuming that atomic operations have global scope, which is not
// required by the OpenCL spec.
TEST_P(Execution, Task_03_26_Atom_Inc_Builtin_Int) {
  const cl_int base_value = 42;
  auto streamer(
      std::make_shared<AtomicStreamer<cl_int>>(base_value, kts::localN));
  AddOutputBuffer(kts::BufferDesc(1, streamer));
  AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
  RunGeneric1D(kts::localN, kts::localN);
}

TEST_P(Execution, Task_03_26_Atom_Inc_Builtin_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const cl_long base_value = 42;
  auto streamer(
      std::make_shared<AtomicStreamer<cl_long>>(base_value, kts::localN));
  AddOutputBuffer(kts::BufferDesc(1, streamer));
  AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
  RunGeneric1D(kts::localN, kts::localN);
}

// Set local workgroup size to be the same as global work size, otherwise the
// test is assuming that atomic operations have global scope, which is not
// required by the OpenCL spec.
TEST_P(Execution, Task_03_27_Atomic_Inc_Builtin) {
  const cl_int base_value = 42;
  auto streamer(
      std::make_shared<AtomicStreamer<cl_int>>(base_value, kts::localN));
  AddOutputBuffer(kts::BufferDesc(1, streamer));
  AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
  RunGeneric1D(kts::localN, kts::localN);
}

TEST_P(Execution, DISABLED_Task_03_28_Normalize4_Builtin) {
  const size_t numSamples = 4;
  const cl_float4 inputs[numSamples] = {
      {{HFLOAT(0x1p0f), HFLOAT(0x0p0f), HFLOAT(0x0p0f), HFLOAT(0x0p0f)}},
      {{HFLOAT(0x1.7ba91ep+48f), HFLOAT(0x1.6580b2p+63f),
        HFLOAT(-0x1.78583ep+123f), HFLOAT(0x1.0ccfd6p-19f)}},
      {{HFLOAT(0x1.e5113ep+106f), HFLOAT(-0x1.5c00eep+115f),
        HFLOAT(0x1.8d3696p+19f), HFLOAT(0x1.205b68p+70f)}},
      {{HFLOAT(0x1.9332aep-125f), HFLOAT(0x1.5677bep-75f),
        HFLOAT(-0x1.239e96p-87f), HFLOAT(-0x1.5e3296p-71f)}}};
  const cl_float4 outputs[numSamples] = {
      {{HFLOAT(0x1p0f), HFLOAT(0x0p0f), HFLOAT(0x0p0f), HFLOAT(0x0p0f)}},
      {{HFLOAT(0x1.02416ep-75f), HFLOAT(0x1.e65dc8p-61f), HFLOAT(-0x1p+0f),
        HFLOAT(0x1.6cp-143f)}},
      {{HFLOAT(0x1.64d37cp-9f), HFLOAT(-0x1.ffff84p-1f),
        HFLOAT(0x1.2432dep-96f), HFLOAT(0x1.a83e54p-46f)}},
      {{HFLOAT(0x1.2631f2p-54f), HFLOAT(0x1.f3c41ep-5f),
        HFLOAT(-0x1.a99012p-17f), HFLOAT(-0x1.ff0bdcp-1f)}}};
  kts::Reference1D<cl_float4> refIn = [=, &inputs](size_t x) {
    const cl_float4 zero = {{0.0f, 0.0f, 0.0f, 0.0f}};
    return (x < numSamples) ? inputs[x] : zero;
  };
  kts::Reference1D<cl_float4> refOut = [=, &outputs](size_t x) {
    const cl_float4 zero = {{0.0f, 0.0f, 0.0f, 0.0f}};
    return (x < numSamples) ? outputs[x] : zero;
  };
  AddInputBuffer(numSamples, refIn);
  AddOutputBuffer(numSamples, refOut);
  RunGeneric1D(numSamples);
}

TEST_P(Execution, Task_03_29_Modf4_Builtin) {
  kts::Reference1D<cl_float> refIn = [](size_t x) {
    return static_cast<cl_float>(x + ((x % 2) * 0.5f));
  };
  kts::Reference1D<cl_float> refFrac = [](size_t x) {
    return static_cast<cl_float>((x % 2) * 0.5f);
  };
  kts::Reference1D<cl_float> refInt = [](size_t x) {
    return static_cast<cl_float>(x);
  };

  AddInputBuffer(kts::N * 4, refIn);
  AddOutputBuffer(kts::N * 4, refFrac);
  AddOutputBuffer(kts::N * 4, refInt);
  RunGeneric1D(kts::N);
}
