// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain fmin.hlsl -Fo fmin.bc

// RUN: %veczc -o %t.bc %S/fmin.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].x = min(a[gid.x].x, a[gid.x].y);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call <2 x float> @llvm.minnum.v2f32(<2 x float> [[a:%[a-zA-Z0-9_]*]], <2 x float>
