// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain dot2.hlsl -Fo dot2.bc

// RUN: %veczc -o %t.bc %S/dot2.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;
RWStructuredBuffer<float2> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = dot(a[gid.x], b[gid.x]);
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK:  [[a1b1:%[a-zA-Z0-9_]*]] = fmul <8 x float> [[a1:%[a-zA-Z0-9_]*]], [[b1:%[a-zA-Z0-9_]*]]
// CHECK:  [[a2b2:%[a-zA-Z0-9_]*]] = fmul <8 x float> [[a2:%[a-zA-Z0-9_]*]], [[b2:%[a-zA-Z0-9_]*]]
// CHECK:  [[c:%[a-zA-Z0-9_]*]] = fadd <8 x float> [[a1b1]], [[a2b2]]
