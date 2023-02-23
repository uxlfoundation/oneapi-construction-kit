// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain dot4.hlsl -Fo dot4.bc

// RUN: %veczc -o %t.bc %S/dot4.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float4> a;
RWStructuredBuffer<float4> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = dot(a[gid.x], b[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK:  [[a1b1:%[a-zA-Z0-9_]*]] = fmul <2 x float> [[a1:%[a-zA-Z0-9_]*]], [[b1:%[a-zA-Z0-9_]*]]
// CHECK:  [[a2b2:%[a-zA-Z0-9_]*]] = fmul <2 x float> [[a2:%[a-zA-Z0-9_]*]], [[b2:%[a-zA-Z0-9_]*]]
// CHECK:  [[a3b3:%[a-zA-Z0-9_]*]] = fmul <2 x float> [[a3:%[a-zA-Z0-9_]*]], [[b3:%[a-zA-Z0-9_]*]]
// CHECK:  [[a4b4:%[a-zA-Z0-9_]*]] = fmul <2 x float> [[a4:%[a-zA-Z0-9_]*]], [[b4:%[a-zA-Z0-9_]*]]
// CHECK:  [[c:%[a-zA-Z0-9_]*]] = fadd <2 x float> [[a1b1]], [[a2b2]]
// CHECK:  [[d:%[a-zA-Z0-9_]*]] = fadd <2 x float> [[a3b3]], [[a4b4]]
// CHECK:  [[e:%[a-zA-Z0-9_]*]] = fadd <2 x float> [[c]], [[d]]
