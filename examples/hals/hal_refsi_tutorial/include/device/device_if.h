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

#ifndef _REFSIDRV_DEVICE_IF_H
#define _REFSIDRV_DEVICE_IF_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

struct exec_state;

#if defined(BUILD_FOR_DEVICE)
struct wg_info;

typedef void *entry_point_fn;
typedef int (*direct_kernel_fn)(const void *args, struct exec_state *state);
typedef void (*ca_wrapped_kernel_fn)(const void *args, struct wg_info *wg);
typedef uint8_t *kernel_args_ptr;
#else
typedef uint64_t entry_point_fn;
typedef uint64_t direct_kernel_fn;
typedef uint64_t ca_wrapped_kernel_fn;
typedef uint64_t kernel_args_ptr;
#endif

typedef struct wg_info {
  size_t group_id[3];
  size_t num_groups[3];
  size_t global_offset[3];
  size_t local_size[3];
  uint32_t num_dim;
  size_t num_groups_per_call[3];
  uintptr_t hal_extra;
} wg_info_t;

#define DIMS 3

// Explicit alignment for fields. Used to have the same struct layout on
// 32-bit and 64-bit kernels.
#define ALIGN8 __attribute__((aligned(8)))
#define ALIGN4 __attribute__((aligned(4)))

typedef struct exec_state {
  ALIGN8 wg_info_t wg;
  ALIGN8 uint32_t local_id[DIMS];
  ALIGN8 entry_point_fn kernel_entry;
  ALIGN8 kernel_args_ptr packed_args;
  ALIGN8 uint32_t magic;
  ALIGN4 uint32_t state_size;
  ALIGN8 uint32_t flags;
  ALIGN4 uint32_t next_xfer_id;
  ALIGN8 uint32_t thread_id;
} exec_state_t;

#define REFSI_MAGIC ('R' | ('e' << 8) | ('S' << 16) | ('i' << 24))

// Launch the kernel directly, without scheduling or argument unpacking.
#define REFSI_FLAG_DIRECT_LAUNCH 0
// Specifies that the kernel has been 'wrapped' as part of the LLVM compilation
// process done by ComputeAorta.
#define REFSI_FLAG_CA_WRAPPED_KERNEL 1

// Needed to implement 'print'.
int vprintm(const char *s, va_list vl);

#endif  // _REFSIDRV_DEVICE_IF_H
