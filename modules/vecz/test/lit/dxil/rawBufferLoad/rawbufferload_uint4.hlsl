// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_2 -E CSMain rawbufferload_uint4.hlsl -Fo rawbufferload_uint4.bc

// RUN: %veczc -o %t.bc %S/rawbufferload_uint4.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<uint4> a;
ByteAddressBuffer b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = b.Load4(gid.x);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[c:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[d:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK:  extractvalue %dx.types.ResRet.i32 [[a]], 0
// CHECK:  extractvalue %dx.types.ResRet.i32 [[b]], 0
// CHECK:  extractvalue %dx.types.ResRet.i32 [[c]], 0
// CHECK:  extractvalue %dx.types.ResRet.i32 [[d]], 0
// CHECK:  extractvalue %dx.types.ResRet.i32 [[a]], 1
// CHECK:  extractvalue %dx.types.ResRet.i32 [[b]], 1
// CHECK:  extractvalue %dx.types.ResRet.i32 [[c]], 1
// CHECK:  extractvalue %dx.types.ResRet.i32 [[d]], 1
// CHECK:  extractvalue %dx.types.ResRet.i32 [[a]], 2
// CHECK:  extractvalue %dx.types.ResRet.i32 [[b]], 2
// CHECK:  extractvalue %dx.types.ResRet.i32 [[c]], 2
// CHECK:  extractvalue %dx.types.ResRet.i32 [[d]], 2
// CHECK:  extractvalue %dx.types.ResRet.i32 [[a]], 3
// CHECK:  extractvalue %dx.types.ResRet.i32 [[b]], 3
// CHECK:  extractvalue %dx.types.ResRet.i32 [[c]], 3
// CHECK:  extractvalue %dx.types.ResRet.i32 [[d]], 3
