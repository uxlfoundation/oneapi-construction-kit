// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain sqrt.hlsl -Fo sqrt.bc

// RUN: %veczc -o %t.bc %S/sqrt.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float4> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].w = sqrt(a[gid.x].z);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call <4 x float> @llvm.sqrt.v4f32(<4 x float>
