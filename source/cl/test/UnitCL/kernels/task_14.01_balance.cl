// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel is meant to test scalarization and vectorization of FNeg.
__kernel void balance(const float src, __global float4 *dst) {

  size_t x = get_global_id(0);

  const size_t size_x = get_local_size(0);

  float4 value = dst[x];

  // operator2 should generate a fneg operation. Furthermore, make operator1
  // and operator2 not trivially optimizable by frontend by adding dependency
  // on get_global_id
  float operator1 = src + (float)x;
  float operator2 = (- src) - (float)x;

  if (x <= size_x / 2) {
    value = 1.0f - value;
  }

  // should generate two fneg operations
  if (x % 2) {
    operator1 = - operator1;
    operator2 = - operator2;
  }

  dst[x] = value * (operator1 + operator2);
}
