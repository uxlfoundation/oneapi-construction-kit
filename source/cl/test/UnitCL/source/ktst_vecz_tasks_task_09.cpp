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

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include "Common.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_09_01_Masked_Interleaved_Store) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    if (x != 24) {
      return 0;
    } else {
      return kts::Ref_A(12);
    }
  };
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N * 2, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_02_Masked_Interleaved_Load) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    if (x != 12) {
      return 0;
    } else {
      return kts::Ref_A(24);
    }
  };
  AddInputBuffer(kts::N * 2, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_03_Masked_Scatter) {
  // Generate the indices from 0 to N-1
  std::vector<cl_int> Indices(kts::N);
  std::iota(Indices.begin(), Indices.end(), 0);
  // We want a random permutation, but we also want to keep it consistent
  // between runs, so we provide a constant as the seed.
  std::default_random_engine e(1);
  std::shuffle(Indices.begin(), Indices.end(), e);

  kts::Reference1D<cl_int> outIndices = [&Indices](size_t x) {
    return Indices[x];
  };
  kts::Reference1D<cl_int> refOut = [&Indices](size_t x) -> cl_int {
    const cl_int Index = static_cast<cl_int>(
        std::find(Indices.begin(), Indices.end(), x) - Indices.begin());
    return Index % 3 == 0 ? 42 : kts::Ref_A(Index);
  };

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  AddInputBuffer(kts::N, outIndices);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_04_Masked_Gather) {
  // Generate the indices from 0 to N-1
  std::vector<cl_int> Indices(kts::N);
  std::iota(Indices.begin(), Indices.end(), 0);
  // We want a random permutation, but we also want to keep it consistent
  // between runs, so we provide a constant as the seed.
  std::default_random_engine e(1);
  std::shuffle(Indices.begin(), Indices.end(), e);

  kts::Reference1D<cl_int> inIndices = [&Indices](size_t x) {
    return Indices[x];
  };
  kts::Reference1D<cl_int> refOut = [&inIndices](size_t x) {
    return x % 3 != 0 ? kts::Ref_A(inIndices(x)) : 42;
  };

  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, inIndices);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_05_Masked_Argument_Stride) {
  static constexpr cl_int Stride = 3;
  static constexpr cl_int Max = 1 << 30;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x % Max);
  };
  kts::Reference1D<cl_int> refOut = [](size_t x) -> cl_int {
    if (x == 0 || x == 1 || x == 2) {
      return 13;
    } else {
      return x % Stride == 0 ? x % Max : 1;
    }
  };

  AddInputBuffer(kts::N * Stride, refIn);
  AddOutputBuffer(kts::N * Stride, refOut);
  AddPrimitive(Stride);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_06_Masked_Negative_Stride) {
  const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x * x);
  };
  kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) -> cl_int {
    if (x == 0) {
      return 13;
    } else {
      return refIn(MaxIndex - x) + refIn(x);
    }
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(MaxIndex);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_07_Masked_Negative_Argument_Stride) {
  const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x * x);
  };
  kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) -> cl_int {
    if (x == 0) {
      return 13;
    } else {
      return refIn(MaxIndex - x) + refIn(x);
    }
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(-1);
  AddPrimitive(MaxIndex);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_09_08_Phi_Memory) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  AddPrimitive(16);
  RunGeneric1D((kts::N)-15);
}

TEST_P(Execution, Task_09_08_Phi_Memory2) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  AddPrimitive(16);
  RunGeneric1D((kts::N)-15);
}

TEST_P(Execution, Task_09_09_Masked_Vector_Load) {
  const cl_uint mask = 7;
  // Clang complains mask does not need to be captured,
  // MSVC fails saying that it does,
  // that's why i put [=], to shut them both up.
  kts::Reference1D<cl_int> refOut = [=](size_t x) {
    if (x & mask) {
      return 1;
    } else {
      return kts::Ref_A(x);
    }
  };

  // We need 7 extra input elements because the kernel accesses a cl_int8
  // through a cl_int* base pointer, although because of the mask these are not
  // actually accessed.
  AddInputBuffer(kts::N + 7, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(mask);
  RunGeneric1D(kts::N);
}
