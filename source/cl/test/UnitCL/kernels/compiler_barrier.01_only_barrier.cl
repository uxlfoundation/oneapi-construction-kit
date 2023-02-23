// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void only_barrier() { barrier(CLK_LOCAL_MEM_FENCE); }
