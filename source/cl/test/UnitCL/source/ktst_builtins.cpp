// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Third-party headers
#include <gtest/gtest.h>

// In-house headers
#include "kts/execution.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Builtins_01_Fmin_Vector_Scalar_NaN) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  AddInputBuffer(kts::N, refIn);
  AddPrimitive(0);
  AddOutputBuffer(kts::N, refIn);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_02_Fmax_Vector_Scalar_NaN) {
  auto refIn = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
  AddInputBuffer(kts::N, refIn);
  AddPrimitive(0);
  AddOutputBuffer(kts::N, refIn);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_03_Mad_Conversions) {
  kts::Reference1D<cl_float> refOut = [](size_t) {
    return static_cast<cl_float>(10.0f * 1.2f + 20.0f);
  };
  AddPrimitive(static_cast<cl_char>(10));
  AddPrimitive(20);
  AddPrimitive(1.2f);
  AddOutputBuffer(1, refOut);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Builtins_04_Mad24_Conversions) {
  kts::Reference1D<cl_float> refOut = [](size_t) {
    return static_cast<cl_float>(10 * 1 + 20);
  };
  AddPrimitive(static_cast<cl_char>(10));
  AddPrimitive(20);
  AddPrimitive(1.2f);
  AddOutputBuffer(1, refOut);

  RunGeneric1D(kts::N);
}
