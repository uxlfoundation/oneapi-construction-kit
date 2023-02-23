// Copyright (C) Codeplay Software Limited. All Rights Reserved.

size_t get_sycl_global_linear_id() {
  return ((get_global_id(0) * get_global_size(1) * get_global_size(2)) +
          (get_global_id(1) * get_global_size(2)) + get_global_id(2));
}

__kernel void global_id_parameter(uint dim, __global uint* out) {
  size_t gid = get_sycl_global_linear_id();
  out[gid] = (uint)get_global_id(dim);
}
