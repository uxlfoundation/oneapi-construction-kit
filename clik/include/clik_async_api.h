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
//
// This header defines the 'high-level', or asynchronous, API of the clik
// compute library. It can be used to create buffers in device memory, transfer
// data between the host CPU and the device and enqueue parallel computation
// to the device in the form of binary kernels.

#ifndef _CLIK_CLIK_ASYNC_API_H
#define _CLIK_CLIK_ASYNC_API_H

#include <stdint.h>

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
struct clik_kernel;

// Create a program object from an ELF binary. The program can contain one or
// more kernel functions.
CLIK_API clik_program *clik_create_program(clik_device *device,
                                           const void *binary_data,
                                           uint64_t binary_size);

// Free the resources used by the program object.
CLIK_API void clik_release_program(clik_program *program);

// Create a kernel object from a program object and the given arguments
// and N-D range dimensions.
CLIK_API clik_kernel *clik_create_kernel(clik_program *program,
                                         const char *name,
                                         const clik_ndrange *nd_range,
                                         const clik_argument *args,
                                         uint32_t num_args);

// Free the resources used by the kernel object.
CLIK_API void clik_release_kernel(clik_kernel *kernel);

/////////////////// Command queue //////////////////////////////////////////////

struct clik_command_queue;

// Get the command queue exposed by the device.
CLIK_API clik_command_queue *clik_get_device_queue(clik_device *device);

// Enqueue a command to read the contents of a buffer back to host memory.
CLIK_API bool clik_enqueue_read_buffer(clik_command_queue *queue, void *dst,
                                       clik_buffer *src, uint64_t src_offset,
                                       uint64_t size);

// Enqueue a command to write host data to device memory.
CLIK_API bool clik_enqueue_write_buffer(clik_command_queue *queue,
                                        clik_buffer *dst, uint64_t dst_offset,
                                        const void *src, uint64_t size);

// Enqueue a command to copy data from one buffer to another buffer.
CLIK_API bool clik_enqueue_copy_buffer(clik_command_queue *queue,
                                       clik_buffer *dst, uint64_t dst_offset,
                                       clik_buffer *src, uint64_t src_offset,
                                       uint64_t size);

// Enqueue a command to execute a kernel on the device.
CLIK_API bool clik_enqueue_kernel(clik_command_queue *queue,
                                  clik_kernel *kernel);

// Start executing enqueued commands on the device.
//
// Returns true if any commands have been dispatched by this call.
CLIK_API bool clik_dispatch(clik_command_queue *queue);

// Wait until enqueued commands have finished executing on the device.
//
// clik_dispatch must have been called previously or this function will return
// without waiting.
CLIK_API bool clik_wait(clik_command_queue *queue);

#endif  // _CLIK_CLIK_ASYNC_API_H
