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

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>

#include "Common.h"
#include "Device.h"
#include "cargo/utility.h"
#include "kts/execution.h"
#include "kts/precision.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Regression_01_Pointer_To_Long_Cast) {
  // This test was compiled for SPIRV and Offline with the value of 256. You
  // will need to recompile these targets.
  ASSERT_EQ(kts::N, 256);
  AddMacro("N", (unsigned int)kts::N);
  AddInputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_02_Work_Dim) {
  kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

// Initial test-case from Redmine #5612.  The x86 back-end was producing wrong
// code for this kernel.
TEST_P(Execution, Regression_03_Shuffle_Cast) {
  kts::Reference1D<cl_short> refIn = [](size_t) {
    return static_cast<cl_short>(7);
  };

  kts::Reference1D<cl_short> refOut = [](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    // This is testing a particular vector shuffle, so the expected output is a
    // little bit convoluted.
    return static_cast<cl_short>(((ix % 8) == 3) ? 7 : 0);
  };

  AddInputBuffer(16, refIn);
  AddOutputBuffer(kts::N * 8, refOut);
  RunGeneric1D(kts::N);
}

// Another test-case for Redmine #5612.  This case was not fixed by the
// upstream changes that fixed the above test-case.
TEST_P(Execution, Regression_04_Shuffle_Copy) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    // This is testing a particular vector shuffle, so the expected output is a
    // little bit convoluted.
    return ((ix % 8) == 0) ? (ix / 8) * 2 : 0;
  };

  AddInputBuffer(kts::N * 2, kts::Ref_Identity);
  AddOutputBuffer(kts::N * 8, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_05_Bit_Shift) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    return kts::Ref_Identity(x) << (35 % 32);
  };

  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_06_Cross_Elem4_Zero) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);

  kts::Reference1D<cl_float4> refOut = [](size_t) {
    // cross(x, x) == 0
    cl_float4 v;
    v.s[0] = 0;
    v.s[1] = 0;
    v.s[2] = 0;
    v.s[3] = 0;
    return v;
  };

  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_07_Mad_Sat_Long) {
  // Skip third party implementations that fail this test, we think the test is
  // correct so silence the failures to allow cross-validation of UnitCL
  // against other implementations.
  if (UCL::isDevice_IntelNeo(this->device)) {
    printf(
        "Intel NEO driver appears to get wrong result for mad_sat, so we "
        "skip the test there.\n");
    GTEST_SKIP();
  }
  if (UCL::isDevice_Oclgrind(this->device)) {
    // https://github.com/jrprice/Oclgrind/issues/117
    printf(
        "Oclgrind appears to get wrong result for mad_sat, so we skip the "
        "test there.\n");
    GTEST_SKIP();
  }

  const cl_long As[] = {6,
                        3037000499,
                        6,
                        0x7fffffffffffffff,
                        -9223372036854775807 - 1,
                        4406688104284751};
  const cl_long Bs[] = {0x5972C40A98CEEF9A, 3037000499, 3,
                        0x7fffffffffffffff, 1,          -1};
  const cl_long Cs[] = {0,  1,        0x7fffffffffffffff, 0x7fffffffffffffff,
                        -1, 619354410};
  const cl_long Outs[] = {0x7fffffffffffffff,       0x7ffffffe9ea1dc2a,
                          0x7fffffffffffffff,       0x7fffffffffffffff,
                          -9223372036854775807 - 1, -4406687484930341};

  const size_t test_cases = sizeof(As) / sizeof(As[0]);

  kts::Reference1D<cl_long> refInA = [=, &As](size_t x) {
    return As[x % test_cases];
  };
  kts::Reference1D<cl_long> refInB = [=, &Bs](size_t x) {
    return Bs[x % test_cases];
  };
  kts::Reference1D<cl_long> refInC = [=, &Cs](size_t x) {
    return Cs[x % test_cases];
  };
  kts::Reference1D<cl_long> refOut = [=, &Outs](size_t x) {
    return Outs[x % test_cases];
  };

  AddInputBuffer(kts::N, refInA);
  AddInputBuffer(kts::N, refInB);
  AddInputBuffer(kts::N, refInC);
  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_08_Mem2Reg_Bitcast) {
  kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };

  // The output is not important, the issue tested causes a compilation error.
  // See redmine #8413.
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_09_Mem2Reg_Store) {
  kts::Reference1D<cl_long> refOut = [](size_t) { return 42; };

  // The output is somewhat important, but this bug causes a compilation failure
  // as well. See redmine #8506.
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_10_Dont_Mask_Workitem_Builtins) {
  // Tests for Redmine #8883

  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return kts::Ref_Identity(x + 2) * 3;
  };
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    const size_t local_id = x % kts::localN;
    if (local_id > 0) {
      return (kts::Ref_Identity(x) + 2) * 3;
    } else {
      return 42;
    }
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Regression_11_InterleavedGroupCombine_Safety) {
  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  kts::Reference1D<cl_double4> refInA = [](size_t) {
    return cl_double4{{1.0, 2.0, 3.0, 4.0}};
  };
  kts::Reference1D<cl_double4> refInB = [](size_t) {
    return cl_double4{{2.0, 3.0, 4.0, 5.0}};
  };
  kts::Reference1D<cl_double4> refInC = [](size_t) {
    return cl_double4{{3.0, 4.0, 5.0, 6.0}};
  };
  kts::Reference1D<cl_double4> refInD = [](size_t) {
    return cl_double4{{4.0, 5.0, 6.0, 7.0}};
  };
  kts::Reference1D<cl_double4> refInE = [](size_t) {
    return cl_double4{{2.0, 5.0, 2.0, 7.0}};
  };

  kts::Reference1D<cl_double4> refOut = [](size_t) {
    return cl_double4{{-7.0, -11.0, -80.0, -27.0}};
  };

  AddInputBuffer(kts::N, refInA);
  AddInputBuffer(kts::N, refInB);
  AddInputBuffer(kts::N, refInC);
  AddInputBuffer(kts::N, refInD);
  AddInputBuffer(kts::N, refInE);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_12_Isgreater_Double3_Vloadstore) {
  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  // Tests for Redmine #8776

  // The kernel used in this test uses vload3 and vstore3, so we need to
  // manually pack the buffers.
  cl_double InputsA[4] = {2.0, 2.0, 1.0, 1.0};
  cl_double InputsB[4] = {1.0, 2.0, 2.0, 1.0};
  cl_long Out[4] = {-1, 0, 0, 0};

  kts::Reference1D<cl_double> refInA = [&InputsA](size_t x) {
    return InputsA[x % 4];
  };
  kts::Reference1D<cl_double> refInB = [&InputsB](size_t x) {
    return InputsB[x % 4];
  };
  kts::Reference1D<cl_long> refOut = [&Out](size_t x) { return Out[x % 4]; };

  AddInputBuffer(kts::N * 3, refInA);
  AddInputBuffer(kts::N * 3, refInB);
  AddOutputBuffer(kts::N * 3, refOut);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_13_Varying_Alloca) {
  const cl_int N = 64;
  kts::Reference1D<cl_int4> refIn = [](size_t x) {
    cl_int v = kts::Ref_Identity(x);
    return cl_int4{{v, v + 1, v - 1, v * 2}};
  };
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    cl_int v = kts::Ref_Identity(x);
    return cl_int4{{v, v + 1, v - 1, v * 2}};
  };

  AddInputBuffer(N, refIn);
  AddOutputBuffer(N, refOut);
  RunGeneric1D(N);
}

