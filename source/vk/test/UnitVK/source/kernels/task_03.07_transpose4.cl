// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void transpose4(__global int4 *in, __global int4 *out) {
  size_t tid = get_global_id(0);

  int4 sa = in[(tid * 4) + 0];
  int4 sb = in[(tid * 4) + 1];
  int4 sc = in[(tid * 4) + 2];
  int4 sd = in[(tid * 4) + 3];

  int4 da = (int4)(sa.x, sb.x, sc.x, sd.x);
  int4 db = (int4)(sa.y, sb.y, sc.y, sd.y);
  int4 dc = (int4)(sa.z, sb.z, sc.z, sd.z);
  int4 dd = (int4)(sa.w, sb.w, sc.w, sd.w);

  out[(tid * 4) + 0] = da;
  out[(tid * 4) + 1] = db;
  out[(tid * 4) + 2] = dc;
  out[(tid * 4) + 3] = dd;
}
