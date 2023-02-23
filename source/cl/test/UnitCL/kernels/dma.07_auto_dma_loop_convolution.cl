// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DMA pass should handle these canonical loops (0..N)
__kernel void auto_dma_loop_convolution(__global uint *src,
                                        __global uint *dst) {
  size_t dstYStride = get_global_size(0);
  size_t dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  size_t srcYStride = dstYStride + 16;
  size_t srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  for (size_t y = 0; y < 3; y++) {
    for (size_t x = 0; x < 3; x++) {
      count = count + src[srcYStride * y + srcIndex + x - 1];
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}
