// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain exp.hlsl -Fo exp.bc

// RUN: %veczc -o %t.bc %S/exp.bc -vecz-choices=TargetIndependentPacketization -k CSMain -w 16
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = exp(a[gid.x]);
}

// CHECK: define void @__vecz_v16_CSMain
// CHECK: call <16 x float> @llvm.exp2.v16f32(<16 x float>
