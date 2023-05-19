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

#ifndef _HAL_CPU_KERNEL_IF_H
#define _HAL_CPU_KERNEL_IF_H

#include <stdio.h>
#include <string.h>

#include "device_if.h"

typedef uint32_t uint;

#define __kernel

#define __global
#define __constant
#define __local
// This can only be used on variable declarations, whether at function level
// or in the global scope.
#define __local_variable static __attribute__((section(".local")))
#define __private

static inline uint32_t get_work_dim(exec_state_t *e) { return e->wg.num_dim; }

static inline uint32_t get_global_id(uint32_t rank, exec_state_t *e) {
  return (e->wg.group_id[rank] * e->wg.local_size[rank]) + e->local_id[rank] +
         e->wg.global_offset[rank];
}

static inline uint32_t get_local_id(uint32_t rank, exec_state_t *e) {
  return e->local_id[rank];
}

static inline uint32_t get_group_id(uint32_t rank, exec_state_t *e) {
  return e->wg.group_id[rank];
}

static inline uint32_t get_global_offset(uint32_t rank, exec_state_t *e) {
  return e->wg.global_offset[rank];
}

static inline uint32_t get_local_size(uint32_t rank, exec_state_t *e) {
  return e->wg.local_size[rank];
}

static inline uint32_t get_global_size(uint32_t rank, exec_state_t *e) {
  return e->wg.local_size[rank] * e->wg.num_groups[rank];
}

static inline int print(exec_state_t *e, const char *fmt, ...) {
  int ret = 0;
  va_list args;
  va_start(args, fmt);
  ret = vprintf(fmt, args);
  va_end(args);
  return ret;
}

static inline void barrier(struct exec_state *e) { e->barrier(e); }

static uintptr_t start_dma(void *dst, const void *src, size_t size_in_bytes,
                           struct exec_state *e) {
  static uint32_t xfer_id = 0;
  if ((e->local_id[0] == 0) && (e->local_id[1] == 0) && (e->local_id[2] == 0)) {
    // Only perform the copy operation on one work-item per work-group.
    memcpy(dst, src, size_in_bytes);
    xfer_id++;
  }
  return xfer_id;
}

static void wait_dma(uintptr_t xfer_id, struct exec_state *e) { /* No-op */
}

#endif  // _HAL_CPU_KERNEL_IF_H
