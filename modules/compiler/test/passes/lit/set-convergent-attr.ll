; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes set-convergent-attr,verify -S %s | %filecheck %s

target datalayout = "e-p:32:32:32-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir-unknown-unknown"

; CHECK: declare spir_func void @_Z7barrierj(i32) [[ATTRS:#[0-9]+]]
declare spir_func void @_Z7barrierj(i32)

; CHECK: define spir_func void @foo() [[ATTRS]]
define spir_func void @foo() {
  call spir_func void @_Z7barrierj(i32 0)
  ret void
}

; CHECK: define spir_kernel void @bar() [[ATTRS]]
define spir_kernel void @bar() {
  call spir_func void @foo()
  ret void
}

; CHECK: declare spir_func void @some_convergent_func() [[ATTRS]]
declare spir_func void @some_convergent_func() #0

; CHECK: define spir_func void @some_transitively_convergent_func() [[ATTRS]]
define spir_func void @some_transitively_convergent_func() {
  call spir_func void @some_convergent_func()
  ret void
}

; CHECK: declare spir_func void @some_unknown_func() [[ATTRS]]
declare spir_func void @some_unknown_func()

; We don't mark definitions convergent unless they are already convergent or
; call convergent functions.
; CHECK-NOT: define spir_func void @some_unknown_func_body() [[ATTRS]]
define spir_func void @some_unknown_func_body() {
  ret void
}

; CHECK: declare spir_func void @_Z18work_group_barrierj(i32) [[ATTRS]]
declare spir_func void @_Z18work_group_barrierj(i32)
; CHECK: declare spir_func void @_Z18work_group_barrierjj(i32, i32) [[ATTRS]]
declare spir_func void @_Z18work_group_barrierjj(i32, i32)
; CHECK: declare spir_func void @_Z18work_group_barrierj12memory_scope(i32, i32) [[ATTRS]]
declare spir_func void @_Z18work_group_barrierj12memory_scope(i32, i32)

; CHECK: declare spir_func void @__mux_sub_group_barrier(i32, i32, i32) [[ATTRS]]
declare spir_func void @__mux_sub_group_barrier(i32, i32, i32)
; CHECK: declare spir_func void @__mux_work_group_barrier(i32, i32, i32) [[ATTRS]]
declare spir_func void @__mux_work_group_barrier(i32, i32, i32)

; CHECK: declare i32 @_Z20sub_group_reduce_addi(i32) [[ATTRS]]
declare i32 @_Z20sub_group_reduce_addi(i32)
; CHECK: declare i32 @_Z21work_group_reduce_addi(i32) [[ATTRS]]
declare i32 @_Z21work_group_reduce_addi(i32)

; Intrinsics aren't convergent unless they've told us so already.
; CHECK: declare float @llvm.minimum.f32(float, float) [[MINIMUM_ATTRS:#[0-9]+]]
declare float @llvm.minimum.f32(float, float)

; CHECK: attributes [[ATTRS]] = { convergent }
; CHECK-NOT: attributes [[MINIMUM_ATTRS]]{{.*}}convergent

attributes #0 = { convergent }
