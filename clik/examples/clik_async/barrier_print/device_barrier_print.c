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

#include "device_barrier_print.h"

// In this example, the kernel has two computation steps represented by printf
// calls. A barrier is used to ensure step A has been performed by all
// work-items in the work-group before any work-item in the group starts step B.
//
// With a work-group size of four, the output of this example will look like:
//
// Kernel part A (tid = 0)
// Kernel part A (tid = 1)
// Kernel part A (tid = 2)
// Kernel part A (tid = 3)
// Kernel part B (tid = 0)
// Kernel part B (tid = 1)
// Kernel part B (tid = 2)
// Kernel part B (tid = 3)
//
// Without a barrier between the two steps, the output would look like:
//
// Kernel part A (tid = 0)
// Kernel part B (tid = 0)
// Kernel part A (tid = 1)
// Kernel part B (tid = 1)
// Kernel part A (tid = 2)
// Kernel part B (tid = 2)
// Kernel part A (tid = 3)
// Kernel part B (tid = 3)

__kernel void barrier_print(exec_state_t *ctx) {
  uint tid = get_global_id(0, ctx);
  print(ctx, "Kernel part A (tid = %d)\n", tid);
  barrier(ctx);
  print(ctx, "Kernel part B (tid = %d)\n", tid);
}
