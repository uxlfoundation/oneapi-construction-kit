// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half parameters
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifdef STORE_FUNC
#define STORE(data, offset, ptr) STORE_FUNC(data, offset, ptr)
#else
#define STORE(data, offset, ptr) ptr[tid] = data
#endif

#ifdef LOAD_FUNC
#define LOAD(offset, ptr) LOAD_FUNC(offset, ptr)
#else
#define LOAD(offset, ptr) ptr[tid]
#endif

__kernel void reinterpret(__global IN_TYPE_SCALAR *in, __global OUT_TYPE_SCALAR *out) {
  size_t tid = get_global_id(0);
  IN_TYPE_VECTOR x = LOAD(tid, in);
  OUT_TYPE_VECTOR reinterp = AS_FUNC(x);
  STORE(reinterp, tid, out);
}
