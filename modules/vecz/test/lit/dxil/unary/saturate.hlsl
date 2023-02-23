// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain saturate.hlsl -Fo saturate.bc

// RUN: %veczc -o %t.bc %S/saturate.bc -k CSMain -w 4
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float2x4> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = saturate(a[gid.x]);
}

// CHECK: define void @__vecz_v4_CSMain

// CHECK: [[min0:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in0:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min0]], <4 x float> zeroinitializer)

// CHECK: [[min1:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in1:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min1]], <4 x float> zeroinitializer)

// CHECK: [[min2:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in2:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min2]], <4 x float> zeroinitializer)

// CHECK: [[min3:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in3:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min3]], <4 x float> zeroinitializer)

// CHECK: [[min4:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in4:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min4]], <4 x float> zeroinitializer)

// CHECK: [[min5:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in5:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min5]], <4 x float> zeroinitializer)

// CHECK: [[min6:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in6:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min6]], <4 x float> zeroinitializer)

// CHECK: [[min7:%[a-zA-Z0-9_]*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[in7:%[a-zA-Z0-9_]*]], <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
// CHECK: call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[min7]], <4 x float> zeroinitializer)
