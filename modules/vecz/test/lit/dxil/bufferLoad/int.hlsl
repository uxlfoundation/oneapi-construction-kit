// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain int.hlsl -Fo int.bc

// RUN: %veczc -o %t.bc %S/int.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = a[gid.x] + 42;
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[a:%[a-zA-Z0-9_]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[x:%[a-zA-Z0-9_]*]] = call i32 @dx.op.threadId.i32(i32 93, i32 0)
// CHECK: [[y:%[a-zA-Z0-9_]*]] = add i32 [[x]], 1
// CHECK: [[lo_call:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]], i32 [[x]], i32 0)
// CHECK: [[hi_call:%[a-zA-Z0-9_]*]] = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle [[a]], i32 [[y]], i32 0)
// CHECK: [[lo_extract:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[lo_call]], 0
// CHECK: [[hi_extract:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[hi_call]], 0
// CHECK: [[partial:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> undef, i32 [[lo_extract]], {{(i32|i64)}} 0
// CHECK: [[vector:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> [[partial]], i32 [[hi_extract]], {{(i32|i64)}} 1
// CHECK: [[add:%[a-zA-Z0-9_]*]] = add nsw <2 x i32> [[vector]], <i32 42, i32 42>
// CHECK: [[lo_store:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add]], {{(i32|i64)}} 0
// CHECK: [[hi_store:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add]], {{(i32|i64)}} 1
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[x]], i32 0, i32 [[lo_store]], i32 undef, i32 undef, i32 undef, i8 1)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[y]], i32 0, i32 [[hi_store]], i32 undef, i32 undef, i32 undef, i8 1)
