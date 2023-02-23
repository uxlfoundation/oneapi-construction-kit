; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -vecz-choices=InstantiateCallsInLoops -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

@.str = private unnamed_addr addrspace(2) constant [23 x i8] c"Hello from %d with %d\0A\00", align 1
@.str.1 = private unnamed_addr addrspace(2) constant [14 x i8] c"Hello from %d\00", align 1

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([23 x i8], [23 x i8] addrspace(2)* @.str, i64 0, i64 0), i64 %call, i32 %0)
  %call2 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([14 x i8], [14 x i8] addrspace(2)* @.str.1, i64 0, i64 0), i64 %call)
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare extern_weak spir_func i32 @printf(i8 addrspace(2)*, ...)

; CHECK-GE15: define spir_kernel void @__vecz_v4_test(ptr addrspace(1) %in)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test(i32 addrspace(1)* %in)

; CHECK: [[LOOPHEADER1:instloop.header.*]]:
; CHECK: %[[INSTANCE1:instance.*]] = phi i32 [ 0, {{.+}} ], [ %[[V7:[0-9]+]], %[[LOOPBODY1:instloop.body.*]] ]
; CHECK: %[[V3:[0-9]+]] = icmp ult i32 %[[INSTANCE1]], 4
; CHECK: br i1 %[[V3]], label %[[LOOPBODY1]], label {{.+}}

; CHECK: [[LOOPBODY1]]:
; CHECK: %[[V4:[0-9]+]] = extractelement <4 x i64> %0, i32 %[[INSTANCE1]]
; CHECK: %[[V5:[0-9]+]] = extractelement <4 x i32> %{{.+}}, i32 %[[INSTANCE1]]
; CHECK-GE15: call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @{{.+}}, i64 %[[V4]], i32 %[[V5]])
; CHECK-LT15: call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([23 x i8], [23 x i8] addrspace(2)* @{{.+}}, i64 0, i64 0), i64 %[[V4]], i32 %[[V5]])
; CHECK: %[[V7]] = add i32 %[[INSTANCE1]], 1
; CHECK: br label %[[LOOPHEADER1]]

; CHECK: [[LOOPHEADER2:instloop.header.*]]:
; CHECK: %[[INSTANCE3:.+]] = phi i32 [ %[[V11:[0-9]+]], %[[LOOPBODY2:instloop.body.*]] ], [ 0, {{.+}} ]
; CHECK: %[[V8:[0-9]+]] = icmp ult i32 %[[INSTANCE3]], 4
; CHECK: br i1 %[[V8]], label %[[LOOPBODY2]], label {{.+}}

; CHECK: [[LOOPBODY2]]:
; CHECK: %[[V9:[0-9]+]] = extractelement <4 x i64> %0, i32 %[[INSTANCE3]]
; CHECK-GE15: call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @{{.+}}, i64 %[[V9]])
; CHECK-LT15: call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([14 x i8], [14 x i8] addrspace(2)* @{{.+}}, i64 0, i64 0), i64 %[[V9]])
; CHECK: %[[V11]] = add i32 %[[INSTANCE3]], 1
; CHECK: br label %[[LOOPHEADER2]]

; CHECK: ret void