TEST_P(Execution, Regression_14_Argument_Stride) {
  static constexpr cl_int Stride = 3;
  static constexpr cl_int Max = 1 << 30;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return kts::Ref_Identity(x) % Max;
  };
  kts::Reference1D<cl_int> refOut = [&refIn](size_t x) {
    return kts::Ref_Identity(x) % Stride == 0 ? refIn(x) : 1;
  };

  AddInputBuffer(kts::N * Stride, refIn);
  AddOutputBuffer(kts::N * Stride, refOut);
  AddPrimitive(Stride);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_15_Negative_Stride) {
  const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x * x);
  };
  kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) {
    return refIn(MaxIndex - x) + refIn(x);
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(MaxIndex);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_16_Negative_Argument_Stride) {
  const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x * x);
  };
  kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) {
    return refIn(MaxIndex - x) + refIn(x);
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(-1);
  AddPrimitive(MaxIndex);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_17_Scalar_Select_Transform) {
  // Inputs are not important, since this bug caused a compilation failure
  // because a function was called with the wrong arguments.
  kts::Reference1D<cl_int4> refA = [](size_t x) -> cl_int4 {
    cl_int A = kts::Ref_A(x);
    return cl_int4{{A, A, A, A}};
  };
  kts::Reference1D<cl_int4> refB = [](size_t x) -> cl_int4 {
    cl_int B = kts::Ref_B(x);
    return cl_int4{{B, B, B, B}};
  };
  kts::Reference1D<cl_int4> refOut = [&refA, &refB](size_t x) {
    return x % 2 == 0 ? refA(x) : refB(x);
  };

  AddInputBuffer(kts::N, refA);
  AddInputBuffer(kts::N, refB);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

// The following test ensures that Masked Stores created by the Ternary
// Transform Pass get a second mask applied to them by Linearization.
TEST_P(Execution, Regression_17_Scalar_Select_Transform_2) {
  const int clear = 180;

  kts::Reference1D<cl_int> refOutB = [&](size_t x) {
    if (x >= 125) {
      return x % 2 == 0 ? kts::Ref_A(x) : clear;
    } else {
      return clear;
    }
  };

  kts::Reference1D<cl_int> refOutC = [&](size_t x) {
    if (x >= 125) {
      return x % 2 == 0 ? clear : kts::Ref_A(x);
    } else {
      return clear;
    }
  };

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, refOutB);
  AddOutputBuffer(kts::N, refOutC);
  AddPrimitive(clear);
  RunGeneric1D(kts::N);
}

