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

#include <string.h>

#include "async/clik_command_queue.h"
#include "async/clik_objects.h"
#include "clik_hal_version.h"
#include "hal.h"
#include "hal_library.h"

// Create a new device object.
clik_device *clik_create_device() {
  hal::hal_library_t library = nullptr;
  hal::hal_t *hal =
      hal::load_hal(CLIK_HAL_NAME, supported_hal_api_version, library);
  if (!hal) {
    return nullptr;
  } else if (hal->get_info().num_devices < 0) {
    hal::unload_hal(library);
    return nullptr;
  }
  hal::hal_device_t *hal_device = hal->device_create(0);
  if (!hal_device) {
    hal::unload_hal(library);
    return nullptr;
  }

  fprintf(stderr, "Using device '%s'\n", hal->get_info().platform_name);
  clik_device *device = new clik_device();
  {
    std::lock_guard<std::mutex> locker(device->lock);
    device->hal_device = hal_device;
    device->hal = hal;
    device->library = library;
  }
  clik_create_command_queue(device);
  return device;
}

// Free the resources used by the device object.
void clik_release_device(clik_device *device) {
  if (!device) {
    return;
  }

  // Release the command queue.
  clik_command_queue *queue = nullptr;
  {
    std::lock_guard<std::mutex> locker(device->lock);
    queue = device->queue;
  }
  clik_release_command_queue(queue);

  // Release the HAL.
  {
    std::lock_guard<std::mutex> locker(device->lock);
    device->hal->device_delete(device->hal_device);
    delete device;
  }
}

// Create a program object from an ELF binary. The program can contain one or
// more kernel functions.
clik_program *clik_create_program(clik_device *device, const void *binary_data,
                                  uint64_t binary_size) {
  if (!device) {
    return nullptr;
  }
  std::lock_guard<std::mutex> locker(device->lock);
  hal::hal_program_t elf =
      device->hal_device->program_load(binary_data, binary_size);
  if (elf == hal::hal_invalid_program) {
    return nullptr;
  }
  clik_program *program = new clik_program();
  program->device = device;
  program->elf = elf;
  return program;
}

// Free the resources used by the program object.
void clik_release_program(clik_program *program) {
  if (!program) {
    return;
  }
  std::lock_guard<std::mutex> locker(program->device->lock);
  program->device->hal_device->program_free(program->elf);
  delete program;
}

// Create a kernel object from a program object and the given arguments
// and N-D range dimensions.
clik_kernel *clik_create_kernel(clik_program *program, const char *name,
                                const clik_ndrange *nd_range,
                                const clik_argument *args, uint32_t num_args) {
  if (!program || !name) {
    return nullptr;
  }
  std::lock_guard<std::mutex> locker(program->device->lock);
  hal::hal_kernel_t function_addr =
      program->device->hal_device->program_find_kernel(program->elf, name);
  if (!function_addr) {
    return nullptr;
  }
  clik_kernel *kernel = new clik_kernel();
  kernel->program = program;
  kernel->function_addr = function_addr;
  memcpy(&kernel->nd_range, nd_range, sizeof(clik_ndrange));
  kernel->args = args;
  kernel->num_args = num_args;
  return kernel;
}

// Free the resources used by the kernel object.
void clik_release_kernel(clik_kernel *kernel) {
  if (!kernel) {
    return;
  }
  std::lock_guard<std::mutex> locker(kernel->program->device->lock);
  delete kernel;
}

// Create a buffer object with the given size, which lives in device memory.
clik_buffer *clik_create_buffer(clik_device *device, uint64_t size) {
  if (!device) {
    return nullptr;
  }
  std::lock_guard<std::mutex> locker(device->lock);
  const uint64_t buffer_alignment = 4096;
  clik_buffer *buffer = new clik_buffer();
  buffer->device = device;
  buffer->device_addr = device->hal_device->mem_alloc(size, buffer_alignment);
  if (buffer->device_addr == hal::hal_nullptr) {
    delete buffer;
    return nullptr;
  }
  buffer->size = size;
  return buffer;
}

// Free the resources used by the buffer object.
void clik_release_buffer(clik_buffer *buffer) {
  if (!buffer) {
    return;
  }
  std::lock_guard<std::mutex> locker(buffer->device->lock);
  buffer->device->hal_device->mem_free(buffer->device_addr);
  delete buffer;
}

// Initialize a kernel argument with a buffer value.
void clik_init_buffer_arg(clik_argument *arg, clik_buffer *buffer) {
  arg->type = clik_argument_type::buffer;
  arg->buffer = buffer;
  arg->size = 0;
  arg->contents = nullptr;
}

// Initialize a kernel argument with a scalar value.
void clik_init_scalar_arg(clik_argument *arg, const void *val, uint64_t size) {
  arg->type = clik_argument_type::byval;
  arg->buffer = nullptr;
  arg->size = size;
  arg->contents = (uint8_t *)val;
}

// Initialize a kernel argument with a local memory value.
void clik_init_local_memory_arg(clik_argument *arg, uint64_t size) {
  arg->type = clik_argument_type::local;
  arg->buffer = nullptr;
  arg->size = size;
  arg->contents = nullptr;
}

// Initialize a N-D range value with a 1-dimensional range.
void clik_init_ndrange_1d(clik_ndrange *ndrange, uint64_t global_size,
                          uint64_t local_size) {
  ndrange->dims = 1;
  for (uint32_t i = 0; i < clik_ndrange::max_dimensions; i++) {
    ndrange->offset[i] = 0;
    ndrange->local[i] = (i == 0) ? local_size : 1;
    ndrange->global[i] = (i == 0) ? global_size : 1;
  }
}

// Initialize a N-D range value with a 2-dimensional range.
void clik_init_ndrange_2d(clik_ndrange *ndrange, uint64_t global_x,
                          uint64_t global_y, uint64_t local_x,
                          uint64_t local_y) {
  ndrange->dims = 2;
  for (uint32_t i = 0; i < clik_ndrange::max_dimensions; i++) {
    ndrange->offset[i] = 0;
    switch (i) {
      default:
        ndrange->local[i] = 1;
        ndrange->global[i] = 1;
        break;
      case 0:
        ndrange->local[i] = local_x;
        ndrange->global[i] = global_x;
        break;
      case 1:
        ndrange->local[i] = local_y;
        ndrange->global[i] = global_y;
        break;
    }
  }
}

// Create a new command with the given type. The device's lock must be held
// while calling this function.
clik_command *clik_create_command(clik_command_queue *queue,
                                  clik_command_type type) {
  if (queue->shutdown) {
    return nullptr;
  }
  clik_command *cmd = new clik_command();
  cmd->timestamp = queue->next_command_timestamp;
  cmd->type = type;
  queue->next_command_timestamp++;
  return cmd;
}

// Free the resources used by the command object. The device's lock must be held
// while calling this function.
void clik_release_command(clik_command *cmd) {
  if (!cmd) {
    return;
  }
  delete cmd;
}
