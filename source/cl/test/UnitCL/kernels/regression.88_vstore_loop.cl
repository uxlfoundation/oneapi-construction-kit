// Copyright (C) Codeplay Software Limited. All Rights Reserved.

typedef struct {
  uint v[4];
} Foo;

uint4 gen_bits(Foo *ctr)
{
  Foo X;

  for (int i=0;i < 4; i++){
    X.v[i]  = ctr->v[i];
  }

  return (uint4)(X.v[0], X.v[1], X.v[2], X.v[3]);
}

kernel void vstore_loop(global float *output, long out_size)
{
  Foo c = {{1, 1, 1, 1}};

  // output bulk
  unsigned long idx = get_global_id(0)*4;
  while (idx + 4 < out_size)
  {
     float4 ran = convert_float4(gen_bits(&c));
     vstore4(ran, 0, &output[idx]);
     idx += 4*get_global_size(0);
  }

  // output tail
  float4 tail_ran = convert_float4(gen_bits(&c));
  if (idx < out_size)
    output[idx] = tail_ran.x;
  if (idx+1 < out_size)
    output[idx+1] = tail_ran.y;
  if (idx+2 < out_size)
    output[idx+2] = tail_ran.z;
  if (idx+3 < out_size)
    output[idx+3] = tail_ran.w;
}
