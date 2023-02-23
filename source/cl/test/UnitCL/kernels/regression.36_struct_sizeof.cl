// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#define ARRAY_SIZE 4

typedef struct {
  char c;
  float3 f;
  int i;
  ulong l[2];
} testStruct;

kernel void struct_sizeof(__global float3* out1, __global uint* out2) {
  const size_t tid = get_global_id(0);
  testStruct myStructs[ARRAY_SIZE];
  const size_t lastIndex = ARRAY_SIZE - 1;

  // Test sizeof() by using it to get a pointer to the last element in struct
  // array. Then write a value here so we can verify correctness host side.
  uchar* base = (uchar*)&myStructs;
  testStruct* lastStruct =
      (testStruct*)(base + (lastIndex * sizeof(testStruct)));
  lastStruct->f = (float3)((float)tid, tid + 0.2f, tid + 0.5f);

  vstore3(myStructs[lastIndex].f, tid, (__global float*)out1);
  out2[tid] = sizeof(testStruct);
}
