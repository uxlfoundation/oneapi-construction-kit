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
#include <stdlib.h>

#include <vector>

#include "clik_async_api.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_ternary.c' and turning it into a C array using the Bin2H tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  size_t local_size = 16;
  size_t global_size = 1024;
  option_parser_t parser;
  parser.help([]() {
    fprintf(stderr,
            "Usage: ./ternary_async [--local-size L] [--global-size S]\n");
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

  // Set up the device.
  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }
  clik_command_queue *queue = clik_get_device_queue(device);

  // Load the kernel program.
  clik_program *program = clik_create_program(
      device, ternary_async_kernel_binary, ternary_async_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Initialize host data.
  size_t num_elements = global_size;
  std::vector<int32_t> in1_data(num_elements);
  std::vector<int32_t> out_data(num_elements);
  for (size_t j = 0; j < num_elements; j++) {
    in1_data[j] = j % 3;
    out_data[j] = -1;
  }

  // Create buffers in device memory.
  uint64_t buffer_size = num_elements * sizeof(int32_t);
  clik_buffer *in1_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *out_buffer = clik_create_buffer(device, buffer_size);
  if (!in1_buffer || !out_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Write host data to device memory.
  if (!clik_enqueue_write_buffer(queue, in1_buffer, 0, &in1_data[0],
                                 buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the in1 buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, out_buffer, 0, &out_data[0],
                                 buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the out buffer.\n");
    return 4;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_1d(&ndrange, num_elements, local_size);
  printf("Running ternary_async example (Global size: %zu, local size: %zu)\n",
         ndrange.global[0], ndrange.local[0]);

  const size_t num_args = 5;
  int32_t bias = 2;
  int32_t trueVal = 0;
  int32_t falseVal = -4;
  clik_argument args[num_args];
  clik_init_buffer_arg(&args[0], in1_buffer);
  clik_init_scalar_arg(&args[1], bias);
  clik_init_buffer_arg(&args[2], out_buffer);
  clik_init_scalar_arg(&args[3], trueVal);
  clik_init_scalar_arg(&args[4], falseVal);
  clik_kernel *kernel =
      clik_create_kernel(program, "kernel_main", &ndrange, &args[0], num_args);
  if (!kernel) {
    fprintf(stderr, "Unable to create a kernel.\n");
    return 5;
  } else if (!clik_enqueue_kernel(queue, kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Read the data produced by the kernel.
  if (!clik_enqueue_read_buffer(queue, &out_data[0], out_buffer, 0,
                                buffer_size)) {
    fprintf(stderr, "Could not read the output data from the kernel.\n");
    return 6;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

  // Validate output buffer.
  bool validated = true;
  size_t num_errors = 0;
  const size_t max_print_errors = 10;
  for (size_t i = 0; i < num_elements; i++) {
    int32_t expected = (i % 3) ? 2 : -2;
    int32_t actual = out_data[i];
    if (expected != actual) {
      num_errors++;
      if (num_errors <= max_print_errors) {
        fprintf(stderr, "Result mismatch at %zu: expected %d, but got %d\n", i,
                expected, actual);
      }
      validated = false;
    }
  }
  if (validated) {
    fprintf(stderr, "Results validated successfully.\n");
  }

  clik_release_buffer(in1_buffer);
  clik_release_buffer(out_buffer);
  clik_release_kernel(kernel);
  clik_release_program(program);
  clik_release_device(device);
  return validated ? 0 : -1;
}
