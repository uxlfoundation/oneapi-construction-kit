// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void global_id_zero_parameter(uint dim1, __global uint* out) {
  out[get_global_id(dim1)] = (uint)get_global_id((uint)get_global_id(0));
}
