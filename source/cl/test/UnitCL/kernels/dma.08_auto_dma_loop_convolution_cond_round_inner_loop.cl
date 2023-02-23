// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This should fail to DMA as the lookup is conditional
__kernel void auto_dma_loop_convolution_cond_round_inner_loop(
    __global uint *src, __global uint *dst) {
  uint dstYStride = get_global_size(0);
  uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  uint srcYStride = dstYStride + 16;
  uint srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  int y = 0;
  for (int y = 0; y < 3; y++) {
    if (y == 1) {
      for (int x = 0; x < 3; x++) {
        count = count + src[srcYStride * y + srcIndex + x - 1];
      }
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}
