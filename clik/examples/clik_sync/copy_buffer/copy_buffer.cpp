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

#include "clik_sync_api.h"
#include "option_parser.h"

int main(int argc, char **argv) {
  // Process command line options.
  size_t buffer_len = 1024;
  option_parser_t parser;
  parser.help(
      []() { fprintf(stderr, "Usage: ./copy_buffer [--buffer-len N]\n"); });
  parser.option('N', "buffer-len", 1,
                [&](const char *s) { buffer_len = strtoull(s, 0, 0); });
  parser.parse(argv);
  if (buffer_len < 1) {
    fprintf(stderr, "error: buffer length must be positive\n");
    return 7;
  }

  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }

  // Initialize host data.
  size_t num_elements = buffer_len;
  std::vector<uint32_t> src_data(num_elements);
  std::vector<uint32_t> dst_data(num_elements);
  for (size_t j = 0; j < num_elements; j++) {
    src_data[j] = j;
    dst_data[j] = ~0u;
  }

  // Create buffers in device memory.
  uint64_t buffer_size = num_elements * sizeof(uint32_t);
  clik_buffer *src_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *dst_buffer = clik_create_buffer(device, buffer_size);
  if (!src_buffer || !dst_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Write host data to device memory.
  if (!clik_write_buffer(device, src_buffer, 0, &src_data[0], buffer_size)) {
    fprintf(stderr, "Could not write to the src1 buffer.\n");
    return 4;
  }
  if (!clik_write_buffer(device, dst_buffer, 0, &dst_data[0], buffer_size)) {
    fprintf(stderr, "Could not write to the dst buffer.\n");
    return 4;
  }

  // Copy the data from the input buffer to the output buffer.
  if (!clik_copy_buffer(device, dst_buffer, 0, src_buffer, 0, buffer_size)) {
    fprintf(stderr, "Could not copy data from one buffer to another.\n");
    return 6;
  }

  // Read the data in the output buffer.
  if (!clik_read_buffer(device, &dst_data[0], dst_buffer, 0, buffer_size)) {
    fprintf(stderr, "Could not read the output data from the buffer.\n");
    return 6;
  }

  // Validate output buffer.
  bool validated = true;
  size_t num_errors = 0;
  const size_t max_print_errors = 10;
  for (size_t i = 0; i < num_elements; i++) {
    uint32_t expected = src_data[i];
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

  clik_release_buffer(src_buffer);
  clik_release_buffer(dst_buffer);
  clik_release_device(device);
  return validated ? 0 : -1;
}