// The following test passed at the time of writing. It exists to ensure that
// any future fix to the Ternary Transform Pass for Vector Selects doesn't
// break when the masked stores created need to be doubly-masked.
TEST_P(Execution, Regression_17_Scalar_Select_Transform_3) {
  const cl_int4 clear{{180, 181, 182, 183}};

  kts::Reference1D<cl_int4> refA = [](size_t x) -> cl_int4 {
    cl_int A = kts::Ref_A(x);
    return cl_int4{{A, A, A, A}};
  };

  kts::Reference1D<cl_int4> refOutB = [&](size_t x) {
    if (x >= 125) {
      return x % 2 == 0 ? refA(x) : clear;
    } else {
      return clear;
    }
  };

  kts::Reference1D<cl_int4> refOutC = [&](size_t x) {
    if (x >= 125) {
      return x % 2 == 0 ? clear : refA(x);
    } else {
      return clear;
    }
  };

  AddInputBuffer(kts::N, refA);
  AddOutputBuffer(kts::N, refOutB);
  AddOutputBuffer(kts::N, refOutC);
  AddPrimitive(clear);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_18_Uniform_Alloca) {
  kts::Reference1D<cl_int> refOut = [](size_t x) -> cl_int {
    if (x == 0 || x == 1) {
      return kts::Ref_A(x);
    } else if (x % 2 == 0) {
      return 11;
    } else {
      return 13;
    }
  };

  AddInputBuffer(2, kts::Ref_A);
  AddOutputBuffer(kts::N * 2, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_19_Memcpy_Optimization) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection causes validation failure.
  }
  // This tests assumes that clang will optimize the struct copying into a
  // memcpy.
  kts::Reference1D<cl_int4> refIn = [](size_t x) {
    cl_int v = kts::Ref_Identity(x);
    return cl_int4{{v, v + 11, v + 12, v + 13}};
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refIn);
  RunGeneric1D(kts::N);
}

