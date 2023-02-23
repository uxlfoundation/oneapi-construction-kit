// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain makeDouble.hlsl -Fo makeDouble.bc

// RUN: %veczc -o %t.bc %S/makeDouble.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<double> a;
RWStructuredBuffer<double> b;

[numthreads(256, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = b[gid.x] + 42.42;
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: [[zextA:%[a-zA-Z0-9_]*]] = zext <2 x i32> [[in1:%[a-zA-Z0-9_]*]] to <2 x i64>
// CHECK: [[zextB:%[a-zA-Z0-9_]*]] = zext <2 x i32> [[in2:%[a-zA-Z0-9_]*]] to <2 x i64>
// CHECK: [[shift:%[a-zA-Z0-9_]*]] = shl nuw <2 x i64> [[zextB]], <i64 32, i64 32>
// CHECK: [[or:%[a-zA-Z0-9_]*]] = or <2 x i64>
// CHECK-DAG: [[shift]]
// CHECK-DAG: [[zextA]]
// CHECK: bitcast <2 x i64> [[or]] to <2 x double>
