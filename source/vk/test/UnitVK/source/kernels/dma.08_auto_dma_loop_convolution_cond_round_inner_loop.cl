// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DMA pass should combine these into a single DMA
__kernel void auto_dma_convolution(__global uint *src, __global uint *dst) {
  uint dstYStride = get_global_size(0);
  uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  uint srcYStride = dstYStride + 16;
  uint srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint leftUpper = src[srcIndex - 1];
  uint middleUpper = src[srcIndex];
  uint rightUpper = src[srcIndex + 1];
  srcIndex += srcYStride;
  uint leftMiddle = src[srcIndex - 1];
  uint rightMiddle = src[srcIndex + 1];
  srcIndex += srcYStride;
  uint leftLower = src[srcIndex - 1];
  uint middleLower = src[srcIndex];
  uint rightLower = src[srcIndex + 1];

  // Write data out
  dst[dstIndex] = (8 + leftUpper + middleUpper + rightUpper + leftMiddle +
                   rightMiddle + leftLower + middleLower + rightLower) /
                  8;
}

// DMA pass should handle these canonical loops (0..N)
__kernel void auto_dma_loop_convolution(__global uint *src,
                                        __global uint *dst) {
  uint dstYStride = get_global_size(0);
  uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  uint srcYStride = dstYStride + 16;
  uint srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  int y = 0;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      count = count + src[srcYStride * y + srcIndex + x - 1];
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}

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

// This one should succeed the DMA as the conditional is common across the group
__kernel void auto_dma_loop_convolution_cond_not_global_id(__global uint *src,
                                                           __global uint *dst,
                                                           uint extraParam) {
  uint dstYStride = get_global_size(0);
  uint dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  uint srcYStride = dstYStride + 16;
  uint srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  if (extraParam > 1) {
    count += 10;
  }
  int y = 0;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      count = count + src[srcYStride * y + srcIndex + x - 1];
    }
  }
  // Write data out
  dst[dstIndex] = count / 9;
}
