// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void convert_half_to_float_nested_ifs(global ushort *src,
                                             global uint *dst) {
  size_t tid = get_global_id(0);

  ushort x = src[tid];
  uint xMant = (x & 0x03ff);
  uint xExp = (x & 0x7c00) >> 10;
  uint xSign = (x & 0x8000) >> 15;

  uint y;
  uint yMant;
  uint yExp;
  uint ySign;

  if (xExp == 0) {
    if (xMant == 0) {
      yMant = 0;
      yExp = 0;
      ySign = xSign;
    } else {
      int Exponent = -1;
      uint Mantissa = xMant;
      do {
        Exponent++;
        Mantissa <<= 1;
      } while ((Mantissa & 0x400) == 0);
      yMant = (Mantissa & 0x3ff) << 13;
      yExp = 127 - 15 - Exponent;
      ySign = xSign;
    }
  } else if (xExp == 0x1f) {
    yMant = xMant << (23 - 10);
    yExp = 0xff;
    ySign = xSign;
  } else {
    yMant = xMant << (23 - 10);
    yExp = 127 - 15 + xExp;
    ySign = xSign;
  }

  y = (ySign << 31) | (yExp << 23) | yMant;
  dst[tid] = y;
}
