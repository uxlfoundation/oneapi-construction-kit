// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain float3.hlsl -Fo float3.bc

// RUN: %veczc -o %t.bc %S/float3.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float3> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = a[gid.x] * 13 + 2;
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[mul0:%[a-zA-Z0-9_\.]*]] = fmul {{reassoc nnan ninf nsz arcp|fast}} <2 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01>
// CHECK: [[mul1:%[a-zA-Z0-9_\.]*]] = fmul {{reassoc nnan ninf nsz arcp|fast}} <2 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01>
// CHECK: [[mul2:%[a-zA-Z0-9_\.]*]] = fmul {{reassoc nnan ninf nsz arcp|fast}} <2 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01>
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <2 x float> [[mul0]], <float 2.000000e+00, float 2.000000e+00>
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <2 x float> [[mul1]], <float 2.000000e+00, float 2.000000e+00>
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <2 x float> [[mul2]], <float 2.000000e+00, float 2.000000e+00>
