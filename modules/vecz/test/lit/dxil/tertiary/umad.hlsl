// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain umad.hlsl -Fo umad.bc

// RUN: %veczc -o %t.bc %S/umad.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint3x3> a;
RWStructuredBuffer<uint3x2> b;
RWStructuredBuffer<uint2x3> c;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = mul(b[gid.x], c[gid.x]);
}

// unsigned ints also use IMAd rather than UMad.

// CHECK: define void @__vecz_v8_CSMain
// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>

// CHECK: mul <8 x i32>
// CHECK: mul <8 x i32>
// CHECK-NEXT: add <8 x i32>