#define GLOBAL_ITEMS_1D 8
#define GLOBAL_ITEMS_2D 4
#define GLOBAL_ITEMS_3D 2
#define LOCAL_ITEMS_1D 4
#define LOCAL_ITEMS_2D 2
#define LOCAL_ITEMS_3D 1
#define GROUP_RANGE_1D (GLOBAL_ITEMS_1D / LOCAL_ITEMS_1D)
#define GROUP_RANGE_2D (GLOBAL_ITEMS_2D / LOCAL_ITEMS_2D)
#define GROUP_RANGE_3D (GLOBAL_ITEMS_3D / LOCAL_ITEMS_3D)
#define GLOBAL_ITEMS_TOTAL (GLOBAL_ITEMS_1D * GLOBAL_ITEMS_2D * GLOBAL_ITEMS_3D)
#define LOCAL_ITEMS_TOTAL (LOCAL_ITEMS_1D * LOCAL_ITEMS_2D * LOCAL_ITEMS_3D)
#define GROUP_RANGE_TOTAL (GLOBAL_ITEMS_TOTAL / LOCAL_ITEMS_TOTAL)

// This generator was created to replicate a failure seen on an external test.
static cl_int calc_group_barrier(size_t x, int vector_width) {
  for (int k = 0; k < GROUP_RANGE_1D; ++k) {
    for (int j = 0; j < GROUP_RANGE_2D; ++j) {
      for (int i = 0; i < GROUP_RANGE_3D; ++i) {
        const int linearIndex =
            (k * GROUP_RANGE_2D * GROUP_RANGE_1D) + (j * GROUP_RANGE_1D) + i;
        const int g = linearIndex * vector_width;
        switch (x - g) {
          case 0:
            return i;
            break;
          case 1:
            return j;
            break;
          case 2:
            return k;
            break;
          case 3:
            return linearIndex;
            break;
          case 4:
            if (vector_width == 8) return i;
            break;
          case 5:
            if (vector_width == 8) return j;
            break;
          case 6:
            if (vector_width == 8) return k;
            break;
          case 7:
            if (vector_width == 8) return linearIndex;
            break;
          default:
            break;  // No match on this iteration.
        }
      }
    }
  }
  return -1;
}

TEST_P(Execution, Regression_20_Group_Barrier_0) {
  const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D,
                                 GLOBAL_ITEMS_3D};
  const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D, LOCAL_ITEMS_3D};
  const size_t vector_width = 4;

  kts::Reference1D<cl_int> refOut = [=](size_t x) -> cl_int {
    return calc_group_barrier(x, vector_width);
  };

  AddMacro("GROUP_RANGE_1D", GROUP_RANGE_1D);
  AddMacro("GROUP_RANGE_2D", GROUP_RANGE_2D);

  AddOutputBuffer(GROUP_RANGE_TOTAL * vector_width, refOut);
  RunGenericND(3, global_range, local_range);
}

TEST_P(Execution, Regression_20_Group_Barrier_1) {
  const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D,
                                 GLOBAL_ITEMS_3D};
  const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D, LOCAL_ITEMS_3D};
  const size_t vector_width = 4;

  AddMacro("GROUP_RANGE_1D", GROUP_RANGE_1D);
  AddMacro("GROUP_RANGE_2D", GROUP_RANGE_2D);

  kts::Reference1D<cl_int> refOut = [=](size_t x) -> cl_int {
    return calc_group_barrier(x, vector_width);
  };

  AddOutputBuffer(GROUP_RANGE_TOTAL * vector_width, refOut);
  RunGenericND(3, global_range, local_range);
}

TEST_P(Execution, Regression_20_Group_Barrier_2) {
  const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D,
                                 GLOBAL_ITEMS_3D};
  const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D, LOCAL_ITEMS_3D};
  const size_t vector_width = 8;

  AddMacro("LOCAL_ITEMS_1D", LOCAL_ITEMS_1D);
  AddMacro("LOCAL_ITEMS_2D", LOCAL_ITEMS_2D);
  AddMacro("GROUP_RANGE_1D", GROUP_RANGE_1D);
  AddMacro("GROUP_RANGE_2D", GROUP_RANGE_2D);

  kts::Reference1D<cl_int> refOut = [=](size_t x) -> cl_int {
    return calc_group_barrier(x, vector_width);
  };

  AddOutputBuffer(GROUP_RANGE_TOTAL * vector_width, refOut);
  RunGenericND(3, global_range, local_range);
}

