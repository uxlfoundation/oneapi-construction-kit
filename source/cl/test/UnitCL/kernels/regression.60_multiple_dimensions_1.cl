// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// The test, although looking very similar
// to regression.60_multiple_dimensions_0.cl, is necessary to compute
// the get_sycl_global_linear_id() the same way ComputeCpp does.
size_t get_sycl_global_linear_id() {
    return (((get_global_id(0) * get_global_size(1))
           + get_global_id(1)) * get_global_size(2)
           + get_global_id(2));
}

__kernel void multiple_dimensions_1(__global int* output) {
  size_t sycl_global_linear_id = get_sycl_global_linear_id();
  output[sycl_global_linear_id] = (int)sycl_global_linear_id;
}
