// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void scalar_loop_tail(global float *output, long out_size)
{
  // output bulk
  unsigned long idx = get_global_id(0);
  float ran;
  while (true)
  {
    for (int i=0;i < 2; i++){
    }
    ran = 1.f;

    if (!(idx + 1 < out_size)) break;
    output[idx] = ran;
    idx += get_global_size(0);
  }

  // output tail
  if (idx < out_size)
    output[idx] = ran;
}
