// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <math.h>
#include <stdio.h>

#include <vector>

#include "clik_async_api.h"
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
    fprintf(stderr,
            "Usage: ./matrix_multiply [--local-size L] [--matrix-size M]\n");
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

  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }
  clik_command_queue *queue = clik_get_device_queue(device);

  // Load the kernel program.
  clik_program *program =
      clik_create_program(device, matrix_multiply_kernel_binary,
                          matrix_multiply_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Initialize host data.
  size_t M = matrix_size;
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
  clik_buffer *a_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *b_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *c_buffer = clik_create_buffer(device, buffer_size);
  if (!a_buffer || !b_buffer || !c_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Write host data to device memory.
  if (!clik_enqueue_write_buffer(queue, a_buffer, 0, &a_data[0], buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the A buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, b_buffer, 0, &b_data[0], buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the B buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, c_buffer, 0, &c_data[0], buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the C buffer.\n");
    return 4;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_2d(&ndrange, M, M, local_size, 1);

  printf(
      "Running matrix_multiply example (Global size: %zux%zu, "
      "local size: %zux%zu)\n",
      ndrange.global[0], ndrange.global[1], ndrange.local[0], ndrange.local[1]);

  const size_t num_args = 4;
  clik_argument args[num_args];
  clik_init_buffer_arg(&args[0], a_buffer);
  clik_init_buffer_arg(&args[1], b_buffer);
  clik_init_buffer_arg(&args[2], c_buffer);
  clik_init_scalar_arg(&args[3], M);
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
  if (!clik_enqueue_read_buffer(queue, &c_data[0], c_buffer, 0, buffer_size)) {
    fprintf(stderr, "Could not read the output data from the kernel.\n");
    return 6;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

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

  clik_release_buffer(a_buffer);
  clik_release_buffer(b_buffer);
  clik_release_buffer(c_buffer);
  clik_release_kernel(kernel);
  clik_release_program(program);
  clik_release_device(device);
  return validated ? 0 : -1;
}
