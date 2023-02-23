// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void convert3( __global long *src, __global float *dest )
{
   size_t i = get_global_id(0);
   vstore3( convert_float3( vload3( i, src)), i, dest );
}
