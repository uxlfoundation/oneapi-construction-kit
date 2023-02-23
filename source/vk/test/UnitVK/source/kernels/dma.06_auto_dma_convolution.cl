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
