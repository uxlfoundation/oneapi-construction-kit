// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void fake_if(__global int *dst) {
  size_t gid = get_global_id(0);

  bool jmp = (gid % 2) == 1;

  if (jmp == 0) goto left;
  if (jmp == 1) goto right;
  goto end; // Unreachable;

  left: {
    dst[gid] = 0;
    goto end;
  }

  right: {
    dst[gid] = 1;
    goto end;
  }

  end: {}
}
