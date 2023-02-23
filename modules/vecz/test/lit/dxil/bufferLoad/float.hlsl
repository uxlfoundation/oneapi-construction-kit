// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain float.hlsl -Fo float.bc

// RUN: %veczc -o %t.bc %S/float.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = a[gid.x] + 13;
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK: fadd {{reassoc nnan ninf nsz arcp|fast}} <8 x float> {{%[a-zA-Z0-9_]*}}, <float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01, float 1.300000e+01>
