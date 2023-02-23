; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

%struct_type = type { i32, i32 }

define spir_kernel void @test(i32* %in, i32* %out, %struct_type* %sin) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %inp = getelementptr inbounds i32, i32* %in, i64 %call
  %oup = getelementptr inbounds i32, i32* %out, i64 %call
  %o = load i32, i32* %oup
  ; do this little compare + phi to throw off the InstCombine pass and ensure
  ; we end up with a phi %struct_type that must be instantiated
  %s = insertvalue %struct_type undef, i32 %o, 1
  %cmpcall = icmp ult i64 16, %call
  br i1 %cmpcall, label %lower, label %higher

lower:
  %lowers = insertvalue %struct_type %s, i32 0, 0
  br label %lower.higher.phi

higher:
  %highers = insertvalue %struct_type %s, i32 1, 0
  br label %lower.higher.phi

lower.higher.phi:
  %lowerhigherstruct = phi %struct_type [%lowers, %lower], [%highers, %higher]
  br label %for.cond

for.cond:
  %storemerge = phi %struct_type [ %incv, %for.inc ], [ %lowerhigherstruct, %lower.higher.phi ]
  %s1 = extractvalue %struct_type %storemerge, 1
  %s1ext = zext i32 %s1 to i64
  %cmp = icmp ult i64 %s1ext, %call
  br i1 %cmp, label %for.body, label %for.end

for.body:
  %l = load i32, i32* %inp, align 4
  store i32 %l, i32* %oup, align 4
  br label %for.inc

for.inc:
  %toadd = extractvalue %struct_type %storemerge, 1
  %toadd64 = zext i32 %toadd to i64
  %ca = add i64 %toadd64, %call
  %sinp = getelementptr inbounds %struct_type, %struct_type* %sin, i64 %ca
  %sinv = load %struct_type, %struct_type* %sinp
  %sinintv = extractvalue %struct_type %sinv, 1
  %incv = insertvalue %struct_type %storemerge, i32 %sinintv, 1
  br label %for.cond

for.end:
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare void @llvm.memset.p0i8.i32(i8*,i8,i32,i32,i1)

; CHECK: define spir_kernel void @__vecz_v4_test

; Check if the struct creation has been instantiated
; CHECK-LT15: %[[V1:[0-9]+]] = bitcast i32* %oup to <4 x i32>*
; CHECK-GE15: %[[V2:[0-9]+]] = load <4 x i32>, ptr %oup, align 4
; CHECK-LT15: %[[V2:[0-9]+]] = load <4 x i32>, <4 x i32>* %[[V1]], align 4
; CHECK: %[[V3:[0-9]+]] = extractelement <4 x i32> %[[V2]], {{(i32|i64)}} 0
; CHECK: %[[V4:[0-9]+]] = extractelement <4 x i32> %[[V2]], {{(i32|i64)}} 1
; CHECK: %[[V5:[0-9]+]] = extractelement <4 x i32> %[[V2]], {{(i32|i64)}} 2
; CHECK: %[[V6:[0-9]+]] = extractelement <4 x i32> %[[V2]], {{(i32|i64)}} 3
; CHECK: %[[S24:.+]] = insertvalue %struct_type undef, i32 %[[V3]], 1
; CHECK: %[[S25:.+]] = insertvalue %struct_type undef, i32 %[[V4]], 1
; CHECK: %[[S26:.+]] = insertvalue %struct_type undef, i32 %[[V5]], 1
; CHECK: %[[S27:.+]] = insertvalue %struct_type undef, i32 %[[V6]], 1

; Check if the phi node has been instantiated
; CHECK: phi %struct_type [ %{{.+}}, %entry ], [ %{{.+}}, %for.cond ]
; CHECK: phi %struct_type [ %{{.+}}, %entry ], [ %{{.+}}, %for.cond ]
; CHECK: phi %struct_type [ %{{.+}}, %entry ], [ %{{.+}}, %for.cond ]
; CHECK: phi %struct_type [ %{{.+}}, %entry ], [ %{{.+}}, %for.cond ]
; CHECK: extractvalue %struct_type %{{.+}}, 1
; CHECK: extractvalue %struct_type %{{.+}}, 1
; CHECK: extractvalue %struct_type %{{.+}}, 1
; CHECK: extractvalue %struct_type %{{.+}}, 1

; Check if the operations that use integer types are vectorized
; CHECK: zext <4 x i32>
; CHECK: icmp ugt <4 x i64>
; CHECK: and <4 x i1>
; CHECK-GE15: %[[L423:.+]] = call <4 x i32> @__vecz_b_masked_load4_Dv4_ju3ptrDv4_b(ptr %{{.*}}, <4 x i1>
; CHECK-LT15: %[[L423:.+]] = call <4 x i32> @__vecz_b_masked_load4_Dv4_jPDv4_jDv4_b(<4 x i32>* %{{.*}}, <4 x i1>
; CHECK-GE15: call void @__vecz_b_masked_store4_Dv4_ju3ptrDv4_b(<4 x i32> %[[L423]], ptr{{( nonnull)? %.*}}, <4 x i1>
; CHECK-LT15: call void @__vecz_b_masked_store4_Dv4_jPDv4_jDv4_b(<4 x i32> %[[L423]], <4 x i32>*{{( nonnull)? %.*}}, <4 x i1>

; CHECK: ret void
