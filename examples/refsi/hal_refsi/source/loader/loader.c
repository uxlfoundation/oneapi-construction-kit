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

#include "loader.h"
#include "machine.h"
#include "io.h"
#include "device/memory_map.h"

#include <stdbool.h>

// Execute a kernel entry point function that follows the 'work-item-per-thread'
// scheduling mode. Such entry points call the kernel function once per
// work-group, on each hardware thread.
int execute_wi_per_thread_kernel(exec_state_t *exec) {
  wi_kernel_fn kernel = (wi_kernel_fn)exec->kernel_entry;
  kernel(exec->packed_args, exec);
  return 0;
}

// Execute a kernel entry point function that follows the
// 'work-group-per-thread' scheduling mode, looping over all work-items in a
// work-group. Such kernels do not perform looping over work-groups. They have
// to be called once for each work-group in the N-D range.
int execute_wg_per_thread_kernel(exec_state_t *exec) {
  wg_info_t *wg = &exec->wg;
  const size_t ngx = wg->num_groups[0];
  const size_t ngy = wg->num_groups[1];
  const size_t ngz = wg->num_groups[2];

  // total number of groups to execute
  const size_t num_groups = ngx * ngy * ngz;
  // evenly divisable number of groups per core
  const size_t num_groups_per_hart = num_groups / NUM_HARTS_FOR_CA_MODE;
  // the starting group and end group
  const size_t group_begin = num_groups_per_hart * exec->thread_id;
  const size_t group_end = group_begin + num_groups_per_hart;
  // num_groups rounded down to a multiple of NUM_HARTS_FOR_CA_MODE.
  const size_t divisable_groups = num_groups_per_hart * NUM_HARTS_FOR_CA_MODE;
  // number of groups that were not evenly divisable
  const size_t remain = num_groups - divisable_groups;
  // calculate if we must execute a tail group and its id
  const bool will_tail = exec->thread_id < remain;
  const size_t tail_id = divisable_groups + exec->thread_id;

  wg_kernel_fn kernel = (wg_kernel_fn)exec->kernel_entry;
  for (size_t i = group_begin; i < group_end; ++i) {
    wg->group_id[0] = i % ngx;
    wg->group_id[1] = (i / ngx) % ngy;
    wg->group_id[2] = (i / (ngx * ngy)) % ngz;
    kernel(exec->packed_args, wg);
  }

  if (will_tail) {
    wg->group_id[0] = tail_id % ngx;
    wg->group_id[1] = (tail_id / ngx) % ngy;
    wg->group_id[2] = (tail_id / (ngx * ngy)) % ngz;
    kernel(exec->packed_args, wg);
  }
  return 0;
}

void execute_nd_range() {
  int exit_code = -1;
  exec_state_t *exec = get_current_context();
  if (exec) {
    int mode = REFSI_FLAG_GET_THREAD_MODE(exec->flags);
    switch (mode) {
    default:
      break;
    case REFSI_THREAD_MODE_WI:
      exit_code = execute_wi_per_thread_kernel(exec);
      break;
    case REFSI_THREAD_MODE_WG:
      exit_code = execute_wg_per_thread_kernel(exec);
      break;
    }
  }
  shutdown(exit_code);
}

exec_state_t * get_current_context() {
  exec_state_t *exec = (exec_state_t *)REFSI_CONTEXT_ADDRESS;
  if ((exec->state_size != sizeof(exec_state_t)) ||
      (exec->magic != REFSI_MAGIC)) {
    printm("error: The kernel execution state header (at %p) is corrupted. "
           "size: %d, expected: %d, magic: %x\n", exec, exec->state_size,
           sizeof(exec_state_t), exec->magic);
    return NULL;
  }
  return exec;
}
