// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain fmad.hlsl -Fo fmad.bc

// RUN: %veczc -o %t.bc %S/fmad.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2x2> a;
RWStructuredBuffer<float2x2> b;
RWStructuredBuffer<float2x2> c;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = mul(b[gid.x], c[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: call <4 x float> @llvm.fmuladd.v4f32
// CHECK: call <4 x float> @llvm.fmuladd.v4f32
// CHECK: call <4 x float> @llvm.fmuladd.v4f32
// CHECK: call <4 x float> @llvm.fmuladd.v4f32
