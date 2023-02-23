// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain round_pi.hlsl -Fo round_pi.bc

// RUN: %veczc -o %t.bc %S/round_pi.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].y = ceil(a[gid.x].x);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call <2 x float> @llvm.ceil.v2f32(<2 x float>
