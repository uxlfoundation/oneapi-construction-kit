// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_2 -E CSMain splitDouble.hlsl -Fo splitDouble.bc

// RUN: %veczc -o %t.bc %S/splitDouble.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<double> a;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = gid.x;
}

// CHECK: define void @__vecz_v4_CSMain

// CHECK: [[bitcast:%[a-zA-Z0-9_]*]] = bitcast <4 x double> {{%[a-zA-Z0-9_]*}} to <4 x i64>
// CHECK: [[truncA:%[a-zA-Z0-9_]*]] = trunc <4 x i64> [[bitcast]] to <4 x i32>
// CHECK: [[truncA0:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncA]], {{(i32|i64)}} 0
// CHECK: [[truncA1:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncA]], {{(i32|i64)}} 1
// CHECK: [[truncA2:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncA]], {{(i32|i64)}} 2
// CHECK: [[truncA3:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncA]], {{(i32|i64)}} 3
// CHECK: [[shift:%[a-zA-Z0-9_]*]] = lshr <4 x i64> [[bitcast]], <i64 32, i64 32, i64 32, i64 32>
// CHECK: [[truncB:%[a-zA-Z0-9_]*]] = trunc <4 x i64> [[shift]] to <4 x i32>
// CHECK: [[truncB0:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncB]], {{(i32|i64)}} 0
// CHECK: [[truncB1:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncB]], {{(i32|i64)}} 1
// CHECK: [[truncB2:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncB]], {{(i32|i64)}} 2
// CHECK: [[truncB3:%[a-zA-Z0-9_]*]] = extractelement <4 x i32> [[truncB]], {{(i32|i64)}} 3
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %a_UAV_structbuf, i32 %0, i32 0, i32 [[truncA0]], i32 [[truncB0]],
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %a_UAV_structbuf, i32 %2, i32 0, i32 [[truncA1]], i32 [[truncB1]],
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %a_UAV_structbuf, i32 %3, i32 0, i32 [[truncA2]], i32 [[truncB2]],
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %a_UAV_structbuf, i32 %4, i32 0, i32 [[truncA3]], i32 [[truncB3]],
