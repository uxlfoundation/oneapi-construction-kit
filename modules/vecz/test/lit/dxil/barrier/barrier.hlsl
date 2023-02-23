// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain barrier.hlsl -Fo barrier.bc

// RUN: %veczc -o %t.bc %S/barrier.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

groupshared float b[16];

[numthreads(16, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID, uint3 lid : SV_GroupThreadID) {
  b[lid.x] = a[gid.x];

  GroupMemoryBarrier();

  float temp = b[0];

  for (uint i = 1; i < 16; i++) {
    temp += b[i];
  }

  a[gid.x] = temp;
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call void @dx.op.barrier(i32 80, i32 8)
