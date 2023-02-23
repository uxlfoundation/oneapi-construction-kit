// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain bfrev.hlsl -Fo bfrev.bc

// RUN: %veczc -o %t.bc %S/bfrev.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint4x1> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = reversebits(a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call <2 x i32> @llvm.bitreverse.v2i32(<2 x i32>
// CHECK: call <2 x i32> @llvm.bitreverse.v2i32(<2 x i32>
// CHECK: call <2 x i32> @llvm.bitreverse.v2i32(<2 x i32>
// CHECK: call <2 x i32> @llvm.bitreverse.v2i32(<2 x i32>
