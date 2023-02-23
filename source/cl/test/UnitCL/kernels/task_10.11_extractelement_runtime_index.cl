// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

kernel void extractelement_runtime_index(global int4* in, global int4* out,
                                         global int* indices) {
  size_t gid = get_global_id(0);

  out[gid] = (4, 4, 4, 4);
  out[gid][2] = in[gid][indices[0]];
}
