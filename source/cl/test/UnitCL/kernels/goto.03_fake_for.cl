// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void fake_for(__global int *dst, int iter) {
  size_t gid = get_global_id(0);

  int i = 0;

  goto start;

  start: {
    if (i < iter) {
      goto head;
    } else {
      goto end;
    }
  }

  head: {
    i += 1;
    goto start;
  }

  end: {
    dst[gid] = i;
  }
}
