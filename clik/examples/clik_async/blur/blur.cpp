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

#include "clik_async_api.h"
#include "constants.h"

// This header contains the kernel binary resulting from compiling
// 'device_vector_add_async.c' and turning it into a C array using the Bin2H
// tool.
#include "blur_kernel_bin.h"

static const uint32_t input[WIDTH * HEIGHT] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  99, 0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  99, 0,  0,  0,  0,  0,  0,  99, 99, 99, 99,
    99, 99, 0,  0,  0,  99, 0,  0,  0,  0,  0,  99, 99, 99, 99, 99, 99, 0,
    0,  0,  0,  33, 0,  0,  0,  0,  99, 99, 99, 99, 99, 99, 0,  0,  0,  0,
    0,  33, 0,  0,  0,  99, 99, 99, 99, 99, 99, 66, 0,  0,  0,  0,  0,  33,
    0,  0,  0,  0,  0,  66, 66, 66, 66, 66, 0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  66, 66, 66, 66, 66, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  66,
    66, 66, 66, 66, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  66, 66, 66,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  18,
};

int main(int argc, char **argv) {
  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }
  clik_command_queue *queue = clik_get_device_queue(device);

  // Load the binary holding the kernels.
  clik_program *program =
      clik_create_program(device, blur_kernel_binary, blur_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Set up buffers. The input_buffer holds the input image, the output_buffer
  // holds the result, the temp_buffer is used for intermediate computations.
  clik_buffer *input_buffer =
      clik_create_buffer(device, sizeof(uint32_t) * WIDTH * HEIGHT);
  clik_buffer *temp_buffer = clik_create_buffer(
      device, sizeof(uint32_t) * EXTENDED_WIDTH * EXTENDED_HEIGHT);
  clik_buffer *output_buffer =
      clik_create_buffer(device, sizeof(uint32_t) * WIDTH * HEIGHT);
  if (!input_buffer || !temp_buffer || !output_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Initialize the input buffer.
  if (!clik_enqueue_write_buffer(queue, input_buffer, 0, &input[0],
                                 sizeof(uint32_t) * WIDTH * HEIGHT)) {
    fprintf(stderr, "Could not enqueue a write to the input buffer.\n");
    return 4;
  }

  // Copy over the input image into the temporary buffer and extend it by one
  // pixel on either side horizontally by duplicating pixels on the left and
  // right side of the image.
  clik_ndrange copy_and_pad_hor_ndrange;
  clik_init_ndrange_1d(&copy_and_pad_hor_ndrange, HEIGHT, 1);
  clik_argument copy_and_pad_hor_args[2];
  clik_init_buffer_arg(&copy_and_pad_hor_args[0], input_buffer);
  clik_init_buffer_arg(&copy_and_pad_hor_args[1], temp_buffer);
  clik_kernel *copy_and_pad_hor_kernel = clik_create_kernel(
      program, "copy_and_pad_hor_main", &copy_and_pad_hor_ndrange,
      &copy_and_pad_hor_args[0], 2);
  if (!copy_and_pad_hor_kernel) {
    fprintf(stderr, "Unable to create the 'copy_and_pad_hor' kernel.\n");
    return 5;
  }
  if (!clik_enqueue_kernel(queue, copy_and_pad_hor_kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Extend the image vertically by duplicating top and bottom pixels.
  clik_ndrange pad_vert_ndrange;
  clik_init_ndrange_1d(&pad_vert_ndrange, EXTENDED_WIDTH, 1);
  clik_argument pad_vert_args[1];
  clik_init_buffer_arg(&pad_vert_args[0], temp_buffer);
  clik_kernel *pad_vert_kernel = clik_create_kernel(
      program, "pad_vert_main", &pad_vert_ndrange, &pad_vert_args[0], 1);
  if (!pad_vert_kernel) {
    fprintf(stderr, "Unable to create the 'pad_vert' kernel.\n");
    return 5;
  }
  if (!clik_enqueue_kernel(queue, pad_vert_kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Blur the image by averaging 3x3 neighborhood.
  clik_ndrange blur_ndrange;
  clik_init_ndrange_2d(&blur_ndrange, WIDTH, HEIGHT, 1, 1);
  clik_argument blur_args[2];
  clik_init_buffer_arg(&blur_args[0], temp_buffer);
  clik_init_buffer_arg(&blur_args[1], output_buffer);
  clik_kernel *blur_kernel =
      clik_create_kernel(program, "blur_main", &blur_ndrange, &blur_args[0], 2);
  if (!blur_kernel) {
    fprintf(stderr, "Unable to create the 'blur' kernel.\n");
    return 5;
  }
  if (!clik_enqueue_kernel(queue, blur_kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Extract the result.
  uint32_t result[WIDTH * HEIGHT] = {0};
  if (!clik_enqueue_read_buffer(queue, &result[0], output_buffer, 0,
                                sizeof(uint32_t) * WIDTH * HEIGHT)) {
    fprintf(stderr, "Could not enqueue reading data from the kernel.\n");
    return 6;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

  // Print the result.
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      fprintf(stderr, " %2d", result[i * WIDTH + j]);
    }
    fprintf(stderr, "\n");
  }

  // Clean up.
  clik_release_kernel(blur_kernel);
  clik_release_kernel(pad_vert_kernel);
  clik_release_kernel(copy_and_pad_hor_kernel);
  clik_release_buffer(input_buffer);
  clik_release_buffer(temp_buffer);
  clik_release_buffer(output_buffer);
  clik_release_program(program);
  clik_release_device(device);

  return 0;
}
