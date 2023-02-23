// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <gtest/gtest.h>

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

namespace {
const size_t local_wg_size = 16;

// Vector addition: C[x] = A[x] + B[x];
kts::Reference1D<cl_int> vaddInA = [](size_t x) {
  return kts::Ref_Identity(x) * 3 + 27;
};

kts::Reference1D<cl_int> vaddInB = [](size_t x) {
  return kts::Ref_Identity(x) * 7 + 41;
};

kts::Reference1D<cl_int> vaddOutC = [](size_t x) {
  return vaddInA(x) + vaddInB(x);
};
}  // namespace

TEST_P(Execution, Ext_Async_01_Simple_2D) {
  // The extension isn't supported in SPIRV yet, only test CL C
  if (!isSourceTypeIn({OPENCL_C, OFFLINE})) {
    GTEST_SKIP();
  }
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Ext_Async_02_Simple_3D) {
  // The extension isn't supported in SPIRV yet, only test CL C
  if (!isSourceTypeIn({OPENCL_C, OFFLINE})) {
    GTEST_SKIP();
  }
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddLocalBuffer(local_wg_size * sizeof(cl_int));
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}
