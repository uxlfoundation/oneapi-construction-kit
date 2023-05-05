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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "core/core.h"
#include "mux_util.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_matrix_multiply.c' and turning it into a C array using the Bin2H
// tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  uint64_t local_size = 16;
  uint64_t matrix_size = 32;
  option_parser_t parser;
  parser.help([]() {
    fprintf(
        stderr,
        "Usage: ./matrix_multiply_mux [--local-size L] [--matrix-size M]\n");
  });
  parser.option('L', "local-size", 1,
                [&](const char *s) { local_size = strtoull(s, 0, 0); });
  parser.option('M', "matrix-size", 1,
                [&](const char *s) { matrix_size = strtoull(s, 0, 0); });
  parser.parse(argv);
  if (local_size < 1) {
    fprintf(stderr, "error: local size must be positive\n");
    return 7;
  } else if (matrix_size < 1) {
    fprintf(stderr, "error: matrix size must be positive\n");
    return 7;
  } else if ((matrix_size % local_size) != 0) {
    fprintf(stderr,
            "error: matrix size (%zd) must be a multiple of local size"
            "(%zd)\n",
            matrix_size, local_size);
    return 7;
  }

  core_allocator_info_t allocator{ExampleAlloc, ExampleFree, nullptr};

  // Find a device to execute kernels.
  core_device_t device = createDevice(core_device_type_accelerator, allocator);
  if (!device) {
    fprintf(stderr, "Could not find any 'accelerator' device.\n");
    return 1;
  }
  fprintf(stderr, "Using device: %s\n", device->info->device_name);
  core_queue_t queue = nullptr;
  coreGetQueue(device, core_queue_type_compute, 0, &queue);
  if (!queue) {
    fprintf(stderr, "The device does not have any compute queue.\n");
    return 1;
  }

  // Load the kernel program.
  const char *kernel_entry_function = "kernel_main";
  core_callback_info_t callback;
  core_finalizer_t finalizer = nullptr;
  core_executable_t executable = nullptr;
  coreCreateFinalizer(device->info, core_source_type_binary, nullptr, 0,
                      callback, allocator, &finalizer);
  coreCreateExecutable(device, finalizer, core_source_type_binary, {},
                       matrix_multiply_mux_kernel_binary,
                       matrix_multiply_mux_kernel_binary_size, allocator,
                       &executable);
  if (!executable) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Initialize host data.
  uint32_t M = matrix_size;
  size_t num_elements = M * M;
  std::vector<float> a_data(num_elements);
  std::vector<float> b_data(num_elements);
  std::vector<float> c_data(num_elements);
  for (size_t row = 0; row < M; row++) {
    for (size_t col = 0; col < M; col++) {
      a_data[(row * M) + col] = 2.0;
      b_data[(row * M) + col] = col;
      c_data[(row * M) + col] = 0.0f;
    }
  }

  // Compute the expected result.
  float exp_data[num_elements];
  for (size_t row = 0; row < M; row++) {
    for (size_t col = 0; col < M; col++) {
      float sum = 0.0f;
      for (size_t i = 0; i < M; i++) {
        sum += a_data[(row * M) + i] * b_data[(i * M) + col];
      }
      exp_data[(row * M) + col] = sum;
    }
  }

  // Create buffers in device memory.
  uint64_t buffer_size = num_elements * sizeof(float);
  core_buffer_t a_buffer = nullptr;
  core_buffer_t b_buffer = nullptr;
  core_buffer_t c_buffer = nullptr;
  core_memory_t memory = nullptr;
  coreCreateBuffer(device, buffer_size, allocator, &a_buffer);
  coreCreateBuffer(device, buffer_size, allocator, &b_buffer);
  coreCreateBuffer(device, buffer_size, allocator, &c_buffer);
  if (!a_buffer || !b_buffer || !c_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }
  uint32_t heap =
      findFirstSupportedHeap(c_buffer->memory_requirements.supported_heaps);
  coreAllocateMemory(device, buffer_size * 3, heap,
                     core_memory_property_device_local,
                     core_allocation_type_alloc_device, 0, allocator, &memory);
  if (!memory) {
    fprintf(stderr, "Could not allocate device memory to store buffers in.\n");
    return 3;
  }
  coreBindBufferMemory(device, memory, a_buffer, buffer_size * 0);
  coreBindBufferMemory(device, memory, b_buffer, buffer_size * 1);
  coreBindBufferMemory(device, memory, c_buffer, buffer_size * 2);

  // Write host data to device memory.
  core_semaphore_t semaphore_write = nullptr;
  core_command_group_t command_write = nullptr;
  coreCreateSemaphore(device, allocator, &semaphore_write);
  coreCreateCommandGroup(device, callback, allocator, &command_write);
  corePushWriteBuffer(command_write, a_buffer, 0, &a_data[0], buffer_size);
  corePushWriteBuffer(command_write, b_buffer, 0, &b_data[0], buffer_size);

  // Run the kernel.
  core_kernel_t kernel = nullptr;
  core_scheduled_kernel_t scheduled_kernel = nullptr;
  core_specialized_kernel_t specialized_kernel = nullptr;
  core_command_group_t command_work = nullptr;
  core_semaphore_t semaphore_work = nullptr;
  size_t global_size[2] = {M, M};
  size_t global_offset[2] = {0, 0};
  const size_t num_args = 4;
  core_descriptor_info_t descriptors[num_args];
  coreCreateKernel(device, finalizer, executable, kernel_entry_function,
                   strlen(kernel_entry_function), allocator, &kernel);
  if (!kernel) {
    fprintf(stderr, "Unable to create a kernel.\n");
    return 5;
  }
  coreCreateScheduledKernel(device, finalizer, kernel, local_size, 1, 1,
                            allocator, &scheduled_kernel);
  descriptors[0].type = core_descriptor_info_type_buffer;
  descriptors[0].buffer_descriptor.buffer = a_buffer;
  descriptors[0].buffer_descriptor.offset = 0;
  descriptors[1].type = core_descriptor_info_type_buffer;
  descriptors[1].buffer_descriptor.buffer = b_buffer;
  descriptors[1].buffer_descriptor.offset = 0;
  descriptors[2].type = core_descriptor_info_type_buffer;
  descriptors[2].buffer_descriptor.buffer = c_buffer;
  descriptors[2].buffer_descriptor.offset = 0;
  descriptors[3].type = core_descriptor_info_type_plain_old_data;
  descriptors[3].plain_old_data_descriptor.data = &M;
  descriptors[3].plain_old_data_descriptor.length = sizeof(M);
  coreCreateSpecializedKernel(device, finalizer, scheduled_kernel, descriptors,
                              num_args, global_offset, global_size, 2,
                              allocator, &specialized_kernel);
  coreCreateSemaphore(device, allocator, &semaphore_work);
  coreCreateCommandGroup(device, callback, allocator, &command_work);
  corePushNDRange(command_work, specialized_kernel);

  // Read the data produced by the kernel.
  core_command_group_t command_read = nullptr;
  coreCreateCommandGroup(device, callback, allocator, &command_read);
  corePushReadBuffer(command_read, c_buffer, 0, &c_data[0], buffer_size);

  // Start executing commands on the device.
  printf(
      "Running matrix_multiply_mux example (Global size: %zux%zu, "
      "local size: %zux%zu)\n",
      global_size[0], global_size[1], local_size, 1ull);
  coreDispatch(queue, command_write, nullptr, 0, &semaphore_write, 1, nullptr,
               nullptr);
  coreDispatch(queue, command_work, &semaphore_write, 1, &semaphore_work, 1,
               nullptr, nullptr);
  coreDispatch(queue, command_read, &semaphore_work, 1, nullptr, 0, nullptr,
               nullptr);

  // Wait for all commands to have finished executing on the device.
  coreWaitAll(queue);

  // Validate output buffer.
  bool validated = true;
  size_t errors = 0;
  const size_t max_errors_to_report = 10;
  const float e = 1e-6f;
  for (size_t row = 0; row < M; row++) {
    for (size_t col = 0; col < M; col++) {
      float expected = exp_data[(row * M) + col];
      float actual = c_data[(row * M) + col];
      if (fabsf(actual - expected) > e) {
        errors++;
        if (errors <= max_errors_to_report) {
          fprintf(stderr,
                  "Result mismatch at (%zu, %zu): expected %f (%a), "
                  "but got %f (%a)\n",
                  row, col, expected, expected, actual, actual);
        }
        validated = false;
      }
    }
  }
  if (validated) {
    fprintf(stderr, "Results validated successfully.\n");
  }

  // Clean up
  coreDestroyCommandGroup(device, command_write, allocator);
  coreDestroyCommandGroup(device, command_work, allocator);
  coreDestroyCommandGroup(device, command_read, allocator);
  coreDestroySemaphore(device, semaphore_write, allocator);
  coreDestroySemaphore(device, semaphore_work, allocator);
  coreDestroySpecializedKernel(device, specialized_kernel, allocator);
  coreDestroyBuffer(device, a_buffer, allocator);
  coreDestroyBuffer(device, b_buffer, allocator);
  coreDestroyBuffer(device, c_buffer, allocator);
  coreFreeMemory(device, memory, allocator);
  coreDestroyScheduledKernel(device, finalizer, scheduled_kernel, allocator);
  coreDestroyKernel(device, finalizer, kernel, allocator);
  coreDestroyExecutable(device, finalizer, executable, allocator);
  coreDestroyFinalizer(device->info, finalizer, allocator);
  coreDestroyDevice(device, allocator);
  return validated ? 0 : -1;
}
