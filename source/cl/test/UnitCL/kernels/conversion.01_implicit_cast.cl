// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifdef STORE_FUNC
#define STORE(data, offset, ptr) STORE_FUNC(data, offset, ptr)
#else
#define STORE(data, offset, ptr) ptr[tid] = data
#endif

__kernel void implicit_cast(__global IN_TYPE *in, __global OUT_TYPE_SCALAR *out) {
  size_t tid = get_global_id(0);
  OUT_TYPE_VECTOR casted = in[tid];
  STORE(casted, tid, out);
}
