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
//
// This example shows how in-kernel DMA can be used to efficiently copy data
// from one area of memory to another from a clik kernel. The kernel provided
// with this example takes two source buffers and concatenates them to a
// destination buffer using concurrent DMA operations.

#include <stdio.h>

#include <vector>

#include "clik_async_api.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_concatenate_dma.c' and turning it into a C array using the Bin2H
// tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  size_t local_size = 1;
  size_t global_size = 1024;
  option_parser_t parser;
  parser.help([]() {
    fprintf(stderr,
            "Usage: ./concatenate_dma [--local-size L] [--global-size S]\n");
  });
  parser.option('S', "global-size", 1,
                [&](const char *s) { global_size = strtoull(s, 0, 0); });
  parser.parse(argv);
  if (global_size < 1) {
    fprintf(stderr, "error: global size must be positive\n");
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
  clik_program *program =
      clik_create_program(device, concatenate_dma_kernel_binary,
                          concatenate_dma_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Initialize host data.
  constexpr const size_t block_size = 128;
  size_t num_src_elements = global_size * block_size;
  size_t num_dst_elements = num_src_elements * 2;
  std::vector<uint32_t> src1_data(num_src_elements);
  std::vector<uint32_t> src2_data(num_src_elements);
  std::vector<uint32_t> dst_data(num_dst_elements);
  for (size_t j = 0; j < num_src_elements; j++) {
    src1_data[j] = j;
    src2_data[j] = 42 - j;
  }
  for (size_t j = 0; j < num_dst_elements; j++) {
    dst_data[j] = ~0u;
  }

  // Create buffers in device memory.
  uint64_t src_buffer_size = num_src_elements * sizeof(uint32_t);
  uint64_t dst_buffer_size = num_dst_elements * sizeof(uint32_t);
  clik_buffer *src1_buffer = clik_create_buffer(device, src_buffer_size);
  clik_buffer *src2_buffer = clik_create_buffer(device, src_buffer_size);
  clik_buffer *dst_buffer = clik_create_buffer(device, dst_buffer_size);
  if (!src1_buffer || !src2_buffer || !dst_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Write host data to device memory.
  if (!clik_enqueue_write_buffer(queue, src1_buffer, 0, &src1_data[0],
                                 src_buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the src1 buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, src2_buffer, 0, &src2_data[0],
                                 src_buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the src2 buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, dst_buffer, 0, &dst_data[0],
                                 dst_buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the dst buffer.\n");
    return 4;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_1d(&ndrange, global_size, local_size);
  printf(
      "Running concatenate_dma example (Global size: %zu, local size: %zu)\n",
      ndrange.global[0], ndrange.local[0]);

  const size_t num_args = 4;
  clik_argument args[num_args];
  clik_init_buffer_arg(&args[0], src1_buffer);
  clik_init_buffer_arg(&args[1], src2_buffer);
  clik_init_buffer_arg(&args[2], dst_buffer);
  clik_init_scalar_arg(&args[3], block_size);
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
  if (!clik_enqueue_read_buffer(queue, &dst_data[0], dst_buffer, 0,
                                dst_buffer_size)) {
    fprintf(stderr, "Could not read the output data from the kernel.\n");
    return 6;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

  // Validate output buffer, which should be the result of concatenating the two
  // input buffers.
  bool validated = true;
  size_t num_errors = 0;
  const size_t max_print_errors = 10;
  for (size_t i = 0; i < num_dst_elements; i++) {
    uint32_t expected =
        (i < num_src_elements) ? src1_data[i] : src2_data[i - num_src_elements];
    uint32_t actual = dst_data[i];
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

  clik_release_buffer(src1_buffer);
  clik_release_buffer(src2_buffer);
  clik_release_buffer(dst_buffer);
  clik_release_kernel(kernel);
  clik_release_program(program);
  clik_release_device(device);
  return validated ? 0 : -1;
}
