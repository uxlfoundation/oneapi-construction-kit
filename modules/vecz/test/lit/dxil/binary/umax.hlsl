// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain umax.hlsl -Fo umax.bc

// RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
// RUN: %veczc -o %t.bc %S/umax.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %t

RWStructuredBuffer<uint3x2> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x][1] = max(a[gid.x][0], a[gid.x][1]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK-GE15: [[c:%.*]] = call <2 x i32> @llvm.umax.v2i32(<2 x i32> [[a:%.*]], <2 x i32> [[b:%.*]])
// CHECK-GE15: [[f:%.*]] = call <2 x i32> @llvm.umax.v2i32(<2 x i32> [[d:%.*]], <2 x i32> [[e:%.*]])

// CHECK-LT15: [[c:%[a-zA-Z0-9_]*]] = icmp ugt <2 x i32> [[a:%[a-zA-Z0-9_]*]], [[b:%[a-zA-Z0-9_]*]]
// CHECK-LT15: select <2 x i1> [[c]], <2 x i32> [[a]], <2 x i32> [[b]]
// CHECK-LT15: [[f:%[a-zA-Z0-9_]*]] = icmp ugt <2 x i32> [[d:%[a-zA-Z0-9_]*]], [[e:%[a-zA-Z0-9_]*]]
// CHECK-LT15: select <2 x i1> [[f]], <2 x i32> [[d]], <2 x i32> [[e]]