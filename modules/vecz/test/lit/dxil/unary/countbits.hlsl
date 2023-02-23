// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain countbits.hlsl -Fo countbits.bc

// RUN: %veczc -o %t.bc %S/countbits.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint2> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].x = countbits(a[gid.x].y);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call <4 x i32> @llvm.ctpop.v4i32(<4 x i32>
