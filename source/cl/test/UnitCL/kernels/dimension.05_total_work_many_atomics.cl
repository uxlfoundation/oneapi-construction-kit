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

// Based off the OpenCL 2.0 get_global_linear_id.
size_t linear_id() {
  if (get_work_dim() == 1) {
    return get_global_id(0) - get_global_offset(0);
  } else if (get_work_dim() == 2) {
    return (get_global_id(1) - get_global_offset(1)) * get_global_size(0) +
           (get_global_id(0) - get_global_offset(0));
  } else { // Presumably the three dimensional case.
    return ((get_global_id(2) - get_global_offset(2)) * get_global_size(1) *
            get_global_size(0)) +
           ((get_global_id(1) - get_global_offset(1)) * get_global_size(0)) +
           (get_global_id(0) - get_global_offset(0));
  }
}

__kernel void total_work_many_atomics(volatile __global unsigned int *count,
                                      ulong lim) {
  // UnitCL ensures that all elements of count are 0 before the kernel starts
  // (see GenericStreamer<T, V>::PopulateBuffer).
  //
  // See the test body for discussion of the legality of this atomic.
  size_t id = linear_id() % lim;
  atomic_inc(&(count[id]));
}
