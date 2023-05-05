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

size_t get_sycl_global_linear_id() {
    return ((get_global_id(0) * get_global_size(1) * get_global_size(2))
           + (get_global_id(1) * get_global_size(2))
           + get_global_id(2));
}

__kernel void multiple_dimensions_0(__global int* output) {
  size_t sycl_global_linear_id = get_sycl_global_linear_id();
  output[sycl_global_linear_id] = (int)sycl_global_linear_id;
}
