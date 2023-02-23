// Copyright (C) Codeplay Software Limited. All Rights Reserved.

size_t get_sycl_global_linear_id() {
    return ((get_global_id(0) * get_global_size(1) * get_global_size(2))
           + (get_global_id(1) * get_global_size(2))
           + get_global_id(2));
}

__kernel void multiple_dimensions_0(__global int* output) {
  size_t sycl_global_linear_id = get_sycl_global_linear_id();
  output[sycl_global_linear_id] = (int)sycl_global_linear_id;
}
