// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DFILTER_SIZE=1"

#if defined(KTS_DEFAULTS)
#define FILTER_SIZE 3
#endif

__kernel void convolution(__global float *src, __global float *dst) {
  int x = get_global_id(0);
  int width = get_global_size(0);
  if ((x >= FILTER_SIZE) && (x < (width - FILTER_SIZE))) {
    float sum = 0.0f;
    float totalWeight = 0.0f;
    for (int i = -FILTER_SIZE; i <= FILTER_SIZE; i++) {
      float weight = convert_float(x - i);
      totalWeight += weight;
      sum += weight * src[x + i];
    }
    sum /= totalWeight;
    dst[x] = sum;
  } else {
    dst[x] = 0.0f;
  }
}
