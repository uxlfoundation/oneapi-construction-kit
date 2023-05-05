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

#include <stdio.h>

#include "clik_async_api.h"
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
            "Usage: ./barrier_print [--local-size L] [--global-size S]\n");
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

  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }
  clik_command_queue *queue = clik_get_device_queue(device);

  // Load the kernel program.
  clik_program *program = clik_create_program(
      device, barrier_print_kernel_binary, barrier_print_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_1d(&ndrange, global_size, local_size);

  printf("Running barrier_print example (Global size: %zu, local size: %zu)\n",
         ndrange.global[0], ndrange.local[0]);

  clik_kernel *kernel =
      clik_create_kernel(program, "kernel_main", &ndrange, nullptr, 0);
  if (!kernel) {
    fprintf(stderr, "Unable to create a kernel.\n");
    return 5;
  } else if (!clik_enqueue_kernel(queue, kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

  clik_release_kernel(kernel);
  clik_release_program(program);
  clik_release_device(device);
  return 0;
}
