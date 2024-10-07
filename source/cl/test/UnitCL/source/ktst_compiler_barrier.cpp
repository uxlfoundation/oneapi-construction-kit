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

#include <Common.h>
#include <kts/execution.h>
#include <kts/reference_functions.h>

using namespace kts::ucl;

TEST_P(Execution, Compiler_Barrier_01_Only_Barrier) { RunGeneric1D(kts::N); }

TEST_P(Execution, Compiler_Barrier_02_Group_Divergent_Barriers) {
  const unsigned offset = 5;
  // If `ARRAY_SIZE` changes from `16`, recompile Offline kernels.
  const size_t ARRAY_SIZE = kts::N / kts::localN;
  ASSERT_EQ(ARRAY_SIZE, 16);
  kts::Reference1D<cl_int> refIn = [&](size_t x) {
    return (x == offset) ? 42 : 0;
  };
  kts::Reference1D<cl_int> refOut = [&](size_t x) {
    const cl_int lid = static_cast<cl_int>(x % kts::localN);
    const size_t gid = static_cast<cl_int>(x / kts::localN);
    return (gid == offset) ? lid : lid + 1;
  };

  AddMacro("ARRAY_SIZE", (cl_int)ARRAY_SIZE);
  AddInputBuffer(ARRAY_SIZE, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

// Pick local work group sizes that are not a power of two so that we can test
// the vectorizer and barrier interaction in this case.
using ExecutionWG = kts::ucl::ExecutionWithParam<size_t>;

TEST_P(ExecutionWG, Compiler_Barrier_03_Odd_Work_Group_Size) {
  // Whether or not the kernel will be vectorized at a local size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Get the parameter, but guard against it being zero (kts::localN - 1,
  // when kts::localN is changed to 1).
  const size_t local = getParam() > 0 ? getParam() : 1;
  const size_t size = (kts::N / kts::localN) * local;

  kts::Reference1D<cl_int> refOut = [&](size_t x) {
    if ((x % local) == 0) {
      if (local > 1) {
        return kts::Ref_Identity(x) + kts::Ref_Identity(x + 1);
      } else {
        return kts::Ref_Identity(x);
      }
    }

    return kts::Ref_Identity(x - 1) + kts::Ref_Identity(x);
  };

  AddMacro("ARRAY_SIZE", (unsigned)local);
  AddInputBuffer(size, kts::Ref_Identity);
  AddOutputBuffer(size, refOut);
  RunGeneric1D(size, local);
}

// Pick various local work group sizes so that we can test vectorizer, barrier,
// local-work-item interaction.
UCL_EXECUTION_TEST_SUITE_P(
    ExecutionWG, testing::Values(OPENCL_C),
    testing::Values(
        // Just use the default as a sanity check.
        kts::localN,
        // Use kts::localN-1 as a non-power of 2 check.
        kts::localN - 1,
        // Test all integers 1-17, some are powers of 2.  Includes 3 which
        // is not a power of 2 but still something that we can vectorize by.
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        // Test a bunch of powers of 2, it is very likely that some of these
        // will be skipped due to the device not supporting large
        // dimensions.
        32, 64, 128, 256, 512, 1024, 2048, 4096,
        // Test a power of numbers that are not powers of two, but are
        // divisible by powers of two (other than 1), i.e. vectorizable.
        20, 40, 60, 120, 240, 480))

TEST_P(Execution, Compiler_Barrier_04_Mutually_Exclusive_Barriers) {
  const size_t offset = 5;
  // If `ARRAY_SIZE` changes from `16`, recompile Offline kernels.
  const size_t ARRAY_SIZE = kts::N / kts::localN;
  ASSERT_EQ(ARRAY_SIZE, 16);
  kts::Reference1D<cl_int> refIn = [&](size_t x) {
    return (x == offset) ? 42 : 0;
  };
  kts::Reference1D<cl_int> refOut = [&](size_t x) {
    const cl_int lid = static_cast<cl_int>(x) % kts::localN;
    const size_t group = x / kts::localN;
    return (group == offset) ? lid * 3 : (lid * 3) + 1;
  };

  AddMacro("ARRAY_SIZE", (int)ARRAY_SIZE);
  AddInputBuffer(ARRAY_SIZE, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Compiler_Barrier_05_Simple_Mutually_Exclusive_Barriers) {
  const size_t offset = 5;
  const size_t ARRAY_SIZE = kts::N / kts::localN;
  kts::Reference1D<cl_int> refIn = [&](size_t x) {
    return (x == offset) ? 42 : 0;
  };
  kts::Reference1D<cl_int> refOut = [&](size_t x) {
    const cl_int lid = static_cast<cl_int>(x) % kts::localN;
    const size_t group = x / kts::localN;
    return (group == offset) ? lid * 3 : (lid * 3) + 1;
  };
  AddInputBuffer(ARRAY_SIZE, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}
