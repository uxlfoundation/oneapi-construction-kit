// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain isnan.hlsl -Fo isnan.bc

// RUN: %veczc -o %t.bc %S/isnan.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;
StructuredBuffer<float> b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = isnan(b[gid.x]);
}

// CHECK: define void @__vecz_v8_CSMain

// CHECK: fcmp uno <8 x float> [[in:%[a-zA-Z0-9_]*]], zeroinitializer
