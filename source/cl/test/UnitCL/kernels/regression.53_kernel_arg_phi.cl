// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DSIZE=45";"-DLOOPS=5"

__kernel void kernel_arg_phi(__global uchar* dst_ptr, int dst_step) {
  const int x = get_group_id(0);
  const int y = get_global_id(1);

  const int block_size = SIZE / LOOPS;

  __local int2 smem[SIZE];

  for (int i = 0; i < LOOPS; i++) {
    smem[y + i * block_size] = (int2)('A', 'B');
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  __global uchar* dst = dst_ptr + mad24(x, (int)sizeof(int2), y * dst_step);
  for (int i = 0; i < LOOPS; i++, dst += block_size * dst_step) {
    vstore2(smem[y + i * block_size], 0, (__global int*)dst);
  }
}