TEST_P(Execution, Regression_20_Group_Barrier_3) {
  const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D,
                                 GLOBAL_ITEMS_3D};
  const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D, LOCAL_ITEMS_3D};
  const size_t vector_width = 8;

  AddMacro("LOCAL_ITEMS_1D", LOCAL_ITEMS_1D);
  AddMacro("LOCAL_ITEMS_2D", LOCAL_ITEMS_2D);
  AddMacro("GROUP_RANGE_1D", GROUP_RANGE_1D);
  AddMacro("GROUP_RANGE_2D", GROUP_RANGE_2D);

  kts::Reference1D<cl_int> refOut = [=](size_t x) -> cl_int {
    return calc_group_barrier(x, vector_width);
  };

  AddOutputBuffer(GROUP_RANGE_TOTAL * vector_width, refOut);
  RunGenericND(3, global_range, local_range);
}

#undef GLOBAL_ITEMS_1D
#undef GLOBAL_ITEMS_2D
#undef GLOBAL_ITEMS_3D
#undef LOCAL_ITEMS_1D
#undef LOCAL_ITEMS_2D
#undef LOCAL_ITEMS_3D
#undef GROUP_RANGE_1D
#undef GROUP_RANGE_2D
#undef GROUP_RANGE_3D
#undef GLOBAL_ITEMS_TOTAL
#undef LOCAL_ITEMS_TOTAL
#undef GROUP_RANGE_TOTAL

// Regression_20_Group_Barrier_4 uses different defines from {0, 1, 2, 3}.
#define GLOBAL_ITEMS_1D 4
#define GLOBAL_ITEMS_2D 2
#define LOCAL_ITEMS_1D 2
#define LOCAL_ITEMS_2D 2
#define GROUP_RANGE_1D (GLOBAL_ITEMS_1D / LOCAL_ITEMS_1D)
#define GLOBAL_ITEMS_TOTAL (GLOBAL_ITEMS_1D * GLOBAL_ITEMS_2D)
#define LOCAL_ITEMS_TOTAL (LOCAL_ITEMS_1D * LOCAL_ITEMS_2D)
#define GROUP_RANGE_TOTAL (GLOBAL_ITEMS_TOTAL / LOCAL_ITEMS_TOTAL)

TEST_P(Execution, Regression_20_Group_Barrier_4) {
  const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D};
  const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D};

  AddMacro("LOCAL_ITEMS_1D", LOCAL_ITEMS_1D);
  AddMacro("GROUP_RANGE_1D", GROUP_RANGE_1D);

  kts::Reference1D<cl_int> refOut = [=](size_t) -> int { return 7; };

  AddOutputBuffer(GROUP_RANGE_TOTAL, refOut);
  RunGenericND(2, global_range, local_range);
}

#undef GLOBAL_ITEMS_1D
#undef GLOBAL_ITEMS_2D
#undef LOCAL_ITEMS_1D
#undef LOCAL_ITEMS_2D
#undef GROUP_RANGE_1D
#undef GLOBAL_ITEMS_TOTAL
#undef LOCAL_ITEMS_TOTAL
#undef GROUP_RANGE_TOTAL

TEST_P(Execution, Regression_21_Unaligned_Load) {
  AddInputBuffer(kts::N * 3, kts::Ref_Identity);
  AddOutputBuffer(kts::N * 3, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_22_Unaligned_Load2) {
  AddInputBuffer(kts::N * 2, kts::Ref_Identity);
  AddOutputBuffer(kts::N * 2, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_23_Shuffle_Copy) {
  const int output[] = {10, 10, 1, 2, 11, 11, 12, 3, 12};
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x) + 1;
  };
  kts::Reference1D<cl_int> refOut = [&output](size_t x) { return output[x]; };

  AddInputBuffer(32, refIn);
  AddOutputBuffer(9, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_24_MemOp_Loop_Dep) {
  // This bug caused a compilation failure, so the results are not too
  // important. Still good to have though, since we are deleting a bunch of
  // instructions.
  //
  auto ref = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  AddInputBuffer(kts::N, ref);
  AddOutputBuffer(kts::N, ref);
  AddPrimitive(0);
  AddPrimitive(1);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_25_Multiple_Inlining) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection causes a deadlock during kernel execeution.
  }
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

// Do not add additional tests here or this file may become too large to link.
// Instead, extend the newest ktst_regression_${NN}.cpp file.
