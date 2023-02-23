// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  cl_ulong long1 = 0xffffffffffffffffull;
  cl_uint int1 = 0xfefefefeull;

  kts::Reference1D<cl_int> refOut = [&ms1, &ms2, &ms3](size_t idx) {
    int r1 = (ms1.foo - ms1.bar) * ms1.gee;  // (2 - 1) * 2 = 2
    int r2 = (ms2.foo - ms2.bar) * ms2.gee;  // (4 - 3) * 5 = 5
    int r3 = (ms3.foo - ms3.bar) * ms3.gee;  // (6 - 9) * 7 = -21
    int out = (idx * r1) + (r2 * 10 - r3);   // idx * 2 + (5 *10 - (-21))
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
  AddLocalBuffer(kts::localN * sizeof(cl_int3));
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
      return static_cast<cl_ushort>(hash);
    }
  };

  AddInputBuffer(16, refIn);
  AddOutputBuffer(16, refOut);
  RunGeneric1D(16);
}

// Do not add tests beyond Regression_125* here, or the file may become too
// large to link. Instead, start a new ktst_regression_${NN}.cpp file.
