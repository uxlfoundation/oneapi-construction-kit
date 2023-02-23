__kernel void negative_comparison(__global float *dst, const int size,
                                  const int t) {
  int globalId = get_global_id(0);

  if (globalId < size - 1 - t) {
    dst[globalId] = globalId * 3.0f;
  } else {
    dst[globalId] = globalId * 4.0f;
  }
}
