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
// DEFINITIONS: "-DNUM_ELEMENTS=2"
struct __attribute__ ((packed)) long_array {
  long data[1];
};

struct __attribute__ ((packed)) s_loops {
  struct long_array loops;
  struct __attribute__ ((packed)) {
    char unused2;
  };
  char unused[7];
};

struct __attribute__ ((packed)) s_step {
  struct long_array step;
  struct s_loops x;
};

struct __attribute__ ((packed)) s_scheduling {
  struct long_array stride;
  struct s_step x;
};

struct __attribute__ ((packed)) s_wrapper {
  struct __attribute__ ((packed)) {
    struct long_array unused;
  };
  struct s_scheduling sched;
};

struct __attribute__ ((packed)) s_top_level {
  struct __attribute__ ((packed)) {
    char unused[2];
  };
  struct s_wrapper wrap;
};

__kernel void nested_structs(__global int* out, struct s_top_level unused,
                             __global int* in) {
  const size_t gid = get_global_id(0);
  const size_t stride = get_global_size(0);

  size_t index = gid * stride;
  int sum = 0;
  for (int i = 0; i < NUM_ELEMENTS; i++) {
    index += i;
    sum += in[index];
  }
  out[gid] = sum;
}
