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
#include "kts/arguments_shared.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Regression_101_Extract_Vec3) {
  auto refIn = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_A);

  kts::Reference1D<cl_int> refOutX = [&](size_t x) { return refIn(x).x; };

  kts::Reference1D<cl_int> refOutY = [&](size_t x) { return refIn(x).y; };

  kts::Reference1D<cl_int> refOutZ = [&](size_t x) { return refIn(x).z; };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOutX);
  AddOutputBuffer(kts::N, refOutY);
  AddOutputBuffer(kts::N, refOutZ);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Regression_102_Shuffle_Vec3) {
  auto refIn = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_A);

  kts::Reference1D<ucl::Int3> refOut = [&](size_t x) {
    auto v = refIn(x);
    return ucl::Int3{{v.y, v.z, v.x}};
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

// Test that multiple 12 byte structs still are able to read the right values
// This showed up in the alignment for RISC-V, which increases the size to the
// next power of 2 when packing
TEST_P(Execution, Regression_103_Byval_Struct_Align) {
  struct my_struct {
    cl_int foo;
    cl_int bar;
    cl_int gee;
  };
  my_struct ms1 = {2, 1, 2};
  my_struct ms2 = {4, 3, 5};
  my_struct ms3 = {6, 9, 7};
  const cl_ulong long1 = 0xffffffffffffffffull;
  const cl_uint int1 = 0xfefefefeull;

  kts::Reference1D<cl_int> refOut = [&ms1, &ms2, &ms3](size_t idx) {
    const int r1 = (ms1.foo - ms1.bar) * ms1.gee;  // (2 - 1) * 2 = 2
    const int r2 = (ms2.foo - ms2.bar) * ms2.gee;  // (4 - 3) * 5 = 5
    const int r3 = (ms3.foo - ms3.bar) * ms3.gee;  // (6 - 9) * 7 = -21
    const int out = (idx * r1) + (r2 * 10 - r3);   // idx * 2 + (5 *10 - (-21))
    return out;
  };

  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(long1);
  AddInputBuffer(kts::N, kts::Ref_A);
  AddPrimitive(long1);
  AddPrimitive(int1);
  AddPrimitive(ms1);
  AddPrimitive(ms2);
  AddPrimitive(ms3);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_104_async_work_group_copy_int3) {
  auto refIn = kts::BuildVec3Reference1D<cl_int3>(kts::Ref_A);
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refIn);
  AddLocalBuffer<cl_int3>(kts::localN);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Regression_105_Alloca_BOSCC_Confuser) {
  const int output[] = {10, 10, 1, 2, 11, 11, 12, 3, 12};
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x) + 1;
  };
  kts::Reference1D<cl_int> refOut = [&output](size_t x) { return output[x]; };

  AddInputBuffer(32, refIn);
  AddOutputBuffer(9, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_106_Varying_LCSSA_Phi) {
  kts::Reference1D<cl_ushort> refIn = [](size_t x) {
    return static_cast<cl_ushort>(kts::Ref_A(x));
  };

  kts::Reference1D<cl_ushort> refOut = [&refIn](size_t x) {
    cl_ushort hash = 0;
    for (size_t i = 0; i < x; ++i) {
      // We explicitly cast 40499 to a cl_uint here to avoid UB. When the
      // cl_ushort multiplication overflows, the result is implicitly promoted
      // to cl_int, which then overflows causing UB.
      hash = hash * (cl_uint)40499 + (cl_ushort)refIn(i);
    }

    if (hash & 1) {
      return static_cast<cl_ushort>(refIn(x));
    } else {
      return hash;
    }
  };

  AddInputBuffer(16, refIn);
  AddOutputBuffer(16, refOut);
  RunGeneric1D(16);
}

// Test that structs maintain their user-facing ABI sizes. This bug showed up
// in the compiler, which was excessively padding structs. This meant that the
// generated LLVM code had incorrect GEP indexings and were over-stepping
// kernel arguments which were set correctly by the driver according to the
// user ABI.
TEST_P(ExecutionOpenCLC, Regression_107_Byval_Struct_Align) {
  struct my_innermost_struct {
    cl_char a;
  };
  struct my_innermost_struct_holder {
    my_innermost_struct s;
  };
  struct my_innermost_struct_holder_holder {
    my_innermost_struct_holder s;
  };
  struct my_struct_tuple {
    my_innermost_struct_holder_holder s;
    my_innermost_struct t;
  };
  struct my_struct {
    my_innermost_struct_holder_holder s;
    my_struct_tuple t;
  };
  my_struct s1 = {{{{2}}}, {{{5}}, {2}}};
  my_struct s2 = {{{{4}}}, {{{6}}, {5}}};
  static constexpr cl_int v = 7;
  static constexpr cl_int w = 9;

  kts::Reference1D<cl_int> refOutS1 = [&s1](size_t) {
    return s1.t.t.a + v + w;
  };
  kts::Reference1D<cl_int> refOutS2 = [&s2](size_t) {
    return s2.t.s.s.s.a + v + w;
  };

  static constexpr int TestN = 16;

  AddPrimitive(s1);
  AddPrimitive(v);
  AddOutputBuffer(TestN, refOutS1);
  AddPrimitive(s2);
  AddPrimitive(w);
  AddOutputBuffer(TestN, refOutS2);
  RunGeneric1D(TestN);
}

TEST_P(Execution, Regression_108_AbsDiff_Int) {
  // We won't vectorize if we know the local work-group size is only 1...
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_int> refInA = [](size_t) { return 0x8c7f0aac; };
  kts::Reference1D<cl_int> refInB = [](size_t) { return 0x1902f8c8; };
  kts::Reference1D<cl_uint> refOut = [](size_t) { return 0x8c83ee1c; };

  AddInputBuffer(1, refInA);
  AddInputBuffer(1, refInB);
  AddOutputBuffer(1, refOut);
  RunGeneric1D(1);
}

TEST_P(Execution, Regression_109_Libm_Native_Double_Input) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  auto refOneDouble = kts::Reference1D<cl_double>([](size_t) { return 1.0; });
  auto refOneUint = kts::Reference1D<cl_uint>([](size_t) { return 1; });

  const size_t num_functions = 14;
  AddBuildOption("-cl-fast-relaxed-math");
  AddInputBuffer(num_functions, refOneDouble);
  AddOutputBuffer(num_functions, refOneUint);
  RunGeneric1D(1, 1);
}

TEST_P(Execution, Regression_110_Libm_Native_Half_Input) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasHalfSupport(this->device)) {
    GTEST_SKIP();
  }

  auto refOneHalf = kts::Reference1D<cl_half>([](size_t) { return 0x3c00; });
  auto refOneUint = kts::Reference1D<cl_uint>([](size_t) { return 1; });

  const size_t num_functions = 14;
  AddBuildOption("-cl-fast-relaxed-math");
  AddInputBuffer(num_functions, refOneHalf);
  AddOutputBuffer(num_functions, refOneUint);
  RunGeneric1D(1, 1);
}

// Do not add tests beyond Regression_125* here, or the file may become too
// large to link. Instead, start a new ktst_regression_${NN}.cpp file.
