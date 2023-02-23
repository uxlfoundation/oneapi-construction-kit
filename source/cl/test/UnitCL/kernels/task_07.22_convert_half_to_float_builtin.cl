// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void convert_half_to_float_builtin(global ushort *src,
                                          global uint *dst) {
  size_t tid = get_global_id(0);
  dst[tid] = as_uint(vload_half(tid, (global half *)src));
}
