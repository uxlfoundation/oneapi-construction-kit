// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct S {
  int a;
  int b;
  int c;
  int d;
  float e;
  int f;
};

__kernel void memcpy_optimization(__global int4 *in, __global int4 *out) {
  size_t gid = get_global_id(0);
  int4 tmp = in[gid];
  struct S in_s = {tmp.s0, tmp.s1, 42, tmp.s2, 25.81, tmp.s3};
  struct S out_s = in_s;
  out[gid] = (int4)(out_s.a, out_s.b, out_s.d, out_s.f);
}
