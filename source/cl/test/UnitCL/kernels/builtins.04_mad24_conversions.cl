// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void mad24_conversions(char a, int b, float c, global float* result) {
  *result = mad24(a, c, b);
}
