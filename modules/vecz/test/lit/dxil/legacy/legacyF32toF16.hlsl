// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain legacyF32toF16.hlsl -Fo legacyF32toF16.bc

// RUN: %veczc -o %t.bc %S/legacyF32toF16.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float> a;
RWStructuredBuffer<uint> b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  b[gid.x] = f32tof16(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[trunc:%[a-zA-Z0-9_]*]] = fptrunc <4 x float> {{%[a-zA-Z0-9_]*}} to <4 x half>
// CHECK: [[bitcast:%[a-zA-Z0-9_]*]] = bitcast <4 x half> [[trunc]] to <4 x i16>
// CHECK: zext <4 x i16> [[bitcast]] to <4 x i32>
