// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_2 -E CSMain rawbufferload_int.hlsl -Fo rawbufferload_int.bc

// RUN: %veczc -o %t.bc %S/rawbufferload_int.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;
ByteAddressBuffer b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = b.Load(gid.x);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK:  extractvalue %dx.types.ResRet.i32 [[a]], 0
// CHECK:  extractvalue %dx.types.ResRet.i32 [[b]], 0
