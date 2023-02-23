// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct __attribute__((packed)) {
  uint id0;
  uint id1;
  uint id2;
} id_data;

__kernel void global_id_elements(__global uint *size, __global id_data* out) {
  size_t gid = get_global_id(0);

  size[gid] = sizeof(id_data);

  out[gid].id0 = (uint)get_global_id(0);
  out[gid].id1 = (uint)get_global_id(1);
  out[gid].id2 = (uint)get_global_id(2);
}
