// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

__kernel void local_select(__global int* out) {
  __local bool local_even[SIZE];
  __local bool local_odd[SIZE];

  int gid = get_global_id(0);
  int lid = get_local_id(0);
  if ((lid % 2) == 0) {
    local_even[lid] = true;
    local_odd[lid] = false;
  } else {
    local_odd[lid] = true;
    local_even[lid] = false;
  }

  out[gid] = local_even[lid] || local_odd[lid];
}
