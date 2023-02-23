// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_06_01_Copy_If_Constant) {
  // Test with the first constant that exercises one path.
  const cl_int C1 = 42;
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  AddPrimitive(C1);
  RunGeneric1D(kts::N);

  // Test with the second constant that exercises the other path.
  const cl_int C2 = 17;
  kts::Reference1D<cl_int> refOut2 = [](size_t) { return 0; };
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut2);
  AddPrimitive(C2);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_06_02_Copy_If_Even_Group) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    size_t gid = x / kts::localN;
    return ((gid & 1) == 0) ? kts::Ref_A(x) : -1;
  };
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}
