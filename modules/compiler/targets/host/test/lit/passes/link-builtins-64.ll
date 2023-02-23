; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: codegen_x86_64
; RUN: %muxc --device "%default_device" --passes link-builtins,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; We don't really care *what* the abs function looks like, but check it's been materialized
; CHECK: define {{.*}}spir_func i32 @_Z3absi(i32 {{.*%.+}}){{.*}} {
; CHECK:   ret i32 {{.*}}
; CHECK: }

; We don't really care *what* the get_global_id function looks like, but check it's been materialized
; CHECK: define {{.*}}spir_func i64 @_Z13get_global_idj(i32 {{.*%.+}}){{.*}} {
; CHECK:   ret i64 {{.*}}
; CHECK: }

declare spir_func i32 @_Z3absi(i32)
declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo() {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  ret void
}
