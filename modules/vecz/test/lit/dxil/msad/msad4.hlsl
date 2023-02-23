// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain msad4.hlsl -Fo msad4.bc

// RUN: %veczc -o %t.bc %S/msad4.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint4> buf_accum;
RWStructuredBuffer<uint> buf_ref;
RWStructuredBuffer<uint> buf_src;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
    uint4 accum = {0,0,0,0};
	accum = msad4(
		buf_ref[3], 
	   uint2(buf_src[gid.x], buf_src[gid.x+1]), 
		accum);
    buf_accum[gid.x] = accum;
}

// This test checks msad and bfi which are both only generated from the msad4
// intrinsic.

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK-NEXT: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK-NEXT: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK-NEXT: call i32 @dx.op.quaternary.i32(i32 53,
// CHECK: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK-NEXT: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK-NEXT: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK-NEXT: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK: call i32 @dx.op.tertiary.i32(i32 50,
// CHECK-NEXT: call i32 @dx.op.tertiary.i32(i32 50,
