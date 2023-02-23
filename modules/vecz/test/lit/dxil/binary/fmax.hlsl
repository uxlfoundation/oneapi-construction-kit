// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain fmax.hlsl -Fo fmax.bc

// RUN: %veczc -o %t.bc %S/fmax.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].x = max(a[gid.x].x, a[gid.x].y);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[a:%[a-zA-Z0-9_]*]], <4 x float>
