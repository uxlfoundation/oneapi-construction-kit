// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain int.hlsl -Fo int.bc

// RUN: %veczc -o %t.bc %S/int.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = 42;
}

// CHECK: define void @__vecz_v4_CSMain
// CHECK: [[a:%[a-zA-Z0-9_.]*]] = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)
// CHECK: [[x:%[a-zA-Z0-9_.]*]] = call i32 @dx.op.threadId.i32(i32 93, i32 0)
// CHECK: [[i:%[a-zA-Z0-9_.]*]] = insertelement <4 x i32> {{poison|undef}}, i32 [[x]], {{(i32|i64)}} 0
// CHECK: [[s:%[a-zA-Z0-9_.]*]] = shufflevector <4 x i32> [[i]], <4 x i32> {{poison|undef}}, <4 x i32>
// CHECK: [[d:%[a-zA-Z0-9_.]*]] = add <4 x i32> [[s]],
// CHECK: [[y:%[a-zA-Z0-9_.]*]] = extractelement <4 x i32> [[d]], {{(i32|i64)}} 1
// CHECK: [[z:%[a-zA-Z0-9_.]*]] = extractelement <4 x i32> [[d]], {{(i32|i64)}} 2
// CHECK: [[w:%[a-zA-Z0-9_.]*]] = extractelement <4 x i32> [[d]], {{(i32|i64)}} 3
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[x]], i32 0, i32 42, i32 undef, i32 undef, i32 undef, i8 1)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[y]], i32 0, i32 42, i32 undef, i32 undef, i32 undef, i8 1)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[z]], i32 0, i32 42, i32 undef, i32 undef, i32 undef, i8 1)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[w]], i32 0, i32 42, i32 undef, i32 undef, i32 undef, i8 1)
