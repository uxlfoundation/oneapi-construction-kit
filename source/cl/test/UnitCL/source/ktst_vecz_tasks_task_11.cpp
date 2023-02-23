// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This test contains tests for various vectorization failures

#include <string>
#include <utility>

#include "Common.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_11_01_Kernel_Signature) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_11_02_Kernel_Signature_NoInline_Before) {
  fail_if_not_vectorized_ = false;
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_11_03_Kernel_Signature_NoInline_After) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_11_04_Kernel_Signature_NoInline_Functions) {
  fail_if_not_vectorized_ = false;
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}
