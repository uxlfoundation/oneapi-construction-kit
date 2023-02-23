// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void convert_half_to_float_vload(global ushort *src, global uint *dst) {
  size_t tid = get_global_id(0);
 private
  ushort temp[1];
 private
  half *h = (private half *)temp;
  temp[0] = src[tid];
  dst[tid] = as_uint(vload_half(0, h));
}
