// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void boscc_sese_backdoor(__global uint* out) {
  size_t x = get_global_id(0);
  size_t y = get_global_id(1);
  size_t z = (y << 8) | x;
  ushort scrambled_x = (((ushort)x) ^ 0x4785) * 0x8257;
  ushort scrambled_y = (((ushort)y) ^ 0x126C) * 0x1351;

  uint route = 0;
  if (scrambled_y & 1) {
    route |= 1;
    if (scrambled_y & 2) {
      route ^= scrambled_y;
    }
    goto F;
  } else {
    route |= 8;
    if (scrambled_x & 1) {
      route |= 16;
      goto G;
    }
  }

F:
  route |= 32;

G:
  out[z] = route;
  return;
}
