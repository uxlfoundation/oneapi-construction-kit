// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain legacyF16toF32.hlsl -Fo legacyF16toF32.bc

// RUN: %veczc -o %t.bc %S/legacyF16toF32.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint> a;
RWStructuredBuffer<float> b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  b[gid.x] = f16tof32(a[gid.x]);
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK: [[trunc:%[a-zA-Z0-9_]*]] = trunc <8 x i32> {{%[a-zA-Z0-9_]*}} to <8 x i16>
// CHECK: [[bitcast:%[a-zA-Z0-9_]*]] = bitcast <8 x i16> [[trunc]] to <8 x half>
// CHECK: fpext <8 x half> [[bitcast]] to <8 x float>
