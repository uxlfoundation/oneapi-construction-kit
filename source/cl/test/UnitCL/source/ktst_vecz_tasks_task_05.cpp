// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

const size_t TRIPS = 256;

TEST_P(Execution, Task_05_01_Sum_Static_Trip) {
  kts::Reference1D<cl_int> refOut = [](size_t) {
    cl_int sum = 0;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      cl_int a = kts::Ref_A(i);
      cl_int b = kts::Ref_B(i);
      sum += (a * i) + b;
    }
    return sum;
  };
  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS, kts::Ref_A);
  AddInputBuffer(TRIPS, kts::Ref_B);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_05_02_SAXPY_Static_Trip) {
  const cl_float A = 1.5f;
  kts::Reference1D<cl_float> refOut = [A](size_t) {
    cl_float sum = 0.0f;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      cl_float X = kts::Ref_NegativeOffset(i);
      cl_float Y = kts::Ref_Float(i);
      sum += (A * X) + Y;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS, kts::Ref_NegativeOffset);
  AddInputBuffer(TRIPS, kts::Ref_Float);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_05_03_Sum_Static_Trip_Uniform) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    cl_int localID = static_cast<cl_int>(x % kts::localN);
    cl_int sum = 0;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      cl_int p = localID + i;
      cl_int a = kts::Ref_A(p);
      cl_int b = kts::Ref_B(p);
      sum += (a * i) + b;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_A);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_B);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Task_05_04_SAXPY_Static_Trip_Uniform) {
  const cl_float A = 1.5f;
  kts::Reference1D<cl_float> refOut = [A](size_t x) {
    cl_int localID = static_cast<cl_int>(x % kts::localN);
    cl_float sum = 0.0f;
    for (cl_int i = 0; i < (cl_int)TRIPS; i++) {
      cl_int p = localID + i;
      cl_float X = kts::Ref_NegativeOffset(p);
      cl_float Y = kts::Ref_Float(p);
      sum += (A * X) + Y;
    }
    return sum;
  };

  AddMacro("TRIPS", TRIPS);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_NegativeOffset);
  AddInputBuffer(TRIPS + kts::localN, kts::Ref_Float);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(A);
  RunGeneric1D(kts::N, kts::localN);
}
