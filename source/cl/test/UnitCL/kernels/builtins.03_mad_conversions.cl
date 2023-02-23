// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void mad_conversions(char a, int b, float c, global float* result) {
  *result = mad(a, c, b);
}
