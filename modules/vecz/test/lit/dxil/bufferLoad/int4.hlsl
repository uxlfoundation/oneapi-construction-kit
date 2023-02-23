// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain int4.hlsl -Fo int4.bc

// RUN: %veczc -o %t.bc %S/int4.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<int4> a;

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

// CHECK: [[lo_extract0:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[lo_call]], 0
// CHECK: [[hi_extract0:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[hi_call]], 0
// CHECK: [[partial0:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> undef, i32 [[lo_extract0]], {{(i32|i64)}} 0
// CHECK: [[vector0:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> [[partial0]], i32 [[hi_extract0]], {{(i32|i64)}} 1

// CHECK: [[lo_extract1:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[lo_call]], 1
// CHECK: [[hi_extract1:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[hi_call]], 1
// CHECK: [[partial1:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> undef, i32 [[lo_extract1]], {{(i32|i64)}} 0
// CHECK: [[vector1:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> [[partial1]], i32 [[hi_extract1]], {{(i32|i64)}} 1

// CHECK: [[lo_extract2:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[lo_call]], 2
// CHECK: [[hi_extract2:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[hi_call]], 2
// CHECK: [[partial2:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> undef, i32 [[lo_extract2]], {{(i32|i64)}} 0
// CHECK: [[vector2:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> [[partial2]], i32 [[hi_extract2]], {{(i32|i64)}} 1

// CHECK: [[lo_extract3:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[lo_call]], 3
// CHECK: [[hi_extract3:%[a-zA-Z0-9_]*]] = extractvalue %dx.types.ResRet.i32 [[hi_call]], 3
// CHECK: [[partial3:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> undef, i32 [[lo_extract3]], {{(i32|i64)}} 0
// CHECK: [[vector3:%[a-zA-Z0-9_]*]] = insertelement <2 x i32> [[partial3]], i32 [[hi_extract3]], {{(i32|i64)}} 1

// CHECK: [[add0:%[a-zA-Z0-9_\.]*]] = add <2 x i32> [[vector0]], <i32 42, i32 42>
// CHECK: [[lo_store0:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add0]], {{(i32|i64)}} 0
// CHECK: [[hi_store0:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add0]], {{(i32|i64)}} 1

// CHECK: [[add1:%[a-zA-Z0-9_\.]*]] = add <2 x i32> [[vector1]], <i32 42, i32 42>
// CHECK: [[lo_store1:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add1]], {{(i32|i64)}} 0
// CHECK: [[hi_store1:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add1]], {{(i32|i64)}} 1

// CHECK: [[add2:%[a-zA-Z0-9_\.]*]] = add <2 x i32> [[vector2]], <i32 42, i32 42>
// CHECK: [[lo_store2:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add2]], {{(i32|i64)}} 0
// CHECK: [[hi_store2:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add2]], {{(i32|i64)}} 1

// CHECK: [[add3:%[a-zA-Z0-9_\.]*]] = add <2 x i32> [[vector3]], <i32 42, i32 42>
// CHECK: [[lo_store3:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add3]], {{(i32|i64)}} 0
// CHECK: [[hi_store3:%[a-zA-Z0-9_]*]] = extractelement <2 x i32> [[add3]], {{(i32|i64)}} 1

// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[x]], i32 0, i32 [[lo_store0]], i32 [[lo_store1]], i32 [[lo_store2]], i32 [[lo_store3]], i8 15)
// CHECK: call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle [[a]], i32 [[y]], i32 0, i32 [[hi_store0]], i32 [[hi_store1]], i32 [[hi_store2]], i32 [[hi_store3]], i8 15)
