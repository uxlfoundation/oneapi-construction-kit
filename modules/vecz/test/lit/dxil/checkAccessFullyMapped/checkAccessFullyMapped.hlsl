// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain checkAccessFullyMapped.hlsl -Fo checkAccessFullyMapped.bc

// RUN: %veczc -o %t.bc %S/checkAccessFullyMapped.bc -k CSMain -w 8
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;
ByteAddressBuffer b;
RWStructuredBuffer<bool> c;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  uint mapFeedbackHandle;
  a[gid.x] = b.Load(gid.x, mapFeedbackHandle);
  c[gid.x] = CheckAccessFullyMapped(mapFeedbackHandle);
}

// CHECK: define void @__vecz_v8_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[b:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[c:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[d:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[e:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[f:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[g:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32
// CHECK: [[h:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32

// CHECK: [[mapInfoA:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[a]], 4
// CHECK: [[mapInfoB:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[b]], 4
// CHECK: [[mapInfoC:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[c]], 4
// CHECK: [[mapInfoD:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[d]], 4
// CHECK: [[mapInfoE:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[e]], 4
// CHECK: [[mapInfoF:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[f]], 4
// CHECK: [[mapInfoG:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[g]], 4
// CHECK: [[mapInfoH:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[h]], 4

// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoA]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoB]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoC]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoD]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoE]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoF]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoG]])
// CHECK: call i1 @dx.op.checkAccessFullyMapped.i32(i32 71, i32 [[mapInfoH]])
