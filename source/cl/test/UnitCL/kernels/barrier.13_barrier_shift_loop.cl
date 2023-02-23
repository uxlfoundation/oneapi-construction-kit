// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DBLOCK_COLS=32";"-DBLOCK_ROWS=32";"-DLOCAL_SIZE=256"

__kernel void barrier_shift_loop(__global uchar *dst, int rows, int columns) {
  int block_x = get_group_id(0);
  int block_y = get_group_id(1);
  int id = get_local_id(0);

  int x0 = block_x * BLOCK_COLS, x1 = min(x0 + BLOCK_COLS, columns);
  int y0 = block_y * BLOCK_ROWS, y1 = min(y0 + BLOCK_ROWS, rows);

  const int step = 512;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      int index = mad24(y, step, x);

      barrier(CLK_GLOBAL_MEM_FENCE);

      for (int lsize = LOCAL_SIZE >> 1; lsize > 2; lsize >>= 1) {
        barrier(CLK_GLOBAL_MEM_FENCE);
      }

      if (id == 0) {
        dst[index] = 'A';
      }
    }
  }
}
