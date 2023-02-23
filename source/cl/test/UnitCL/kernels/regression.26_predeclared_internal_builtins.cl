// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: images

kernel void failed_function(global int *src, global int *dst,
                            read_only image2d_t img, sampler_t smplr) {
  size_t gid = get_global_id(0);
  // Force masked loads and stores
  if (gid) {
    // Uniform value
    int4 src_0 = vload4(0, src);
    dst[0] = src_0.s0;
    // We can't handle images
    dst[gid] = read_imagei(img, smplr, (int2)(2, 3)).x;
  }
}

kernel void predeclared_internal_builtins(global int *src, global int *dst,
                                          int i) {
  size_t gid = get_global_id(0);
  // Force masked loads and stores
  if (gid) {
    // Uniform value
    int4 src_0 = vload4(i, src);
    dst[i] = src_0.s0;
  }

  dst[gid] = src[gid];
}
