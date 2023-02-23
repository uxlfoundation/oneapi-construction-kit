// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct __attribute__((packed)) {
  uint id[4];
} id_data;

__kernel void global_id_array4(__global uint* size, __global id_data* out) {
  size_t gid = get_global_id(0);

  size[gid] = sizeof(id_data);

  for (int i = 0; i < 4; i++) {
    // The OpenCL spec says that get_global_id(3) returns "0" even though the
    // value "3" exceeds the value of "get_work_dim()".
    out[gid].id[i] = (uint)get_global_id(i);
  }
}
