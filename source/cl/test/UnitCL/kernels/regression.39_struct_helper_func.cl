// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

typedef struct {
  char offset;
  short3 m;
} testStruct;

testStruct returnHelper() {
  __private testStruct x = {'a', {1, 2, 3}};
  return x;
}

ulong passValHelper(testStruct x) { return (ulong) & (x.m); }

ulong passPtrHelper(testStruct *x) { return (ulong) & (x->m); }

kernel void struct_helper_func(__global ulong *mask, __global ulong *out) {
  testStruct myStruct = returnHelper();
  out[0] = passValHelper(myStruct) & mask[0];
  out[1] = passPtrHelper(&myStruct) & mask[1];
}
