// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_2 -E CSMain rawbufferstore_uint2.hlsl -Fo rawbufferstore_uint2.bc

// RUN: %veczc -o %t.bc %S/rawbufferstore_uint2.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint2> a;
RWByteAddressBuffer b;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  b.Store2(gid.x, a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK:  [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 1, i32 1, i1 false)
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[x:%[a-zA-Z0-9_]*]] = call i32 @dx.op.threadId.i32(i32 93, i32 0)
// CHECK: [[y:%[a-zA-Z0-9_]*]] = add i32 [[x]], 1
// CHECK: [[i0:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 %RawBufferLoad1, 0
// CHECK: [[i1:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 %RawBufferLoad2, 0
// CHECK: [[i2:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 %RawBufferLoad1, 1
// CHECK: [[i3:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 %RawBufferLoad2, 1
// CHECK: call void @dx.op.rawBufferStore.i32(i32 140, %dx.types.Handle [[a]], i32 [[x]], i32 undef, i32 [[i0]], i32 [[i2]], i32 undef, i32 undef, i8 3, i32 4)
// CHECK: call void @dx.op.rawBufferStore.i32(i32 140, %dx.types.Handle [[a]], i32 [[y]], i32 undef, i32 [[i1]], i32 [[i3]], i32 undef, i32 undef, i8 3, i32 4)
