// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_2 -E CSMain rawbufferload_float3.hlsl -Fo rawbufferload_float3.bc

// RUN: %veczc -o %t.bc %S/rawbufferload_float3.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float3> a;
ByteAddressBuffer b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = b.Load3(gid.x);
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[c:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[d:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.rawBufferLoad.i32
// CHECK: [[f1:%[a-zA-Z0-9_.]*]] = uitofp <4 x i32> [[x:%[a-zA-Z0-9_]*]] to <4 x float>
// CHECK: [[f2:%[a-zA-Z0-9_.]*]] = uitofp <4 x i32> [[y:%[a-zA-Z0-9_]*]] to <4 x float>
// CHECK: [[f3:%[a-zA-Z0-9_.]*]] = uitofp <4 x i32> [[z:%[a-zA-Z0-9_]*]] to <4 x float>
