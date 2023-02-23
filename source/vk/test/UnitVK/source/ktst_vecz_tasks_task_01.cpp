// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Task_01_01_Copy) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_02_Add) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddInputBuffer(kts::N, kts::Ref_B);
    AddOutputBuffer(kts::N, kts::Ref_Add);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_03_Mul_FMA) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_PlusOne);
    AddInputBuffer(kts::N, kts::Ref_MinusOne);
    AddInputBuffer(kts::N, kts::Ref_Triple);
    AddOutputBuffer(kts::N, kts::Ref_Mul);
    AddOutputBuffer(kts::N, kts::Ref_FMA);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_04_Ternary) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_Odd);
    AddPrimitive(1);
    AddPrimitive(-1);
    AddOutputBuffer(kts::N, kts::Ref_Ternary);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_05_Broadcast) {
  if (clspvSupported_) {
    AddOutputBuffer(kts::N, kts::Ref_Identity);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_01_06_Broadcast_Uniform) {
  if (clspvSupported_) {
    cl_int foo = 41;
    kts::Reference1D<cl_int> refOut = [&foo](size_t) { return foo + 1; };
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(foo);
    RunGeneric1D(kts::N);
  }
}
