// Copyright (C) Codeplay Software Limited. All Rights Reserved.

void __attribute__((overloadable))
barrier(cl_mem_fence_flags flags);

void Barrier_Function() { barrier(CLK_LOCAL_MEM_FENCE); }

__kernel void barrier_in_function() { Barrier_Function(); }
