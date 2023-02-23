// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void insertelement_runtime_index(global int4* in, global int4* out,
                                        global int* index) {
  size_t gid = get_global_id(0);

  out[gid] = in[gid];

  // This is not actually legal OpenCL C, the only specified way to access
  // elements of a vector is via either .xyzw, or .s0123 style syntax.
  // However, Clang accepts this and the test specifically depends on the index
  // being a runtime value (the other syntaxes are implicitly compile time
  // indices).
  out[gid][index[gid]] = 42;
}
