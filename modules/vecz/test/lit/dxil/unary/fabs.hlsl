// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain fabs.hlsl -Fo fabs.bc

// RUN: %veczc -o %t.bc %S/fabs.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float3x3> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = abs(a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.fabs.v2f32(<2 x float>
