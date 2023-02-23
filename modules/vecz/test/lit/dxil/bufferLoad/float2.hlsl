// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain float2.hlsl -Fo float2.bc

// RUN: %veczc -o %t.bc %S/float2.bc -vecz-choices=TargetIndependentPacketization -k CSMain -w 32
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = a[gid.x] * 13 + 2;
}

// CHECK: define void @__vecz_v32_CSMain
// CHECK: [[mul0:%[a-zA-Z0-9_\.]*]] = fmul {{reassoc nnan ninf nsz arcp|fast}} <32 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01>
// CHECK: [[mul1:%[a-zA-Z0-9_\.]*]] = fmul {{reassoc nnan ninf nsz arcp|fast}} <32 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01>
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <32 x float> [[mul0]], <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <32 x float> [[mul1]], <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
