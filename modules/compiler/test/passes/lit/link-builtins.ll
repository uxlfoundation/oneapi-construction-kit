; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes link-builtins,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; Without a target, there's no builtins to link
; CHECK: declare spir_func i32 @_Z3absi(i32)
; CHECK: declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func i32 @_Z3absi(i32)
declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo(i32 addrspace(1)* %in) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %addr = getelementptr i32, i32 addrspace(1)* %in, i64 %gid
  %x = load i32, i32 addrspace(1)* %addr
  %abs = call i32 @_Z3absi(i32 %x)
  store i32 %abs, i32 addrspace(1)* %addr
  ret void
}
