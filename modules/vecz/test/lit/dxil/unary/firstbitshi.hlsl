// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain firstbitshi.hlsl -Fo firstbitshi.bc

// RUN: %veczc -o %t.bc %S/firstbitshi.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int2x1> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = firstbithigh(a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK:  [[l1:%.[a-zA-Z0-9_]*]] = ashr <2 x i32> [[a:%[a-zA-Z0-9_]*]], <i32 31, i32 31>
// CHECK:  [[x:%[a-zA-Z0-9_]*]] = xor <2 x i32> [[a]], [[l1]]
// CHECK:  [[y:%[a-zA-Z0-9_]*]] = call <2 x i32> @llvm.ctlz.v2i32(<2 x i32> [[x]], i1 true)
// CHECK:  [[l2:%.[a-zA-Z0-9_]*]] = ashr <2 x i32> [[b:%[a-zA-Z0-9_]*]], <i32 31, i32 31>
// CHECK:  [[z:%[a-zA-Z0-9_]*]] = xor <2 x i32> [[b]], [[l2]]
// CHECK:  [[w:%[a-zA-Z0-9_]*]] = call <2 x i32> @llvm.ctlz.v2i32(<2 x i32> [[z]], i1 true)
