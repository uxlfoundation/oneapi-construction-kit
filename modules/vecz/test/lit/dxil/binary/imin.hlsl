// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain imin.hlsl -Fo imin.bc

// RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
// RUN: %veczc -o %t.bc %S/imin.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %t

RWStructuredBuffer<int3> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x].z = min(a[gid.x].x, a[gid.x].y);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK-GE15: [[c:%.*]] = call <2 x i32> @llvm.smin.v2i32(<2 x i32> [[a:%.*]], <2 x i32> [[b:%.*]])
// CHECK-LT15: [[c:%[a-zA-Z0-9_]*]] = icmp slt <2 x i32> [[a:%[a-zA-Z0-9_]*]], [[b:%[a-zA-Z0-9_]*]]
// CHECK-LT15: select <2 x i1> [[c]], <2 x i32> [[a]], <2 x i32> [[b]]
