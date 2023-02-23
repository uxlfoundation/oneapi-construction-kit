// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_DEVICE_IF_H
#define _REFSIDRV_DEVICE_IF_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

struct exec_state;

#if defined(BUILD_FOR_DEVICE)
struct wg_info;

typedef void *entry_point_fn;
typedef int (*wi_kernel_fn)(const void *args, struct exec_state *state);
typedef void (*wg_kernel_fn)(const void *args, struct wg_info *wg);
typedef uint8_t* kernel_args_ptr;
#else
typedef uint64_t entry_point_fn;
typedef uint64_t wi_kernel_fn;
typedef uint64_t wg_kernel_fn;
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

// Retrieve the thread mode from RefSi flags.
#define REFSI_FLAG_GET_THREAD_MODE(x) ((x) & 0x1)

// Launch the kernel using the work-item-per-thread mode.
#define REFSI_THREAD_MODE_WI            0
// Launch the kernel using the work-group-per-thread mode.
#define REFSI_THREAD_MODE_WG            1

// Needed to implement 'print'.
int vprintm(const char* s, va_list vl);

#endif // _REFSIDRV_DEVICE_IF_H
