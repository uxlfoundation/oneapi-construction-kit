// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: images

// This test has no real purpose besides preventing a regression when trying
// packetize a CFG that still has vector instructions in it, which triggers a
// call to `emitRemarkOptimizer` which was calling wrongly the ORE API.
void __kernel check_ore_call(__global uint *out, __write_only image2d_t img) {
  size_t gid = get_global_id(0);
  write_imagef(img, (int2)(gid, gid), (float4)(0.0f, 0.0f, 0.0f, 1.0f));
  out[gid] = 0;
}
