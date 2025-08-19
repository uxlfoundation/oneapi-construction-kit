// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// This kernel is meant to test scalarization and vectorization of FNeg.
__kernel void balance(const float src, __global float4 *dst) {

  size_t x = get_global_id(0);

  const size_t size_x = get_local_size(0);

  float4 value = dst[x];

  // operator2 should generate a fneg operation. Furthermore, make operator1
  // and operator2 not trivially optimizable by frontend by adding dependency
  // on get_global_id
  float operator1 = src + (float)x;
  float operator2 = (- src) - (float)x;

  if (x <= size_x / 2) {
    value = 1.0f - value;
  }

  // should generate two fneg operations
  if (x % 2) {
    operator1 = - operator1;
    operator2 = - operator2;
  }

  dst[x] = value * (operator1 + operator2);
}
