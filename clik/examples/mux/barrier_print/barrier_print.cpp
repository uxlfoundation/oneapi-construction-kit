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

#include <stdio.h>
#include <string.h>

#include <vector>

#include "core/core.h"
#include "mux_util.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_barrier_print.c' and turning it into a C array using the Bin2H tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  size_t local_size = 4;
  size_t global_size = 4;
  option_parser_t parser;
  parser.help([]() {
    fprintf(stderr,
            "Usage: ./barrier_print_mux [--local-size L] [--global-size S]\n");
  });
  parser.option('L', "local-size", 1,
                [&](const char *s) { local_size = strtoull(s, 0, 0); });
  parser.option('S', "global-size", 1,
                [&](const char *s) { global_size = strtoull(s, 0, 0); });
  parser.parse(argv);
  if (local_size < 1) {
    fprintf(stderr, "error: local size must be positive\n");
    return 7;
  } else if (global_size < 1) {
    fprintf(stderr, "error: global size must be positive\n");
    return 7;
  } else if ((global_size % local_size) != 0) {
    fprintf(stderr,
            "error: global size (%zd) must be a multiple of local size"
            "(%zd)\n",
            global_size, local_size);
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
                       barrier_print_mux_kernel_binary,
                       barrier_print_mux_kernel_binary_size, allocator,
                       &executable);
  if (!executable) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Run the kernel.
  core_kernel_t kernel = nullptr;
  core_scheduled_kernel_t scheduled_kernel = nullptr;
  core_specialized_kernel_t specialized_kernel = nullptr;
  core_command_group_t command_work = nullptr;
  size_t global_offset = 0;
  const size_t num_args = 0;
  coreCreateKernel(device, finalizer, executable, kernel_entry_function,
                   strlen(kernel_entry_function), allocator, &kernel);
  if (!kernel) {
    fprintf(stderr, "Unable to create a kernel.\n");
    return 5;
  }
  coreCreateScheduledKernel(device, finalizer, kernel, local_size, 1, 1,
                            allocator, &scheduled_kernel);
  coreCreateSpecializedKernel(device, finalizer, scheduled_kernel, nullptr,
                              num_args, &global_offset, &global_size, 1,
                              allocator, &specialized_kernel);
  coreCreateCommandGroup(device, callback, allocator, &command_work);
  corePushNDRange(command_work, specialized_kernel);

  // Start executing commands on the device.
  printf(
      "Running barrier_print_mux example (Global size: %zu, local size: %zu)\n",
      global_size, local_size);
  coreDispatch(queue, command_work, nullptr, 0, nullptr, 0, nullptr, nullptr);

  // Wait for all commands to have finished executing on the device.
  coreWaitAll(queue);

  // Clean up
  coreDestroyCommandGroup(device, command_work, allocator);
  coreDestroySpecializedKernel(device, specialized_kernel, allocator);
  coreDestroyScheduledKernel(device, finalizer, scheduled_kernel, allocator);
  coreDestroyKernel(device, finalizer, kernel, allocator);
  coreDestroyExecutable(device, finalizer, executable, allocator);
  coreDestroyFinalizer(device->info, finalizer, allocator);
  coreDestroyDevice(device, allocator);
  return 0;
}
