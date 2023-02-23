// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain tan.hlsl -Fo tan.bc

// RUN: %veczc -o %t.bc %S/tan.bc -vecz-choices=TargetIndependentPacketization -k CSMain -w 32
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = tan(a[gid.x]);
}

// CHECK: define void @__vecz_v32_CSMain

// CHECK: [[sin:%[a-zA-Z0-9_]*]] = call <32 x float> @llvm.sin.v32f32(<32 x float> [[in:%[a-zA-Z0-9_]*]])
// CHECK: [[cos:%[a-zA-Z0-9_]*]] = call <32 x float> @llvm.cos.v32f32(<32 x float> [[in]])
// CHECK: fdiv <32 x float> [[sin]], [[cos]]
