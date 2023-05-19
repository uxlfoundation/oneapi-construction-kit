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

#ifndef _CLIK_RUNTIME_CLIK_OBJECTS_H
#define _CLIK_RUNTIME_CLIK_OBJECTS_H

#include <mutex>
#include <string>

#include "clik_async_api.h"

struct clik_command_queue;
class elf_program;

namespace hal {
using hal_program_t = uint64_t;
using hal_kernel_t = uint64_t;
using hal_library_t = void *;
struct hal_t;
struct hal_device_t;
}  // namespace hal

// Contains state required for a device object.
struct clik_device {
  // Low-level interface to the device.
  hal::hal_device_t *hal_device;
  // HAL object used to create and delete HAL devices.
  hal::hal_t *hal;
  // Handle to the HAL library.
  hal::hal_library_t library;
  // Command queue used to process commands on the device.
  clik_command_queue *queue;
  // Global lock used to protect any clik state from other threads.
  std::mutex lock;
};

// Contains state required for a program object.
struct clik_program {
  // Refers to the device object this program was created for.
  clik_device *device;
  // ELF program read from a binary.
  hal::hal_program_t elf;
};

// Contains state required for a kernel object.
struct clik_kernel {
  // Refers to the program object this kernel was created from.
  clik_program *program;
  // Address of the entry point function for the kernel, in device memory.
  hal::hal_kernel_t function_addr;
  // Dimensions of work to be done by the kernel.
  clik_ndrange nd_range;
  // Array of arguments to pass to the kernel.
  const clik_argument *args;
  // Number of arguments to pass to the kernel.
  uint32_t num_args;
};

// Contains state required for a buffer object.
struct clik_buffer {
  // Refers to the device object this buffer was created for.
  clik_device *device;
  // Address of the memory allocated for the buffer, in device memory.
  uint64_t device_addr;
  // Size of the buffer, in bytes.
  uint64_t size;
};

// Identifies commands in a command queue.
enum class clik_command_type {
  // Not a valid command.
  invalid = 0,
  // Command that copies data from device memory back to the host.
  read_buffer = 1,
  // Command that copies data from the host to device memory.
  write_buffer = 2,
  // Command that copies data from one device buffer to another.
  copy_buffer = 3,
  // Command that executes a kernel on the device.
  run_kernel = 4
};

// Contains arguments to a 'read buffer' command.
struct read_buffer_args {
  clik_buffer *buffer;
  uint64_t offset;
  uint64_t size;
  void *dst;
};

// Contains arguments to a 'write buffer' command.
struct write_buffer_args {
  clik_buffer *buffer;
  uint64_t offset;
  uint64_t size;
  const void *src;
};

// Contains arguments to a 'copy buffer' command.
struct copy_buffer_args {
  clik_buffer *dst_buffer;
  uint64_t dst_offset;
  clik_buffer *src_buffer;
  uint64_t src_offset;
  uint64_t size;
};

// Contains arguments to a 'run kernel' command.
struct run_kernel_args {
  clik_kernel *kernel;
};

// Represents work to be executed on the device.
struct clik_command {
  // Represents a point in time for this command. Commands enqueued earlier have
  // a strictly smaller timestamp than this command while commands enqueued
  // later will have a strictly larger timestamp.
  uint64_t timestamp;
  // Type of command to be executed.
  clik_command_type type;
  union {
    read_buffer_args read_buffer;
    write_buffer_args write_buffer;
    run_kernel_args run_kernel;
    copy_buffer_args copy_buffer;
  };
};

// Create a new command with the given type. The device's lock must be held
// while calling this function.
clik_command *clik_create_command(clik_command_queue *queue,
                                  clik_command_type type);

// Free the resources used by the command object. The device's lock must be held
// while calling this function.
void clik_release_command(clik_command *cmd);

#endif
