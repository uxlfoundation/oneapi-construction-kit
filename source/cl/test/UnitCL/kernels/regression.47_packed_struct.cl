// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

typedef struct {
  short s;
  float f;
} paddedStruct;

typedef struct {
  char2 c;
  paddedStruct p;
} __attribute__((packed)) packedStruct;

__kernel void packed_struct(__global ulong *out) {
  const size_t gid = get_global_id(0);
  packedStruct s = {('a', 'b'), {42, 3.14f}};
  s.p.s = gid;

  out[gid] = (ulong)(&s.p) - (ulong)(&s);
}
