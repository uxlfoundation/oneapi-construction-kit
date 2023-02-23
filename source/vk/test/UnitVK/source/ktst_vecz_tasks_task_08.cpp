// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Task_08_01_User_Fn_Identity) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, kts::Ref_A);
    AddOutputBuffer(kts::N, kts::Ref_A);
    RunGeneric1D(kts::N, kts::localN);
  }
}

TEST_F(Execution, Task_08_02_User_Fn_SExt) {
  if (clspvSupported_) {
    kts::Reference1D<short> refIn = [](size_t x) { return (short)(x * 2); };
    kts::Reference1D<cl_int> refOut = [&refIn](size_t x) { return -refIn(x); };
    AddOutputBuffer(kts::N, refOut);
    AddInputBuffer(kts::N, refIn);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Task_08_03_User_Fn_Two_Contexts) {
  if (clspvSupported_) {
    const cl_int alpha = 17;
    auto foo = [](cl_int x, cl_int y) { return x * (y - 1); };
    kts::Reference1D<cl_int> refOut = [=, &foo](size_t x) {
      cl_int src1 = kts::Ref_A(x);
      cl_int src2 = kts::Ref_B(x);
      cl_int res1 = foo(src1, src2);
      cl_int res2 = foo(alpha, src2);
      return res1 + res2;
    };
    AddOutputBuffer(kts::N, refOut);
    AddInputBuffer(kts::N, kts::Ref_A);
    AddInputBuffer(kts::N, kts::Ref_B);
    AddPrimitive(alpha);
    RunGeneric1D(kts::N);
  }
}
