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

#ifndef _CLIK_CLIK_SYNC_API_H
#define _CLIK_CLIK_SYNC_API_H

#include "clik_common.h"

/////////////////// Device /////////////////////////////////////////////////////

struct clik_device;

// Create a new device object.
CLIK_API clik_device *clik_create_device();

// Free the resources used by the device object.
CLIK_API void clik_release_device(clik_device *device);

/////////////////// Buffers ////////////////////////////////////////////////////

struct clik_buffer;

// Create a buffer object with the given size, which lives in device memory.
CLIK_API clik_buffer *clik_create_buffer(clik_device *device, uint64_t size);

// Free the resources used by the buffer object.
CLIK_API void clik_release_buffer(clik_buffer *buffer);

/////////////////// Programs and kernels ///////////////////////////////////////

struct clik_program;

// Create a program object from an ELF binary. The program can contain one or
// more kernel functions.
CLIK_API clik_program *clik_create_program(clik_device *device,
                                           const void *binary_data,
                                           uint64_t binary_size);

// Free the resources used by the program object.
CLIK_API void clik_release_program(clik_program *program);

// Read the contents of a buffer back to host memory.
CLIK_API bool clik_read_buffer(clik_device *device, void *dst, clik_buffer *src,
                               uint64_t src_offset, uint64_t size);

// Write host data to device memory.
CLIK_API bool clik_write_buffer(clik_device *device, clik_buffer *dst,
                                uint64_t dst_offset, const void *src,
                                uint64_t size);

// Copy data from one buffer to another buffer.
CLIK_API bool clik_copy_buffer(clik_device *device, clik_buffer *dst,
                               uint64_t dst_offset, clik_buffer *src,
                               uint64_t src_offset, uint64_t size);

// Execute a kernel taken from the given program, with the specified N-D range
// and kernel arguments.
CLIK_API bool clik_run_kernel(clik_program *program, const char *name,
                              const clik_ndrange *nd_range,
                              const clik_argument *args, uint32_t num_args);

#endif  // _CLIK_CLIK_SYNC_API_H
