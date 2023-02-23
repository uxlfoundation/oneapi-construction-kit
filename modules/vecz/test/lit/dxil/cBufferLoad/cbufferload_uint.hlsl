// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain cbufferload_uint.hlsl -Fo cbufferload_uint.bc -not_use_legacy_cbuf_load

// RUN: %veczc -o %t.bc %S/cbufferload_uint.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;

cbuffer foo { float b; int c; uint d;};

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = d;
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call i32 @dx.op.cbufferLoad.i32(
