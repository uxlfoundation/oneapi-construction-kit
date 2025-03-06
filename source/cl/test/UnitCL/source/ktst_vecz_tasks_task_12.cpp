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

// this file contains tests related to the Interleaved Group Combine pass.
// The same kernels also exist as Lit tests to ensure the transform
// is actually applied where expected.

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

TEST_P(Execution, Task_12_01_Interleaved_Load_4) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};
  const cl_int Stride = 4;

  // it is just a bunch of "random" numbers
  int InBuffer[] = {54, 61, 29, 76, 56, 26, 75, 63,  //
                    29, 86, 57, 34, 37, 15, 91, 56,  //
                    51, 48, 19, 95, 20, 78, 73, 32,  //
                    75, 51, 8,  29, 56, 34, 85, 45};

  kts::Reference1D<cl_int> refIn = [=, &InBuffer](size_t x) -> int {
    return InBuffer[x];
  };

  kts::Reference1D<cl_int> refOut = [=, &InBuffer](size_t x) -> int {
    return InBuffer[(x * 2) + 1] - InBuffer[x * 2];
  };

  const size_t N = sizeof(InBuffer) / sizeof(cl_int);
  AddOutputBuffer(N / 2, refOut);
  AddInputBuffer(N, refIn);
  AddPrimitive(Stride);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Task_12_02_Interleaved_Load_5) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};
  const cl_int Stride = 4;

  // it is just a bunch of "random" numbers
  // there are two extra elements in the input buffer so
  // the kernel doesn't read off the end of it..
  int InBuffer[] = {54, 61, 29, 76, 56, 26, 75, 63,  //
                    29, 86, 57, 34, 37, 15, 91, 56,  //
                    51, 48, 19, 95, 20, 78, 73, 32,  //
                    75, 51, 8,  29, 56, 34, 85, 45,  //
                    33, 55};

  kts::Reference1D<cl_int> refIn = [=, &InBuffer](size_t x) -> int {
    return InBuffer[x];
  };

  kts::Reference1D<cl_int> refOut = [=, &InBuffer](size_t x) -> int {
    return InBuffer[x * 2] + InBuffer[(x * 2) + 1] + InBuffer[(x * 2) + 2] +
           InBuffer[(x * 2) + 3];
  };

  const size_t N = sizeof(InBuffer) / sizeof(cl_int);
  AddOutputBuffer((N - 2) / 2, refOut);
  AddInputBuffer(N, refIn);
  AddPrimitive(Stride);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Task_12_03_Interleaved_Load_6) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};
  const cl_int Stride = 4;

  // it is just a bunch of "random" numbers
  // there are two extra elements in the input buffer so
  // the kernel doesn't read off the end of it..
  int InBuffer[] = {54, 61, 29, 76, 56, 26, 75, 63,  //
                    29, 86, 57, 34, 37, 15, 91, 56,  //
                    51, 48, 19, 95, 20, 78, 73, 32,  //
                    75, 51, 8,  29, 56, 34, 85, 45,  //
                    33, 55};

  kts::Reference1D<cl_int> refIn = [=, &InBuffer](size_t x) -> int {
    return InBuffer[x];
  };

  kts::Reference1D<cl_int> refOut = [=, &InBuffer](size_t x) -> int {
    return (InBuffer[(x * 2) + 3] << 1) - InBuffer[(x * 2) + 2];
  };

  const size_t N = sizeof(InBuffer) / sizeof(cl_int);
  AddOutputBuffer((N - 2) / 2, refOut);
  AddInputBuffer(N, refIn);

  AddPrimitive(Stride);

  RunGenericND(2, global_range, local_range);
}
