// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct __attribute__((packed)) {
  uint id[3];
} id_data;

__kernel void global_id_array3(__global uint* size, __global id_data* out) {
  size_t gid = get_global_id(0);

  size[gid] = sizeof(id_data);

  for (int i = 0; i < 3; i++) {
    out[gid].id[i] = (uint)get_global_id(i);
  }
}
