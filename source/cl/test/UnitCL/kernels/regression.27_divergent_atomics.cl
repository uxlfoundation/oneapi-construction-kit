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

kernel void divergent_atomics(global unsigned *flag) {
  // Test that atomics inside conditions are correctly instantiated when the
  // kernel is vectorized. The `do` body increments an even flag value to an odd
  // flag value, and the `while` condition increments an odd flag value to an
  // even flag value. This ensures that `atomic_cmpxchg()` is executed once per
  // `do` body and once per `while` condition regardless of the number of work
  // items and regardless of vectoriztion. The order in which work items
  // encounter the atomics is undefined. Without the `atomic_cmpxchg()` inside
  // the `while` body, the `atomic_cmpxchg()` in the `while` condition could be
  // executed several times in one loop, causing some work items to exit the
  // loop early. This would cause the remaining work items to hang at the
  // barrier. (This hang was present in a previous version of this kernel.)
  const unsigned new_even = get_global_id(0) * 2u;
  const unsigned new_odd = new_even + 1u;
  const unsigned double_size = get_global_size(0) * 2u;
  do {
    barrier(CLK_GLOBAL_MEM_FENCE);
    atomic_cmpxchg(flag, new_even, new_even + 1);
    barrier(CLK_GLOBAL_MEM_FENCE);
  } while (double_size != atomic_cmpxchg(flag, new_odd, new_odd + 1));
}
